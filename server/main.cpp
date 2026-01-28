#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 5100
#define BUFFER_SIZE 1024

using namespace std;

// 클라이언트 처리를 위한 스레드 함수
void* handle_client(void* arg) {
    int client_sock = *((int*)arg);
    delete (int*)arg; // 동적 할당된 메모리 해제

    char buffer[BUFFER_SIZE];
    int str_len;

    cout << "[INFO] Client connected (Socket: " << client_sock << ")" << endl;

    // 데이터 수신 루프 (연결이 끊길 때까지 반복)
    while ((str_len = read(client_sock, buffer, BUFFER_SIZE)) > 0) {
        buffer[str_len] = 0; // 문자열 끝 처리
        cout << "[RECV] " << buffer;
        
        // 에코: 받은 데이터를 그대로 다시 전송
        write(client_sock, buffer, str_len);
    }

    // 연결 종료 처리
    close(client_sock);
    cout << "[INFO] Client disconnected (Socket: " << client_sock << ")" << endl;
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pthread_t t_id;

    // 소켓 생성
    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        cerr << "Socket creation failed" << endl;
        return -1;
    }

    // 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // bind (포트 할당) - 주소 재사용 옵션 추가 (SO_REUSEADDR)
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "Bind error" << endl;
        return -1;
    }

    // listen (대기열 생성)
    if (listen(server_sock, 5) == -1) {
        cerr << "Listen error" << endl;
        return -1;
    }

    cout << "=== Chat Server Started on Port " << PORT << " ===" << endl;

    // accept Loop (무한 루프로 클라이언트 대기)
    while (1) {
        client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);

        if (client_sock == -1) continue;

        // 스레드에 소켓 정보를 넘겨주기 위해 메모리 동적 할당
        int* new_sock = new int;
        *new_sock = client_sock;

        // 새로운 클라이언트가 올 때마다 스레드 생성
        pthread_create(&t_id, NULL, handle_client, (void*)new_sock);
        pthread_detach(t_id); // 스레드가 끝나면 리소스 자동 회수
    }

    close(server_sock);
    return 0;
}