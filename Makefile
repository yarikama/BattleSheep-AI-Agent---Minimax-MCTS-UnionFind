# 定義變量
CXX = g++
CXXFLAGS = -Wall -std=c++17 -O3
LDFLAGS = -static -lws2_32

# 定義目標文件列表
TARGETS = game_1/team4_agent1.exe game_2/team4_agent2.exe game_3/team4_agent3.exe game_4/team4_agent4.exe

# 預設目標
all: $(TARGETS)

# 編譯規則
game_%/team4_agent%.exe: game_%/team4_agent%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

# 清理規則
clean:
	del $(TARGETS)

