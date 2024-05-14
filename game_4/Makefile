# 定義變數
CXX = g++
CXXFLAGS = -Wall -std=c++17 -O3
LDFLAGS = -static -lws2_32
TARGET = sample/team4_agent4.exe
SRCS = team4_agent4.cpp

# 預設目標
all: $(TARGET)

# 編譯規則
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 清理規則
clean:
	del $(TARGET)