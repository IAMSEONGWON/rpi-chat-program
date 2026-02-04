#include <iostream>
#include <thread>
#include <string>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h> // 터미널 제어용 헤더
#include <opencv2/opencv.hpp> // OpenCV 및 비디오 설정
#include <signal.h>  
#include <atomic>
#include "../common/defs.h" 

using namespace cv;
using namespace std;

// 전역 변수 
int sock; // 채팅 소켓
string current_input = ""; // 사용자가 현재 치고 있는 내용을 저장
mutex input_mtx;           // 화면 출력 충돌 방지용 뮤텍스
char name[BUF_SIZE];       // 닉네임 저장할 변수
atomic<bool> is_running(true); // 프로그램 실행 상태 플래그 (true면 실행, false면 종료)

// 영상 송출 스레드
void send_video_thread(string nickname) {
    int vid_sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    
    // defs.h에 있는 VIDEO_SERVER_IP 사용
    serv_addr.sin_addr.s_addr = inet_addr(VIDEO_SERVER_IP); 
    serv_addr.sin_port = htons(VIDEO_PORT);

    if (connect(vid_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        // 영상 연결 실패해도 채팅은 계속 진행
        return;
    }

    // 프로토콜 전송: "UPLOAD 닉네임"
    string cmd = "UPLOAD " + nickname + "\n";
    send(vid_sock, cmd.c_str(), cmd.length(), 0);

    // 카메라 열기 (V4L2 + MJPG)
    VideoCapture cap(0, CAP_V4L2);
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap.set(CAP_PROP_FRAME_WIDTH, 320);
    cap.set(CAP_PROP_FRAME_HEIGHT, 240);
    cap.set(CAP_PROP_FPS, 30);

    if (!cap.isOpened()) { close(vid_sock); return; }

    Mat frame;
    vector<uchar> buffer;
    vector<int> param = {IMWRITE_JPEG_QUALITY, 50};

    while (is_running) {
        cap >> frame;
        if (frame.empty()) continue;

        // JPG 인코딩
        imencode(".jpg", frame, buffer, param);

        // 데이터 전송: [크기(4byte)] + [데이터]
        int img_size = buffer.size();
        
        // MSG_NOSIGNAL 사용 및 에러 체크 (연결 끊기면 루프 탈출)
        if (send(vid_sock, &img_size, sizeof(int), MSG_NOSIGNAL) < 0) break;
        if (send(vid_sock, buffer.data(), img_size, MSG_NOSIGNAL) < 0) break;

        usleep(33000); // 30 FPS
    }
    cap.release();
    close(vid_sock);
}

// 터미널 설정 (Canonical 모드 끄기)
// 엔터를 치지 않아도 키 입력을 바로 받고, 화면에 자동으로 글자가 안 찍히게 함
struct termios orig_termios;

void reset_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode() {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));
    atexit(reset_terminal_mode); // 프로그램 종료 시 원래대로 복구

    new_termios.c_lflag &= ~(ICANON | ECHO); // ICANON(버퍼), ECHO(자동출력) 끄기
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

// 키보드 입력 하나 받기 (getch 구현)
int getch() {
    int r;
    unsigned char c;
    if ((r = read(STDIN_FILENO, &c, sizeof(c))) < 0) return r;
    return c;
}

// 수신 전용 스레드
void receive_msg() {
    char buffer[BUF_SIZE];
    while (true) {
        int str_len = read(sock, buffer, BUF_SIZE - 1);
        if (str_len <= 0) {
            // 연결 끊김 시 터미널 복구 후 종료
            reset_terminal_mode();
            cout << "\n[!] 서버와 연결이 끊어졌습니다." << endl;
            exit(0);
        }
        buffer[str_len] = 0;

        // 화면 갱신
        input_mtx.lock();
        
        // 현재 줄을 지움 (\r:맨앞으로, \033[K:커서 뒤로 싹 지움)
        printf("\r\033[K"); 
        
        // 받은 메시지 출력
        printf("%s\n", buffer);
        
        // 내가 치고 있던 글자 다시 복구해서 보여줌
        printf("[Me]: %s", current_input.c_str());
        fflush(stdout); // 화면 즉시 반영
        
        input_mtx.unlock();
    }
}

int main(int argc, char *argv[]) {
    // SIGPIPE 시그널 무시 (강제 종료 및 세그폴트 방지)
    signal(SIGPIPE, SIG_IGN);

    if (argc != 2) {
        cout << "사용법: " << argv[0] << " <닉네임>" << endl;
        return 1;
    }

    sprintf(name, "[%s]", argv[1]);

    struct sockaddr_in serv_addr;
    char name_msg[BUF_SIZE + BUF_SIZE]; // 이름과 메시지를 합칠 공간

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) { cerr << "Socket error" << endl; return 1; }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    
    // IP 상황에 맞게 수정 (defs.h의 VIDEO_SERVER_IP 사용)
    serv_addr.sin_addr.s_addr = inet_addr(VIDEO_SERVER_IP); 
    serv_addr.sin_port = htons(PORT_NUM);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        cerr << "Connect error! (IP: " << VIDEO_SERVER_IP << " 확인 필요)" << endl; 
        return 1;
    }

    // 접속하자마자 닉네임을 먼저 전송
    write(sock, argv[1], strlen(argv[1]));

    // 영상 송출 스레드 시작
    thread vid_t(send_video_thread, string(argv[1]));
    vid_t.detach();

    // 터미널 모드 변경 (getch로 직접 처리)
    set_conio_terminal_mode();

    cout << "=== 라즈베리파이 콘솔 채팅 (닉네임: " << argv[1] << ") ===" << endl;
    cout << ">> 종료하려면 '/q' 를 입력하세요." << endl;
    cout << ">> 웹 영상 접속: http://" << VIDEO_SERVER_IP << ":" << VIDEO_PORT << "/" << "닉네임" << endl;

    thread t(receive_msg);
    t.detach();

    // 초기 프롬프트 출력
    printf("[Me]: ");
    fflush(stdout);

    while (true) {
        int c = getch(); // 키 하나 입력 대기

        input_mtx.lock(); // 화면 꼬임 방지 잠금

        if (c == '\n') { // [엔터 키] 전송
            printf("\n");
            if (current_input.length() > 0) {
                // 종료 커맨드를 "/q"로 변경하여 실수 방지
                if (current_input == "/q") { 
                    input_mtx.unlock();
                    
                    // [Segfault 해결]                    
                    cout << "\n[!] 종료 중입니다... 잠시만 기다려주세요." << endl;
                    is_running = false; // 스레드들에게 멈추라고 신호 보냄
                    usleep(500000);     // 스레드가 루프를 빠져나올 시간(0.5초)을 줌
                    reset_terminal_mode();
                    cout << "\n[!] 채팅을 종료합니다." << endl;
                    close(sock); // 채팅 소켓 닫기
                    exit(0); // 안전하게 종료
                }

                // 메시지 전송
                sprintf(name_msg, "%s %s", name, current_input.c_str());
                
                // MSG_NOSIGNAL 추가
                send(sock, name_msg, strlen(name_msg), MSG_NOSIGNAL);
                 
                // 서버가 메세지 반송 (디버그용)
                // printf("%s %s\n", name, current_input.c_str());

                current_input = ""; // 입력 버퍼 초기화
            }
            printf("[Me]: "); // 새 입력창 띄우기
        } 
        else if (c == 127 || c == 8) { // [백스페이스] (Mac/Linux 차이 고려)
            if (current_input.length() > 0) {
                current_input.pop_back(); // 문자열에서 삭제
                printf("\b \b"); // 화면에서 삭제 (커서뒤로->공백->커서뒤로)
            }
        } 
        else { // [일반 글자]
            current_input += (char)c;
            putchar(c); // 화면에 글자 찍기
        }
        
        fflush(stdout); // 즉시 출력
        input_mtx.unlock(); // 잠금 해제
    }

    // 여기까지 도달하지 않음 (exit(0)으로 종료)
    close(sock);
    reset_terminal_mode(); 
    return 0;
}