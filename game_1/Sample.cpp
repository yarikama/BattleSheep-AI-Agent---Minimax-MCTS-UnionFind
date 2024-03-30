
#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <utility>
#include <math.h>
#include <algorithm>
#include <limits.h>

#define MAXGRID 12
#define powSurroundLen 1.5
#define weightSurroundLen 1
#define weightEmpty 2.5
#define exponentDFSArea 1.15
#define exponentEvaluate 1.25

// 8個方向 
int8_t dx[] = {0, -1, 0, 1, -1, 0, 1, -1, 0, 1};
int8_t dy[] = {0, 1, 1, 1, 0, 0, 0, -1, -1, -1};

// 4個方向
int8_t dxx[] = {-1, 0, 1, 0};
int8_t dyy[] = {0, 1, 0, -1};

inline bool isPositionValid(int8_t x, int8_t y) { return x >= 0 && x < MAXGRID && y >= 0 && y < MAXGRID; }
template<typename T>
inline bool isPositionValidForOccupying(int8_t x, int8_t y, T mapState[MAXGRID][MAXGRID]) {return isPositionValid(x, y) && mapState[x][y] == 0; }
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
		std::vector<sheepBlock> mySheepBlocks;
	public:
		GameState(int8_t playerID, int8_t mapState[MAXGRID][MAXGRID], int8_t sheepState[MAXGRID][MAXGRID], sheepBlock& initSheepBlock){
			this->playerID = playerID;
			std::copy(&mapState[0][0], &mapState[0][0] + MAXGRID * MAXGRID, &this->mapState[0][0]);
			std::copy(&sheepState[0][0], &sheepState[0][0] + MAXGRID * MAXGRID, &this->sheepState[0][0]);
			this->mySheepBlocks.emplace_back(initSheepBlock);
			mapState[initSheepBlock.first][initSheepBlock.second] = playerID;
			sheepState[initSheepBlock.first][initSheepBlock.second] = 16;
		}
		inline int8_t getPlayerID() { return this->playerID; }
		inline int8_t (*getMapState())[MAXGRID] { return this->mapState; }
		inline int8_t (*getSheepState())[MAXGRID] { return this->sheepState; }
		inline std::vector<sheepBlock> getMySheepBlocks() { return this->mySheepBlocks; }
		std::vector<Move> getWhereToMoves();
		int8_t getSheepNumberToDivide(int8_t xMove, int8_t yMove, int8_t x, int8_t y);
		float calculateArea(int8_t x, int8_t y);
		int dfs(int8_t x, int8_t y, std::vector<std::vector<bool>>& visited, int8_t anyPlayerID);
		GameState applyMove(const Move& move, const GameState& state);
		int Minimax(int depth, int alpha, int beta, int playerID);
		int evaluate();
};

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
			int8_t xMove = x;
			int8_t yMove = y;
			while(isPositionValidForOccupying(xMove, yMove, this->mapState)) {
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
	int sheepNumberToDivide = std::max(int(sheepNumber * (areaMove / (areaMove + area))), 1);
	return sheepNumberToDivide;			
}

// 加總連通面積1.25次方
float GameState::calculateArea(int8_t x, int8_t y){
	std::vector<std::vector<bool>> visited(3, std::vector<bool>(3, false));
	float totalArea = 0;
	for(int i = 1 ; i <= 9 ; ++i){
		float area = 0;
		int8_t xMove = x + dx[i];
		int8_t yMove = y + dy[i];
		if(isPositionValidForOccupying(xMove, yMove, this->mapState)) area = pow(this->dfs(xMove, yMove, visited, this->playerID), exponentDFSArea);
		totalArea += area;
	}
	return totalArea;
}

// DFS 算連通面積，不可以走過，並且要是自己地盤或是空地
int GameState::dfs(int8_t x, int8_t y, std::vector<std::vector<bool>>& visited, int8_t anyPlayerID){
	if(visited[x][y] or !(this->mapState[x][y] == 0 or this->mapState[x][y] == anyPlayerID)) return 0;
	visited[x][y] = true;
	int area = 1;
	for(int i = 1 ; i <= 4 ; ++i){
		int8_t xMove = x + dxx[i];
		int8_t yMove = y + dyy[i];
		if(isPositionValidForOccupying(xMove, yMove, this->mapState)) area += this->dfs(xMove, yMove, visited, anyPlayerID);
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

int GameState::Minimax(int depth, int alpha, int beta, int playerID){
	if(depth == 0) return this->evaluate();

	if(playerID == this->playerID){
		int maxEvaluation = INT_MIN;
		for(auto& move : this->getWhereToMoves()){
			GameState newState = this->applyMove(move, *this);
			int evaluation = newState.Minimax(depth - 1, alpha, beta, (playerID % 4) + 1);
			maxEvaluation = std::max(maxEvaluation, evaluation);
			alpha = std::max(alpha, evaluation);
			if(beta <= alpha) break;
		}
		return maxEvaluation;
	}else{
		int minEvaluation = INT_MAX;
		for(auto& move : this->getWhereToMoves()){
			GameState newState = this->applyMove(move, *this);
			int evaluation = newState.Minimax(depth - 1, alpha, beta, (playerID % 4) + 1);
			minEvaluation = std::min(minEvaluation, evaluation);
			beta = std::min(beta, evaluation);
			if(beta <= alpha) break;
		}
		return minEvaluation;
	}
}

// 計算面積起始點要是某個玩家的地盤，並且沒有走過
int GameState::evaluate(){
	std::vector<std::vector<bool>> visited(MAXGRID, std::vector<bool>(MAXGRID, false));
	std::vector<float> playerArea(5, 0);
	playerArea[0] = -1;
	for(int i = 0 ; i < MAXGRID ; ++i){
		for(int j = 0 ; j < MAXGRID ; ++j){
			int8_t anyPlayerID = this->mapState[i][j];
			if(visited[i][j] or anyPlayerID == 0 or anyPlayerID == -1) continue;
			float area = pow(this->dfs(i, j, visited, anyPlayerID), exponentEvaluate);
			playerArea[anyPlayerID] += area;
		}
	}
	int rank = 4;
	for(int i = 1 ; i <= 4 ; ++i) if(playerArea[i] < playerArea[this->playerID]) --rank;
	return -1 * rank;
}
/*
    選擇起始位置
    選擇範圍僅限場地邊緣(至少一個方向為牆)
    
    return: init_pos
    init_pos=<x,y>,代表你要選擇的起始位置
    
*/

void printMap(int mapStat[MAXGRID][MAXGRID]){
	printf("my gamestate\n");
	for(int y = 0; y < MAXGRID; y++){
		for(int x = 0; x < MAXGRID; x++){
			printf("%d ", mapStat[x][y]);
		}
		printf("\n\n");
	}
}

std::vector<int> InitPos(int mapStat[MAXGRID][MAXGRID])
{
	printMap(mapStat);

	std::vector<int> init_pos;
	init_pos.resize(2);
	int maxScore = 0;
	for(int y = 0; y < MAXGRID; y++){
		for(int x = 0; x < MAXGRID; x++){

			//檢查是否為障礙或其他player
			if(mapStat[x][y] != 0) continue;

			//檢查是否為場地邊緣
			bool isEdge = false;
			for(int i = 0 ; i < 4 ; i++){
				int8_t newX = x + dxx[i], newY = y + dyy[i];
				if(!isPositionValid(newX, newY)) continue;
				if(mapStat[newX][newY] == -1) isEdge = true;
			}

			//周圍空格數、周圍延伸距離
			int numEmpty = 0;
			double numSurround = 0;
			for(int i = 1 ; i <= 9 ; i++){
				if(i == 5) continue;
				int newX = x + dx[i], newY = y + dy[i];
				if(!isPositionValidForOccupying(newX, newY, mapStat)) continue;
				numEmpty++;
				//周圍延伸距離
				int lenSurround = 0;
				while(isPositionValidForOccupying(newX, newY, mapStat)){
					lenSurround++;
					newX += dx[i];
					newY += dy[i];
				}
				numSurround += pow(lenSurround, powSurroundLen);
			}

			//限定場地邊緣
			if(isEdge == false) continue;

			//目標周圍空格多、周圍延伸距離短
			int score = weightEmpty * numEmpty - weightSurroundLen * numSurround / 8;
			if(score > maxScore){
				maxScore = score;
				init_pos[0] = x;
				init_pos[1] = y;
			}
		}
	}    
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
	GetMap(id_package, playerID, mapStat);
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
