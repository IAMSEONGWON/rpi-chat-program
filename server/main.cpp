#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
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
}

void handle_clnt(int clnt_sock) {
    int str_len = 0;
    char msg[BUF_SIZE];
    char notice[BUF_SIZE]; // 공지용 버퍼

    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) {
        send_msg(msg, str_len, clnt_sock);
    }

    // --- 접속 종료 처리 ---
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

    // 나갔다는 알림 전송 (sender_sock에 -1을 넣어 모두에게 전송)
    sprintf(notice, "[Server] A user disconnected. (Total: %d)", clnt_cnt);
    send_msg(notice, strlen(notice), -1); // -1 = 전체 공지
    
    close(clnt_sock);
}

int main() {
    // 서버 자체 터미널 로그는 끔 (화면 꼬임 방지)
    freopen("/dev/null", "w", stdout);

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    char notice[BUF_SIZE]; // 공지용 버퍼

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

        // 들어왔다는 알림 전송 (-1 = 전체 공지)
        sprintf(notice, "[Server] New user connected: %s (Total: %d)", 
                inet_ntoa(clnt_adr.sin_addr), clnt_cnt);
        
        // 새로 들어온 사람을 포함한 모든 사람에게 알림을 줌
        // 스레드 생성 전에 보내야함
        send_msg(notice, strlen(notice), -1);
        thread t(handle_clnt, clnt_sock);
        t.detach();
    }

    close(serv_sock);
    return 0;
}