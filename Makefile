# 컴파일러 설정
CXX = g++
# 컴파일 옵션 (-Wall: 모든 경고 표시, -O2: 최적화)
CXXFLAGS = -Wall -O2
# 링크 라이브러리 (pthread)
LDFLAGS = -lpthread

# 실행 파일 이름
TARGET = chat_server
# 소스 파일 경로
SRCS = server/main.cpp

# make
all: $(TARGET)

# 빌드 규칙
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

# make clean
clean:
	rm -f $(TARGET)