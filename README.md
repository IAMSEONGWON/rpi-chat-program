# Raspberry Pi TCP Chat Program (Console & Qt GUI)

라즈베리파이(Linux)와 윈도우(Windows) 환경을 모두 지원하는 멀티 스레드 기반의 TCP 채팅 프로그램입니다.
서버는 C++ 표준 소켓 라이브러리를 사용하였으며, 클라이언트는 **CLI(Console)** 버전과 **GUI(Qt)** 버전 두 가지를 제공합니다.

## 주요 기능
- **멀티 유저 채팅:** 다수의 클라이언트가 동시에 접속하여 대화 가능 (스레드 처리)
- **크로스 플랫폼 지원:**
  - **Console Client:** 라즈베리파이/리눅스 터미널 환경 최적화 (ANSI Code 사용)
  - **Qt GUI Client:** 윈도우/리눅스 데스크탑 환경 지원 (Qt Widgets 기반)
- **서버 알림:** 입장/퇴장 및 공지 사항을 서버가 클라이언트에게 전송
- **동기화:** Mutex를 사용한 임계 영역 보호 (Thread-safe)
- **UI 최적화:** 메시지 수신 시 입력창 깨짐 방지 및 닉네임 표시 기능

## 디렉토리 구조
```text
.
├── common/         # 공통 헤더 (포트 번호, 버퍼 크기 등)
├── server/         # 채팅 서버 소스 코드 (C++ Standard)
├── client/         # 콘솔용 클라이언트 소스 코드 (Linux/RPi)
├── client_qt/      # [NEW] GUI 클라이언트 소스 코드 (Qt Framework)
├── Makefile        # 통합 빌드 스크립트 (Server & Console Client)
└── README.md       # 문서
```

## 빌드 및 실행 방법

### 1. 서버 및 콘솔 클라이언트 (Raspberry Pi / Linux)
프로젝트 루트에서 `make` 명령어를 실행하여 빌드합니다.

```bash
# 빌드
make

# 서버 실행 (백그라운드 모드)
./chat_server &

# 콘솔 클라이언트 실행
./chat_client_console <닉네임>
# 예시: ./chat_client_console yun
```

### 2. GUI 클라이언트 (Windows / PC)
Qt Creator를 사용하여 빌드 및 실행합니다.

1. **Qt Creator** 실행 후 `Open Project` 클릭.
2. `client_qt/chat-client.pro` 파일을 선택하여 열기.
3. **Configure Project** 단계에서 기본 Desktop Kit 선택.
4. 소스 코드(`mainwindow.cpp`)에서 서버 IP 주소를 라즈베리파이의 IP로 수정.
   ```cpp
   socket->connectToHost("192.168.x.x", 12345);
   ```
5. 좌측 하단의 **Run (초록색 재생 버튼)** 클릭하여 실행.
6. 프로그램 실행 후 `닉네임` 입력 및 `연결` 버튼 클릭.

## 개발 환경
- **OS:** Raspberry Pi OS (Linux), Windows 10/11
- **Language:** C++11 or higher
- **Framework:** Qt 5 / Qt 6 (Network, Widgets Module)
- **Libraries:** pthread (POSIX Threads)
- **Compiler:** g++, MSVC/MinGW (for Qt)

## 추후 계획 (To-Do)
- [x] Qt 프레임워크를 이용한 GUI 클라이언트 연동 (완료)
- [ ] 라즈베리파이 카메라 영상 송수신 기능 통합