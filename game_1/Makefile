# 定義變數
CXX = g++
CXXFLAGS = -Wall -std=c++11 -O3
LDFLAGS = -lws2_32
TARGET = sample/Sample.exe
SRCS = Sample.cpp

# 預設目標
all: $(TARGET)

# 編譯規則
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 清理規則
clean:
	del $(TARGET)