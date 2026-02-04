# 컴파일러 설정
CXX = g++

# OpenCV 설정 (pkg-config 사용)
OPENCV_FLAGS = $(shell pkg-config --cflags --libs opencv4)

# CXXFLAGS: 공통 플래그 + 스레드 지원
CXXFLAGS = -I./common -pthread 

# 기본 타겟: 서버와 콘솔 클라이언트만 빌드 (Qt 제외)
all: server_build console_client_build

# 서버 빌드 (OpenCV 라이브러리 추가 - 영상을 저장하고 관리하는 로직엔 필요 없지만 map등 자료구조 사용에 유용)
server_build:
	$(CXX) -o chat_server server/main.cpp $(CXXFLAGS) $(OPENCV_FLAGS)

# 콘솔 클라이언트 빌드
console_client_build:
	$(CXX) -o chat_client_console client/main_console.cpp $(CXXFLAGS) $(OPENCV_FLAGS)

clean:
	rm -f chat_server chat_client_console