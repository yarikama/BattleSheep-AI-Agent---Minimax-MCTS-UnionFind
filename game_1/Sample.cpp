
#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <utility>
#include <math.h>
#include <algorithm>
#include <limits.h>
#include <limits>

#define MAXGRID 12
#define powSurroundLen 1.5
#define weightSurroundLen 1
#define weightEmpty 3
#define exponentDFSArea 1.15
#define exponentEvaluate 3

// 8個方向 
int dx[] = {0, -1, 0, 1, -1, 0, 1, -1, 0, 1};
int dy[] = {0, -1, -1, -1, 0, 0, 0, 1, 1, 1};

// 4個方向
int dxx[] = {-1, 0, 1, 0};
int dyy[] = {0, 1, 0, -1};

FILE* outfile;

inline bool isPositionValid(int x, int y) { return x >= 0 && x < MAXGRID && y >= 0 && y < MAXGRID; }
template<typename T>
inline bool isPositionValidForOccupying(int x, int y, T mapState[MAXGRID][MAXGRID]) {return isPositionValid(x, y) && mapState[x][y] == 0; }
// 移動方式
struct Move{
	int x;
	int y;
	int subSheepNumber;
	int direction;
	int displacement;
};

typedef struct Move Move;
typedef std::pair<int, int> sheepBlock; 

void printMap(int mapStat[MAXGRID][MAXGRID]){
    fprintf(outfile, "   ");
    for(int x = 0; x < MAXGRID; x++) fprintf(outfile, "%d  ", x);
    fprintf(outfile, "\n");

    for(int y = 0; y < MAXGRID; y++){
        fprintf(outfile, "%d  ", y);
        for(int x = 0; x < MAXGRID; x++){
			if(mapStat[x][y] == -1) fprintf(outfile, "X  ");
			else fprintf(outfile, "%d  ", mapStat[x][y]);
        }
        fprintf(outfile, "\n");
    }
    fprintf(outfile, "\n");
}

void printMapAndSheep(int mapStat[MAXGRID][MAXGRID], int sheepStat[MAXGRID][MAXGRID]){
    // Print column headers
    fprintf(outfile, "y\\x ");
    for(int x = 0; x < MAXGRID; x++){
        fprintf(outfile, "  x=%-3d  ", x);    }
    fprintf(outfile, "\n");

    // Print separator line
    fprintf(outfile, "   +");
    for(int x = 0; x < MAXGRID; x++){
        fprintf(outfile, "--------+");
    }
    fprintf(outfile, "\n");

    // Print map and sheep data
    for(int y = 0; y < MAXGRID; y++){
        fprintf(outfile, "y=%-2d|", y);
        for(int x = 0; x < MAXGRID; x++){
            if(mapStat[x][y] == -1)
                fprintf(outfile, "   X    |");
            else
                fprintf(outfile, " %d, %-3d |", mapStat[x][y], sheepStat[x][y]);
        }
        fprintf(outfile, "\n");

        // Print separator line
        fprintf(outfile, "   +");
        for(int x = 0; x < MAXGRID; x++){
            fprintf(outfile, "--------+");
        }
        fprintf(outfile, "\n");
    }
    fprintf(outfile, "\n");
}

void printsb(std::vector<sheepBlock> sb){
	for(auto& block : sb){
		fprintf(outfile, "(%d %d), ", block.first, block.second);
	}
}

std::vector<int> InitPos(int mapStat[MAXGRID][MAXGRID])
{
	// printMap(mapStat);

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
				int newX = x + dxx[i], newY = y + dyy[i];
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


// 版面資訊：棋盤狀態、羊群分布狀態、玩家ID、我的羊群位置（Stack）
class GameState{
	private:
		int playerID;
		int mapState[MAXGRID][MAXGRID];
		int sheepState[MAXGRID][MAXGRID];
		std::vector<sheepBlock> mySheepBlocks;
	public:
		GameState(int playerID, int mapState[MAXGRID][MAXGRID], int sheepState[MAXGRID][MAXGRID], std::vector<sheepBlock> sheepBlocks){
			this->playerID = playerID;
			std::copy(&mapState[0][0], &mapState[0][0] + MAXGRID * MAXGRID, &this->mapState[0][0]);
			std::copy(&sheepState[0][0], &sheepState[0][0] + MAXGRID * MAXGRID, &this->sheepState[0][0]);
			std::copy(sheepBlocks.begin(), sheepBlocks.end(), std::back_inserter(this->mySheepBlocks));
		}

		inline int getPlayerID() { return this->playerID; }
		inline int (*getMapState())[MAXGRID] { return this->mapState; }
		inline int (*getSheepState())[MAXGRID] { return this->sheepState; }
		inline std::vector<sheepBlock> getMySheepBlocks() { return this->mySheepBlocks; }

		std::vector<Move> getWhereToMovesEveryPosiblity();
		std::vector<Move> getWhereToMoves();
		int getSheepNumberToDivide(int xMove, int yMove, int x, int y);
		float calculateArea(int x, int y);
		int dfs(int x, int y, std::vector<std::vector<bool>>& visited, int anyPlayerID, int originX, int originY, bool nineNine);

		GameState applyMove(Move move, GameState state);

		float minimax(int depth, float alpha, float beta, int playerID);
		float evaluate();
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
		int x = sheepBlock.first, y = sheepBlock.second;
		int sheepNumber = this->sheepState[x][y];
		if(sheepNumber <= 1) continue;

		// 找出所有羊群各自可以走到底的地方（不能是自己）
		for(int direction = 1; direction <= 9; ++direction){
			if(direction == 5) continue;
			int xMove = x;
			int yMove = y;
			while(isPositionValidForOccupying(xMove + dx[direction], yMove + dy[direction], this->mapState)) {
				xMove += dx[direction]; 
				yMove += dy[direction];
			}
			if(xMove == x && yMove == y) continue;
			int subSheepNumber = this->getSheepNumberToDivide(xMove, yMove, x, y);
			if(!moves.empty() and moves.back().x == xMove and moves.back().y == yMove){
				fprintf(outfile, "Duplicate move\n");
			}
			moves.emplace_back(Move{x, y, subSheepNumber, direction, std::max(abs(xMove - x), abs(yMove - y))});
		}
	}
	return moves;
}

std::vector<Move> GameState::getWhereToMovesEveryPosiblity(){
	std::vector<Move> moves;
	for(auto& sheepBlock : this->mySheepBlocks){
		// 找出我有羊群的位置
		int x = sheepBlock.first, y = sheepBlock.second;
		int sheepNumber = this->sheepState[x][y];
		if(sheepNumber <= 1) continue;

		// 找出所有羊群各自可以走到底的地方（不能是自己）
		for(int direction = 1; direction <= 9; ++direction){
			if(direction == 5) continue;
			int xMove = x;
			int yMove = y;
			while(isPositionValidForOccupying(xMove + dx[direction], yMove + dy[direction], this->mapState)) {
				xMove += dx[direction]; 
				yMove += dy[direction];
			}
			if(xMove == x && yMove == y) continue;
			int displacement = std::max(abs(xMove - x), abs(yMove - y));
			for(int subSheepNumber = 1; subSheepNumber < sheepNumber; ++subSheepNumber) moves.emplace_back(Move{x, y, subSheepNumber, direction, displacement});
		}
	}
	return moves;
}
// 這裡的函數算法會依據地圖特性為羊群進行分割
// 使用 DFS 兩邊的連通面積得分 再根據得分比例決定要分割多少羊群
int GameState::getSheepNumberToDivide(int xMove, int yMove, int x, int y){
	int sheepNumber = this->sheepState[x][y];
	float areaMove = this->calculateArea(xMove, yMove);
	float area = this->calculateArea(x, y);
	int sheepNumberToDivide = std::min(std::max(int(sheepNumber * (areaMove / (areaMove + area))), 1) , sheepNumber - 1);
	return sheepNumberToDivide;			
}

// 加總連通面積1.25次方
float GameState::calculateArea(int x, int y){
	std::vector<std::vector<bool>> visited(3, std::vector<bool>(3, false));
	float totalArea = 0;
	for(int i = 1 ; i <= 9 ; ++i){
		float area = 0;
		int xMove = x + dx[i];
		int yMove = y + dy[i];
		if(isPositionValidForOccupying(xMove, yMove, this->mapState)) area = pow(this->dfs(xMove, yMove, visited, this->playerID, xMove, yMove, 1), exponentDFSArea);
		totalArea += area;
	}
	return totalArea;
}

// DFS 算連通面積，不可以走過，並且要是自己地盤或是空地
int GameState::dfs(int x, int y, std::vector<std::vector<bool>>& visited, int anyPlayerID, int originX, int originY, bool nineNine){
	int relativeX = nineNine? x - originX + 1 : x, relativeY = nineNine? y - originY + 1 : y;
	if(nineNine and (relativeX < 0 or relativeX > 2 or relativeY < 0 or relativeY > 2)) return 0; 
	if(visited[relativeX][relativeY] or !(this->mapState[x][y] == 0 or this->mapState[x][y] == anyPlayerID)) return 0;
	visited[relativeX][relativeY] = true;
	int area = 1;
	for(int i = 1 ; i <= 4 ; ++i){
		int xMove = x + dxx[i];
		int yMove = y + dyy[i];
		if(isPositionValidForOccupying(xMove, yMove, this->mapState)) area += this->dfs(xMove, yMove, visited, anyPlayerID, originX, originY, nineNine);
	}
	return area;
}

GameState GameState::applyMove(Move move, GameState state){
	GameState newState(state.playerID, state.mapState, state.sheepState, state.mySheepBlocks);
	int x = move.x, y = move.y;
	int xMove = x + dx[move.direction] * move.displacement;
	int yMove = y + dy[move.direction] * move.displacement;
	newState.mapState[xMove][yMove] = this->playerID;
	newState.sheepState[x][y] -= move.subSheepNumber;
	newState.sheepState[xMove][yMove] = move.subSheepNumber;
	return newState;
}

float GameState::minimax(int depth, float alpha, float beta, int playerID){
	std::vector<Move> availableMoves = this->getWhereToMovesEveryPosiblity();
	if(depth == 0 or availableMoves.empty()) return this->evaluate();
	if(playerID == this->playerID){
        float maxEvaluation = std::numeric_limits<float>::min();
		for(auto& move : availableMoves){
			GameState newState = this->applyMove(move, *this);
			float evaluation = newState.minimax(depth - 1, alpha, beta, (playerID % 4) + 1);
			maxEvaluation = std::max(maxEvaluation, evaluation);
			alpha = std::max(alpha, evaluation);
			if(beta <= alpha) break;
		}
		return maxEvaluation;
	}
	else{
        float minEvaluation = std::numeric_limits<float>::max();
		for(auto& move : availableMoves){
			GameState newState = this->applyMove(move, *this);
			float  evaluation = newState.minimax(depth - 1, alpha, beta, (playerID % 4) + 1);
			minEvaluation = std::min(minEvaluation, evaluation);
			beta = std::min(beta, evaluation);
			if(beta <= alpha) break;
		}
		return minEvaluation;
	}
}

// 計算面積起始點要是某個玩家的地盤，並且沒有走過
float GameState::evaluate(){
	std::vector<std::vector<bool>> visited(MAXGRID, std::vector<bool>(MAXGRID, false));
	std::vector<float> playerArea(5, 0);
	playerArea[0] = -1;
	for(int y = 0 ; y < MAXGRID ; ++y){
		for(int x = 0 ; x < MAXGRID ; ++x){
			int anyPlayerID = this->mapState[x][y];
			if(visited[x][y] or anyPlayerID == 0 or anyPlayerID == -1) continue;
			float area = pow(this->dfs(x, y, visited, anyPlayerID, x, y, 0), exponentEvaluate);
			playerArea[anyPlayerID] += area;
		}
	}
	return playerArea[this->playerID];
	// int rank = 4;
	// for(int i = 1 ; i <= 4 ; ++i) if(playerArea[i] < playerArea[this->playerID]) --rank;
	// return -1 * rank;
}
/*
    選擇起始位置
    選擇範圍僅限場地邊緣(至少一個方向為牆)
    
    return: init_pos
    init_pos=<x,y>,代表你要選擇的起始位置
    
*/


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
std::vector<int> GetStep(int playerID, int mapStat[MAXGRID][MAXGRID], int sheepStat[MAXGRID][MAXGRID], std::vector<sheepBlock>& sb)
{
	std::vector<int> step;
	step.resize(5);
	GameState gameState(playerID, mapStat, sheepStat, sb);
	int depth = 10;
	int bestScore = INT_MIN;
    Move bestMove;
	std::vector<Move> availableMoves = gameState.getWhereToMoves();
	if (availableMoves.empty()){
		step[0] = -1;
		step[1] = -1;
		return step;
	}
	for (auto& move : availableMoves) {
		GameState newState = gameState.applyMove(move, gameState);
		int score = newState.minimax(depth, INT_MIN, INT_MAX, playerID);
		fprintf(outfile, "\nMove: (x, y) = (%d, %d), (xMove, yMove, direction) = (%d, %d, %d), sheepNum = %d, (score, bestscore) = (%d, %d)\n", move.x, move.y, move.x + dx[move.direction] * move.displacement, move.y + dy[move.direction] * move.displacement, move.direction, move.subSheepNumber, score, bestScore);
		if (score > bestScore) {
			bestScore = score;
			bestMove = move;
		}
	}
    // 返回最佳移動
    step[0] = bestMove.x;
    step[1] = bestMove.y;
    step[2] = bestMove.subSheepNumber;
    step[3] = bestMove.direction;
	step[4] = bestMove.displacement;
	sb.push_back(std::make_pair(bestMove.x + dx[bestMove.direction] * bestMove.displacement, bestMove.y + dy[bestMove.direction] * bestMove.displacement));
	fprintf(outfile, "\nStep: (x, y) = (%d, %d), (xMove, yMove, direction) = (%d, %d, %d), sheepNum = %d, bestscore = %d\n", bestMove.x, bestMove.y, bestMove.x + dx[bestMove.direction] * bestMove.displacement, bestMove.y + dy[bestMove.direction] * bestMove.displacement, bestMove.direction, bestMove.subSheepNumber, bestScore);
	printMapAndSheep(mapStat, sheepStat);
	printsb(sb);
    return step;    
}

	int main()
	{
		std::string filename = "output.txt";
		outfile = fopen(filename.c_str(), "w");

		int id_package;
		int playerID;
		int mapStat[12][12];
		int sheepStat[12][12];
		std::vector<sheepBlock> sheepBlocks;

		// player initial
		GetMap(id_package, playerID, mapStat);
		std::vector<int> init_pos = InitPos(mapStat);
		SendInitPos(id_package,init_pos);
		sheepBlocks.emplace_back(std::make_pair(init_pos[0], init_pos[1]));

		while (true){
			if(GetBoard(id_package, mapStat, sheepStat)) break;
			std::vector<int> bestMove = GetStep(playerID, mapStat, sheepStat, sheepBlocks);
			std::vector<int> step = {bestMove[0], bestMove[1], bestMove[2], bestMove[3]};
			SendStep(id_package, step);
		}
		fclose(outfile);
	}
