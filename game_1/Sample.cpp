
#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <utility>
#include <math.h>
#include <algorithm>

#define MAXGRID 12

float exponentDFSArea = 1.15

// 8個方向 or 4個方向
int8_t dx[] = {0, -1, 0, 1, -1, 0, 1, -1, 0, 1};
int8_t dy[] = {0, 1, 1, 1, 0, 0, 0, -1, -1, -1};
int8_t dxx[] = {-1, 0, 1, 0};
int8_t dyy[] = {0, 1, 0, -1};

inline bool isPositionValid(int8_t x, int8_t y) { return x >= 0 && x < MAXGRID && y >= 0 && y < MAXGRID; }
inline bool isPositionValidForOccupying(int8_t x, int8_t y, int8_t playerID) { return isPositionValid(x, y) && mapState[x][y] == 0; }
// 移動方式
struct Move{
	int8_t x;
	int8_t y;
	int8_t subSheepNumber;
	int8_t direction;
	int8_t displacement;
};

typedef struct Move Move;
typedef std::pair<int8_t, int8_t> sheepBlock; 

// 版面資訊：棋盤狀態、羊群分布狀態、玩家ID、我的羊群位置（Stack）
class GameState{
	private:
		int8_t playerID;
		int8_t mapState[MAXGRID][MAXGRID];
		int8_t sheepState[MAXGRID][MAXGRID];
		vector<int8_t> mySheepBlocks;
	public:
		GameState(int8_t playerID, int8_t mapState[MAXGRID][MAXGRID], int8_t sheepState[MAXGRID][MAXGRID], sheepBlock& initSheepBlock){
			this->playerID = playerID;
			memcpy(this->mapState, mapState, sizeof(mapState));
			memcpy(this->sheepState, sheepState, sizeof(sheepState));
			this->mySheepBlocks.emplace_back(initSheepBlock);
			mapState[initSheepBlock.first][initSheepBlock.second] = playerID;
			sheepState[initSheepBlock.first][initSheepBlock.second] = 16;
		}
		inline int8_t getPlayerID() { return this->playerID; }
		inline int8_t (*getMapState())[MAXGRID] { return this->mapState; }
		inline int8_t (*getSheepState())[MAXGRID] { return this->sheepState; }
		std::vector<Move> getWhereToMoves();
		int8_t getSheepNumberToDivide(int8_t x, int8_t y);
		float calculateArea(int8_t x, int8_t y);
		int dfs(int8_t x, int8_t y, vector<vector<bool>>& visited);
		void applyMove(const Move& move, const GameState& state);
}

/* 找可以走的地方 
	1. 找出我有羊群的位置（>1）
	2. 找出所有羊群各自可以走的地方
	3. 分隔羊群
	4. 儲存分隔後的羊群可以走的地方 
*/

std::vector<Move> GameState::getWhereToMoves(){
	std::vector<Move> moves;
	for(auto& sheepBlock : this->mySheepBlocks){

		// 找出我有羊群的位置
		int8_t x = sheepBlock.first, y = sheepBlock.second;
		int8_t sheepNumber = this->sheepState[x][y];
		if(sheepNumber <= 1) continue;

		// 找出所有羊群各自可以走到底的地方（不能是自己）
		for(int8_t direction = 1; direction <= 9; ++direction){
			if(direction == 5) continue;
			int8_t xMove = x
			int8_t yMove = y;
			while(isPositionValidForOccupying(xMove, yMove)) {
				xMove += dx[direction]; 
				yMove += dy[direction];
			}
			if(xMove == x && yMove == y) continue;
			int8_t subSheepNumber = this->getSheepNumberToDivide(xMove, yMove, x, y);
			moves.emplace_back(Move{x, y, subSheepNumber, direction, abs(xMove - x)});
		}
	}
	return moves;
}


// 這裡的函數算法會依據地圖特性為羊群進行分割
// 使用 DFS 兩邊的連通面積得分 再根據得分比例決定要分割多少羊群
int8_t GameState::getSheepNumberToDivide(int8_t xMove, int8_t yMove, int8_t x, int8_t y){
	int8_t sheepNumber = this->sheepState[x][y];
	float areaMove = this->calculateArea(xMove, yMove);
	float area = this->calculateArea(x, y);
	int sheepNumberToDivide = max(int(sheepNumber * (areaMove / (areaMove + area))), 1);
	return sheepNumberToDivide;			
}

// 加總連通面積1.25次方
float GameState::calculateArea(int8_t x, int8_t y){
	vector<vector<bool>> visited(3, vector<bool>(3, false));
	float totalArea = 0;
	for(int i = 1 ; i <= 9 ; ++i){
		float area = 0;
		int8_t xMove = x + dx[i];
		int8_t yMove = y + dy[i];
		if(isPositionValidForOccupying(xMove, yMove)) area = pow(this->dfs(xMove, yMove, visited), exponentDFSArea);
		totalArea += area;
	}
	return totalArea;
}

// DFS 算連通面積
int GameState::dfs(int8_t x, int8_t y, vector<vector<bool>>& visited){
	if(visited[x][y]) return 0;
	visited[x][y] = true;
	int area = 1;
	for(int i = 1 ; i <= 4 ; ++i){
		int8_t xMove = x + dxx[i];
		int8_t yMove = y + dyy[i];
		if(isPositionValidForOccupying(xMove, yMove)) area += this->dfs(xMove, yMove, visited);
	}
	return area;
}

GameState GameState::applyMove(const Move& move, const GameState& state){
	GameState newState = state;
	int8_t x = move.x, y = move.y;
	int8_t xMove = x + dx[move.direction] * move.displacement;
	int8_t yMove = y + dy[move.direction] * move.displacement;
	newState.mapState[xMove][yMove] = this->playerID;
	newState.sheepState[x][y] -= move.subSheepNumber;
	newState.sheepState[xMove][yMove] = move.subSheepNumber;
	newState.mySheepBlocks.emplace_back(xMove, yMove);
	return newState;
}


/*
    選擇起始位置
    選擇範圍僅限場地邊緣(至少一個方向為牆)
    
    return: init_pos
    init_pos=<x,y>,代表你要選擇的起始位置
    
*/
std::vector<int> InitPos(int mapStat[MAXGRID][MAXGRID])
{
	std::vector<int> init_pos;
	init_pos.resize(2);

	/*
		Write your code here
	*/
    
    
    return init_pos;
}

/*
	產出指令
    
    input: 
	playerID: 你在此局遊戲中的角色(1~4)
    mapStat : 棋盤狀態, 為 12*12矩陣, 
					0=可移動區域, -1=障礙, 1~4為玩家1~4佔領區域
    sheepStat : 羊群分布狀態, 範圍在0~16, 為 12*12矩陣

    return Step
    Step : <x,y,m,dir> 
            x, y 表示要進行動作的座標 
            m = 要切割成第二群的羊群數量
            dir = 移動方向(1~9),對應方向如下圖所示
            1 2 3
			4 X 6
			7 8 9
*/
std::vector<int> GetStep(int playerID, int mapStat[MAXGRID][MAXGRID], int sheepStat[MAXGRID][MAXGRID])
{

	std::vector<int> step;
	step.resize(4);

	/*
		Write your code here
	*/
    
    return step;
}

int main()
{
	int id_package;
	int playerID;
    int mapStat[12][12];
    int sheepStat[12][12];

	// player initial
	GetMap(id_package,playerID,mapStat);
	std::vector<int> init_pos = InitPos(mapStat);
	SendInitPos(id_package,init_pos);

	while (true)
	{
		if (GetBoard(id_package, mapStat, sheepStat))
			break;

		std::vector<int> step = GetStep(playerID,mapStat,sheepStat);
		SendStep(id_package, step);
	}
}
