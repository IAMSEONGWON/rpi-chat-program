#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../common/defs.h"

using namespace std;

#define MAX_CLNT 10

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
mutex mtx;

// sender_sock가 -1이면 "모두에게(Server 공지)", 아니면 "보낸 사람 빼고" 전송
void send_msg(char *msg, int len, int sender_sock) {
    lock_guard<mutex> guard(mtx);
    for (int i = 0; i < clnt_cnt; i++) {
        // -1인 경우(공지)는 무조건 전송, 아니면 보낸 사람 제외하고 전송
        if (sender_sock == -1 || clnt_socks[i] != sender_sock) {
            write(clnt_socks[i], msg, len);
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

int main() {
    // 서버 자체 터미널 로그는 끔 (화면 꼬임 방지)
    freopen("/dev/null", "w", stdout);

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