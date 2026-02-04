#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <map> 
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream> 
#include <signal.h> 
#include "../common/defs.h"

using namespace std;

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
mutex mtx;

// 영상 중계용 저장소 (Key: 닉네임, Value: JPG 데이터)
map<string, vector<unsigned char>> frame_map;
mutex video_mtx;

// sender_sock가 -1이면 "모두에게(Server 공지)", 아니면 "보낸 사람 빼고" 전송
void send_msg(char *msg, int len, int sender_sock) {
    lock_guard<mutex> guard(mtx);
    for (int i = 0; i < clnt_cnt; i++) {
        // -1인 경우(공지)는 무조건 전송, 아니면 보낸 사람 제외하고 전송
        if (sender_sock == -1 || clnt_socks[i] != sender_sock) {
             // MSG_NOSIGNAL 옵션 사용 (혹시 모를 2차 방어)
            send(clnt_socks[i], msg, len, MSG_NOSIGNAL);
        }
    }

    // 파일에 로그 저장, "a" 모드: 파일 끝에 이어쓰기 (append)
    FILE *fp = fopen("chat_log.txt", "a"); 
    if (fp != nullptr) {
        // 줄바꿈이 없는 메시지일 수 있으니 뒤에 \n을 붙여서 저장하면 보기 편함
        // (단, 클라이언트가 이미 \n을 보냈는지에 따라 조정 필요. 보통은 그냥 저장)
        fprintf(fp, "%s\n", msg);
        fclose(fp);
    }
}

// 인자에 'string client_ip' 추가 (IP를 받아오기 위함)
void handle_clnt(int clnt_sock, string client_ip) {
    int str_len = 0;
    char msg[BUF_SIZE];
    char notice[BUF_SIZE]; // 공지용 버퍼
    char nickname[BUF_SIZE] = "Unknown"; 

    // 접속하자마자 닉네임 수신
    int name_len = read(clnt_sock, nickname, sizeof(nickname)-1);
    if (name_len > 0) {
        nickname[name_len] = 0; 
    }

    // 닉네임과 IP를 합쳐서 접속 알림 전송
    // 포맷: [Server] '닉네임(IP)' connected.
    sprintf(notice, "[Server] '%s(%s)' connected. (Total: %d)", 
            nickname, client_ip.c_str(), clnt_cnt);
    send_msg(notice, strlen(notice), -1); // 전체 공지

    // 채팅 루프
    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) {
        send_msg(msg, str_len, clnt_sock);
    }

    // 접속 종료 처리
    mtx.lock();
    for (int i = 0; i < clnt_cnt; i++) {
        if (clnt_socks[i] == clnt_sock) {
            while (i < clnt_cnt - 1) {
                clnt_socks[i] = clnt_socks[i + 1];
                i++;
            }
            break;
        }
    }
    clnt_cnt--;
    mtx.unlock();

    // 나갈 때도 닉네임(IP) 형식으로 알림
    sprintf(notice, "[Server] '%s(%s)' disconnected. (Total: %d)", 
            nickname, client_ip.c_str(), clnt_cnt);
    send_msg(notice, strlen(notice), -1);
    
    close(clnt_sock);
}

// 영상 서버 스레드 함수 (9999번 포트 - 중계 역할)
void video_relay_thread() {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(VIDEO_PORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) return;
    if (listen(serv_sock, 5) == -1) return;

    // 접속 대기 루프
    while (true) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        
        // 새로운 클라이언트를 처리할 스레드 생성 (람다 함수 사용)
        thread([clnt_sock]() {
            char header_buf[1024];
            // 헤더를 살짝 읽어서(MSG_PEEK) 요청 종류 파악
            int len = recv(clnt_sock, header_buf, 1023, MSG_PEEK);
            if (len <= 0) { close(clnt_sock); return; }
            header_buf[len] = 0;
            string header(header_buf);

            // 웹 브라우저 (시청자) 접속: "GET /닉네임"
            if (header.find("GET /") == 0) {
                recv(clnt_sock, header_buf, 1023, 0); // 버퍼 비우기
                
                // URL에서 닉네임 추출
                string target_nick = "Unknown";
                size_t start = 5; // "GET /" 다음
                size_t end = header.find(" ", start);
                if(end != string::npos) target_nick = header.substr(start, end - start);

                // HTTP 응답 헤더 전송
                string http_head = "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
                // MSG_NOSIGNAL 추가
                send(clnt_sock, http_head.c_str(), http_head.length(), MSG_NOSIGNAL);

                // 영상 전송 루프
                while(true) {
                    vector<unsigned char> frame_data;
                    {
                        lock_guard<mutex> lock(video_mtx);
                        if (frame_map.count(target_nick)) {
                            frame_data = frame_map[target_nick];
                        }
                    }

                    if (!frame_data.empty()) {
                        string boundary = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + to_string(frame_data.size()) + "\r\n\r\n";
                        
                        // 전송 실패(웹 닫음) 시 루프 탈출, MSG_NOSIGNAL 사용
                        if (send(clnt_sock, boundary.c_str(), boundary.length(), MSG_NOSIGNAL) < 0) break;
                        if (send(clnt_sock, frame_data.data(), frame_data.size(), MSG_NOSIGNAL) < 0) break;
                        if (send(clnt_sock, "\r\n", 2, MSG_NOSIGNAL) < 0) break;
                    }
                    usleep(50000); // 약 20 FPS (웹 부하 줄이기)
                }
            }
            // 클라이언트 앱 (송출자) 접속: "UPLOAD 닉네임"
            else if (header.find("UPLOAD") == 0) {
                // "UPLOAD 닉네임\n" 읽기
                int r = recv(clnt_sock, header_buf, 1023, 0);
                header_buf[r] = 0;
                stringstream ss(header_buf);
                string cmd, my_nick;
                ss >> cmd >> my_nick;

                // 영상 수신 루프
                while(true) {
                    int img_size;
                    // 크기 수신 (4byte)
                    if (recv(clnt_sock, &img_size, sizeof(int), 0) <= 0) break;
                    
                    vector<unsigned char> img_data(img_size);
                    int total = 0;
                    while(total < img_size) {
                        int r = recv(clnt_sock, img_data.data() + total, img_size - total, 0);
                        if (r <= 0) break;
                        total += r;
                    }
                    if (total < img_size) break;

                    // 맵 업데이트
                    {
                        lock_guard<mutex> lock(video_mtx);
                        frame_map[my_nick] = img_data;
                    }
                }
            }
            close(clnt_sock);
        }).detach();
    }
    close(serv_sock);
}

int main() {
    // SIGPIPE 시그널 무시 (웹/클라이언트 연결 끊김 시 서버 다운 방지)
    signal(SIGPIPE, SIG_IGN);

    // 서버 자체 터미널 로그는 끔 (화면 꼬임 방지)
    freopen("/dev/null", "w", stdout);

    // 영상 서버 스레드 시작 (백그라운드 실행)
    thread video_t(video_relay_thread);
    video_t.detach();

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    
    // notice 변수는 main에서 사용하지 않으므로 삭제

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(PORT_NUM);

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1) return 1;
    if (listen(serv_sock, 5) == -1) return 1;

    while (true) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        mtx.lock();
        if(clnt_cnt >= MAX_CLNT) {
            close(clnt_sock);
            mtx.unlock();
            continue;
        }
        clnt_socks[clnt_cnt++] = clnt_sock;
        mtx.unlock();

        // 여기서 바로 공지를 띄우지 않고, IP 문자열을 만들어서 스레드에 넘김
        string client_ip = inet_ntoa(clnt_adr.sin_addr);

        // 스레드 생성 시 IP 문자열도 같이 전달
        thread t(handle_clnt, clnt_sock, client_ip);
        t.detach();
    }

    close(serv_sock);
    return 0;
}