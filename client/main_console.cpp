#include <iostream>
#include <thread>
#include <string>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <termios.h> // 터미널 제어용 헤더
#include "../common/defs.h"

using namespace std;

// 전역 변수 
int sock;
string current_input = ""; // 사용자가 현재 치고 있는 내용을 저장
mutex input_mtx;           // 화면 출력 충돌 방지용 뮤텍스
char name[BUF_SIZE];       // 닉네임 저장할 변수

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
    // IP 상황에 맞게 수정
    serv_addr.sin_addr.s_addr = inet_addr("192.168.0.30"); 
    serv_addr.sin_port = htons(PORT_NUM);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        cerr << "Connect error!" << endl; return 1;
    }

    // 접속하자마자 닉네임을 먼저 전송
    // argv[1]은 실행할 때 넣은 닉네임 (예: "yun")
    write(sock, argv[1], strlen(argv[1]));

    // 터미널 모드 변경 (getch로 직접 처리)
    set_conio_terminal_mode();

    cout << "=== 라즈베리파이 콘솔 채팅 (닉네임: " << argv[1] << ") ===" << endl;

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
                // 종료 커맨드 'q' 확인
                if (current_input == "q") {
                    input_mtx.unlock();
                    break; 
                }

                // 메시지 전송
                sprintf(name_msg, "%s %s", name, current_input.c_str());
                write(sock, name_msg, strlen(name_msg));
                 
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

    close(sock);
    reset_terminal_mode(); // 터미널 복구
    return 0;
}