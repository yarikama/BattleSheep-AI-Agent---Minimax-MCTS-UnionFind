# 定義變數
CXX = g++
CXXFLAGS = -Wall -std=c++17 -O3
LDFLAGS = -lws2_32
TARGET = sample/team4_agent1.exe
SRCS = team4_agent1.cpp

# 預設目標
all: $(TARGET)

# 編譯規則
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 清理規則
clean:
	del $(TARGET)