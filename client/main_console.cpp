#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../common/defs.h"

using namespace std;

char name[BUF_SIZE]; // 닉네임 저장할 변수

// 수신 전용 스레드
void receive_msg(int sock) {
    char buffer[BUF_SIZE];
    while (true) {
        int str_len = read(sock, buffer, BUF_SIZE - 1);
        if (str_len <= 0) {
            cout << "\n[!] 서버와 연결이 끊어졌습니다." << endl;
            exit(0);
        }
        buffer[str_len] = 0;

        // 커서 제어 
        // \033[2K : 현재 커서가 있는 줄(Line)을 깨끗하게 지움
        // \r      : 커서를 줄의 맨 앞(왼쪽 끝)으로 이동
        cout << "\033[2K\r" << buffer << endl;

        // 메시지 출력 후 다시 입력 프롬프트([Me]:)를 띄움
        cout << "[Me]: " << flush; 
    }
}

int main(int argc, char *argv[]) {
    // 실행할 때 닉네임을 입력했는지 확인
    if (argc != 2) {
        cout << "사용법: " << argv[0] << " <닉네임>" << endl;
        return 1;
    }

    // 닉네임 저장
    sprintf(name, "[%s]", argv[1]);

    int sock;
    struct sockaddr_in serv_addr;
    char msg[BUF_SIZE];
    char name_msg[BUF_SIZE + BUF_SIZE]; // 이름과 메시지를 합칠 공간

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) { cerr << "Socket error" << endl; return 1; }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    // IP 상황에 맞게 수정
    serv_addr.sin_addr.s_addr = inet_addr("192.168.0.20"); 
    serv_addr.sin_port = htons(PORT_NUM);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        cerr << "Connect error!" << endl; return 1;
    }

    cout << "=== 라즈베리파이 콘솔 채팅 (닉네임: " << argv[1] << ") ===" << endl;

    thread t(receive_msg, sock);
    t.detach();

    while (true) {
        cout << "[Me]: " << flush;
        cin.getline(msg, BUF_SIZE);

        if (!strcmp(msg, "q")) {
            close(sock);
            break;
        }

        // 보낼 때 "닉네임 + 메시지"를 합쳐서 전송
        sprintf(name_msg, "%s %s", name, msg);
        write(sock, name_msg, strlen(name_msg));
    }

    return 0;
}