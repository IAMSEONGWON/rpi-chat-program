# 컴파일러 설정
CXX = g++
CXXFLAGS = -I./common -pthread 

# 기본 타겟: 서버와 콘솔 클라이언트만 빌드 (Qt 제외)
all: server_build console_client_build

# 1. 서버 빌드
server_build:
	$(CXX) -o chat_server server/main.cpp $(CXXFLAGS)

# 2. 콘솔 클라이언트 빌드 (SSH용)
console_client_build:
	$(CXX) -o chat_client_console client/main_console.cpp $(CXXFLAGS)

# 3. Qt 클라이언트 빌드 (PC에서나 필요할 때 명시적으로 호출)
qt_client_build:
	cd client && qmake client.pro && make
	mv client/chat_client_app ./chat_client_gui

clean:
	rm -f chat_server chat_client_console chat_client_gui
	cd client && make clean || true