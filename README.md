# Raspberry Pi TCP Chat Program

라즈베리파이와 리눅스 환경을 위한 멀티 스레드 기반의 TCP 콘솔 채팅 프로그램입니다.
C++ 표준 소켓 라이브러리를 사용하였으며, `server`와 `client`가 분리되어 있습니다.

## 주요 기능
- **멀티 유저 채팅:** 다수의 클라이언트가 동시에 접속하여 대화 가능 (스레드 처리)
- **콘솔 UI 최적화:** ANSI Escape Code를 사용하여 메시지 수신 시 입력창(`[Me]:`) 깨짐 방지
- **서버 알림:** 입장/퇴장 및 공지 사항을 서버가 클라이언트에게 전송
- **동기화:** Mutex를 사용한 임계 영역 보호 (Thread-safe)
- **크로스 플랫폼 구조:** Qt GUI 확장을 고려한 디렉토리 구조 설계

## 디렉토리 구조
```text
.
├── common/         # 공통 헤더 (포트 번호, 버퍼 크기 등)
├── server/         # 채팅 서버 소스 코드 (C++ Standard)
├── client/         # 채팅 클라이언트 소스 코드 (Console & Qt)
├── Makefile        # 통합 빌드 스크립트
└── README.md       # 문서
```

## 빌드 및 실행 방법

### 1. 빌드 (Build)
프로젝트 루트에서 `make` 명령어를 실행합니다.

```bash
make
```

### 2. 서버 실행 (Server)
서버를 백그라운드 모드로 실행합니다. (로그는 내부적으로 `/dev/null`로 처리됨)

```bash
./chat_server &
```

### 3. 클라이언트 실행 (Client)
닉네임을 인자로 주어 실행합니다.

```bash
./chat_client_console <닉네임>
# 예시: ./chat_client_console yun
```

## 개발 환경
- **OS:** Raspberry Pi OS (Linux)
- **Language:** C++11 or higher
- **Libraries:** pthread (POSIX Threads)
- **Compiler:** g++

## 추후 계획 (To-Do)
- [ ] Qt 프레임워크를 이용한 GUI 클라이언트 연동
- [ ] 라즈베리파이 카메라 영상 송수신 기능 통합