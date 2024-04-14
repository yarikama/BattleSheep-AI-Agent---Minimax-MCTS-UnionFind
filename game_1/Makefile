# # 定義變數
# CXX = g++
# CXXFLAGS = -Wall -std=c++17 -O3
# LDFLAGS = -lws2_32
# TARGET = sample/Sample.exe
# SRCS = Sample.cpp

# # 預設目標
# all: $(TARGET)

# # 編譯規則
# $(TARGET): $(SRCS)
# 	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# # 清理規則
# clean:
# 	del $(TARGET)


# For parameter tuning
CXX = g++
CXXFLAGS = -Wall -std=c++17 -O3
LDFLAGS = -lws2_32

PARAMS1 = -DexponentEvaluate=1.8 -DMCTSSIMULATIONS=90 -DMCTSDEPTH=9 -DminimaxDepth=2 -DTEAM_ID=1
PARAMS2 = -DexponentEvaluate=1.8 -DMCTSSIMULATIONS=90 -DMCTSDEPTH=9 -DminimaxDepth=2 -DTEAM_ID=2
PARAMS3 = -DexponentEvaluate=1.8 -DMCTSSIMULATIONS=90 -DMCTSDEPTH=9 -DminimaxDepth=2 -DTEAM_ID=3
PARAMS4 = -DexponentEvaluate=1.8 -DMCTSSIMULATIONS=90 -DMCTSDEPTH=9 -DminimaxDepth=2 -DTEAM_ID=4

TARGET1 = sample/Sample1.exe
TARGET2 = sample/Sample2.exe
TARGET3 = sample/Sample3.exe
TARGET4 = sample/Sample4.exe
SRCS = Sample.cpp

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

$(TARGET1): $(SRCS)
	$(CXX) $(CXXFLAGS) $(PARAMS1) $^ -o $@ $(LDFLAGS)

$(TARGET2): $(SRCS)
	$(CXX) $(CXXFLAGS) $(PARAMS2) $^ -o $@ $(LDFLAGS)

$(TARGET3): $(SRCS)
	$(CXX) $(CXXFLAGS) $(PARAMS3) $^ -o $@ $(LDFLAGS)

$(TARGET4): $(SRCS)
	$(CXX) $(CXXFLAGS) $(PARAMS4) $^ -o $@ $(LDFLAGS)

clean:
	del $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)