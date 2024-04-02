
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
#include <queue>
#include <functional>
#include <set>

#define MAXGRID 12
#define powSurroundLen 1.5
#define weightSurroundLen 1
#define weightEmptyNum 3
#define rewardOpponentNear 10
#define rewardOpponentFar 5
#define weightDfsArea 0.45
#define weightOpponentNum 0.28
#define weightOpponentSheep 0.27
#define exponentDFSArea 1.5
#define exponentEvaluate 1.25
#define MCTS_SIMULATIONS 10
#define minimaxDepth 2
#define FLT_MAX std::numeric_limits<float>::max()
#define FLT_MIN std::numeric_limits<float>::min()
#define isEveryPosibility false

// 8個方向 
constexpr int dx[] = {0, -1, 0, 1, -1, 0, 1, -1, 0, 1};
constexpr int dy[] = {0, -1, -1, -1, 0, 0, 0, 1, 1, 1};
// 4個方向
constexpr int dxx[] = {-1, 0, 1, 0};
constexpr int dyy[] = {0, 1, 0, -1};

FILE* outfile;

inline bool isPositionValid(int x, int y) { return x >= 0 && x < MAXGRID && y >= 0 && y < MAXGRID; }
inline bool isPositionValidForOccupying(int x, int y, int mapState[MAXGRID][MAXGRID]) {return isPositionValid(x, y) && mapState[x][y] == 0; }
inline bool isPositionBelongToPlayer(int x, int y, int playerID, int mapState[MAXGRID][MAXGRID]) { return isPositionValid(x, y) && mapState[x][y] == playerID; }
inline bool isPositionValidForOccupyingOrBelongToPlayer(int x, int y, int playerID, int mapState[MAXGRID][MAXGRID]) { return isPositionValid(x, y) && (mapState[x][y] == 0 or mapState[x][y] == playerID); }

struct NewMapBlock{
	int x;
	int y;
	int playerID;
	// int sheepNumber;
}

// 移動方式
struct Move{
	int x;
	int y;
	int subSheepNumber;
	int direction;
	int displacement;
};

struct rootSize{
	int x;
	int y;
	int size;
};

typedef struct Move Move;
typedef std::pair<int, int> sheepBlock; 

std::vector<NewMapBlock> compareChangesInMapState(int newMapState[MAXGRID][MAXGRID], int lastMapState[MAXGRID][MAXGRID]){
	std::vector<NewMapBlock> changes;
	for(int y = 0 ; y < MAXGRID ; ++y){
		for(int x = 0 ; x < MAXGRID ; ++x){
			if(newMapState[x][y] != lastMapState[x][y]){
				changes.push_back(NewMapBlock{x, y, newMapState[x][y]});
			}
		}
	}
	std::copy(&newMapState[0][0], &newMapState[0][0] + MAXGRID * MAXGRID, &lastMapState[0][0]);
	return changes;
}


class UnionFind{
	friend class GameState;
	private:
		std::vector<int> parent;
		std::vector<int> size;
		std::set<int> rootSet;
	public:
		UnionFind(){}

		UnionFind(int unionSize){
			parent.resize(unionSize);
			size.resize(unionSize, 0);
			for(int i = 0 ; i < unionSize ; ++i) parent[i] = i;
		}

		UnionFind(const UnionFind& lastUnionFind){
			std::copy(lastUnionFind.parent.begin(), lastUnionFind.parent.end(), this->parent.begin());
			std::copy(lastUnionFind.size.begin(), lastUnionFind.size.end(), this->size.begin());
			std::copy(lastUnionFind.rootSet.begin(), lastUnionFind.rootSet.end(), this->rootSet.begin());
		}

		inline int unionFindIndex(int x, int y){
			return x * MAXGRID + y;
		}

		int findUnionRoot(int x, int y){
			int index = unionFindIndex(x, y);
			while(parent[index] != index){
				parent[index] = parent[parent[index]];
				index = parent[index];
			}
			return index;
		}

		void unionSet(int xA, int yA, int xB, int yB){
			int rootA = findUnionRoot(xA, yA);
			int rootB = findUnionRoot(xB, yB);
			if(size[rootA] == 0) size[rootA] = 1;
			if(size[rootB] == 0) size[rootB] = 1;
			
			if(rootA != rootB){
				if(size[rootA] < size[rootB]) std::swap(rootA, rootB);
				size[rootA] += size[rootB];
				size[rootB] = 0;	
				parent[rootB] = rootA;
				rootSet.erase(rootB);
			}
		}

		inline void addRoot(int x, int y){
			rootSet.insert(unionFindIndex(x, y));
		}

		inline int getUnionSize(int x, int y){
			return size[findUnionRoot(x, y)];
		}

		inline std::vector<rootSize> returnRootSizes(){
			std::vector<rootSize> rootSizes;
			for(auto& root : rootSet){
				rootSizes.emplace_back(rootSize{root/MAXGRID, root%MAXGRID, size[root]});
			}
			return rootSizes;
		}

		// std::vector<sheepBlock> returnRootSize(){
		// 	std::vector<sheepBlock> rootSize;
		// 	for(int i = 0 ; i < MAXGRID * MAXGRID ; ++i) if(size[i] != 0) rootSize.emplace_back(std::make_pair(i/MAXGRID, i%MAXGRID));
		// 	return rootSize;
		// } 
};


struct MCTSNode {
    int visits;
    float value;
    std::vector<Move> untriedMoves;
    std::vector<MCTSNode*> children;
    
    MCTSNode(std::vector<Move> moves) : visits(0), value(0), untriedMoves(moves) {}
};

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

void scorePosition(int x, int y, int mapStat[MAXGRID][MAXGRID], int& scoreEmptyNum, double& scoreSurroundNum) {
    scoreEmptyNum = 0;
    scoreSurroundNum = 0;

    for (int i = 1; i <= 9; i++) {
        if (i == 5) continue;
        int newX = x + dx[i], newY = y + dy[i];

        if (!isPositionValidForOccupying(newX, newY, mapStat)) continue;
        // 計算周圍空格數
		scoreEmptyNum++;
        // 計算周圍延伸距離
        int lenSurround = 0;
        while (isPositionValidForOccupying(newX, newY, mapStat)) {
            lenSurround++;
            newX += dx[i];
            newY += dy[i];
        }
        scoreSurroundNum += pow(lenSurround, powSurroundLen);
    }
}

int scoreOpponentDistance(int x, int y, int mapStat[MAXGRID][MAXGRID], int playerID) {
    int opponentDistance = INT_MAX;
	bool haveOpponent = false;
    for (int iy = 0; iy < MAXGRID; iy++) {
        for (int ix = 0; ix < MAXGRID; ix++) {
            if (mapStat[ix][iy] != playerID && mapStat[ix][iy] > 0) {
				haveOpponent = true;
                int distance = std::max(abs(ix - x), abs(iy - y));
                opponentDistance = std::min(opponentDistance, distance);
            }
        }
    }
	if(!haveOpponent) return -1;
    return opponentDistance;
}

/*
    選擇起始位置
    選擇範圍僅限場地邊緣(至少一個方向為牆)
    
    return: init_pos
    init_pos=<x,y>,代表你要選擇的起始位置
    
*/
std::vector<int> InitPos(int playerID, int mapStat[MAXGRID][MAXGRID]) {
    std::vector<int> init_pos;
    init_pos.resize(2);
    float maxScore = 0;

    for (int y = 0; y < MAXGRID; y++) {
        for (int x = 0; x < MAXGRID; x++) {
			//檢查是否為障礙或其他player
            if (mapStat[x][y] != 0) continue;

			//檢查是否為場地邊緣
            bool isEdge = false;
            for (int i = 0; i < 4; i++) {
                int newX = x + dxx[i], newY = y + dyy[i];
                if (!isPositionValid(newX, newY)) continue;
                if (mapStat[newX][newY] == -1) isEdge = true;
            }

			//分數1 : 周圍空格數、周圍延伸距離
            int scoreEmptyNum;
            double scoreSurroundNum;
            scorePosition(x, y, mapStat, scoreEmptyNum, scoreSurroundNum);
			//分數2 : 對手距離
            int opponentDistance = scoreOpponentDistance(x, y, mapStat, playerID);

			//限定場地邊緣
            if (isEdge == false) continue;

			//目標周圍空格多、周圍延伸距離短
            float score = weightEmptyNum * scoreEmptyNum - weightSurroundLen * scoreSurroundNum / 8;

			// fprintf(outfile, "(%d, %d), score = (%d*%d - %d*%f/8)", x, y, weightEmptyNum, scoreEmptyNum, weightSurroundLen, scoreSurroundNum);
            // 根據與對手的距離調整分數
			if (opponentDistance <= 2 && opponentDistance >= 0) {
                score -= rewardOpponentNear;  // 離對手太近,減少分數
				// fprintf(outfile, " - %d = %f, distance = %d\n", rewardOpponentNear, score, opponentDistance);
            } else if (opponentDistance >= 6) {
                score += rewardOpponentFar;   // 離對手較遠,增加分數
				// fprintf(outfile, " - %d = %f, distance = %d\n", rewardOpponentFar, score, opponentDistance);
            }

            if (score > maxScore) {
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
	friend class UnionFind;
	private:
		int myPlayerID;
		int mapState[MAXGRID][MAXGRID];
		int sheepState[MAXGRID][MAXGRID];
		UnionFind unionFind;
		std::vector<std::vector<sheepBlock>> sheepBlocks;
	public:
		// 從 GetStep 取得的 GameState（server 回傳）不需要進行複製
		GameState(int playerID, int mapState[MAXGRID][MAXGRID], int sheepState[MAXGRID][MAXGRID], std::vector<newMapBlock>& newMapBlocks, UnionFind& lastUnionFind, std::vector<std::vector<sheepBlock>>& lastSheepBlocks){
			this->myPlayerID = playerID;
			this->mapState = mapState;
			this->sheepState = sheepState;
			this->unionFind = lastUnionFind;
			this->sheepBlocks = lastSheepBlocks;

			for(auto& newMapBlock : newMapBlocks){
				int x = newMapBlock.x, y = newMapBlock.y, anyPlayerID = newMapBlock.playerID;
				this->sheepBlocks[anyPlayerID].emplace_back(std::make_pair(x, y));
				for(int i = 1 ; i < 4 ; ++i){
					int xMove = x + dxx[i];
					int yMove = y + dyy[i];
					if(isPositionBelongToPlayer(xMove, yMove, anyPlayerID, mapState)) this->unionFind.unionSet(x, y, xMove, yMove);
					else if(isPositionValidForOccupying(xMove, yMove, mapState)) this->unionFind.addRoot(xMove, yMove);
				}
			}
		}

		// 從 applyMove 取得的 GameState，需要進行複製
		GameState(int playerID, int mapState[MAXGRID][MAXGRID], int sheepState[MAXGRID][MAXGRID], std::vector<std::vector<sheepBlock>> sheepBlocks){
			this->myPlayerID = playerID;
			std::copy(&mapState[0][0], &mapState[0][0] + MAXGRID * MAXGRID, &this->mapState[0][0]);
			std::copy(&sheepState[0][0], &sheepState[0][0] + MAXGRID * MAXGRID, &this->sheepState[0][0]);
			this->sheepBlocks = sheepBlocks;
		}

		GameState(const GameState& lastGameState){
			this->myPlayerID = lastGameState.myPlayerID;
			std::copy(&lastGameState.mapState[0][0], &lastGameState.mapState[0][0] + MAXGRID * MAXGRID, &this->mapState[0][0]);
			std::copy(&lastGameState.sheepState[0][0], &lastGameState.sheepState[0][0] + MAXGRID * MAXGRID, &this->sheepState[0][0]);
			std::copy(lastGameState.sheepBlocks.begin(), lastGameState.sheepBlocks.end(), this->sheepBlocks.begin());
			std::copy(lastGameState.unionFind.parent.begin(), lastGameState.unionFind.parent.end(), this->unionFind.parent.begin());
		}


		// return function
		inline int getPlayerID() { return this->myPlayerID; }
		inline int (*getMapState())[MAXGRID] { return this->mapState; }
		inline int (*getSheepState())[MAXGRID] { return this->sheepState; }
		inline std::vector<std::vector<sheepBlock>> getMySheepBlocks() { return this->sheepBlocks; }

		// 給定某玩家，找出他可以走的所有地方
		std::vector<Move> getWhereToMoves(int anyPlayerID, bool everyPosibility);

		// 依據地圖特性為羊群進行分割
		int getSheepNumberToDivide(int xMove, int yMove, int x, int y, int anyPlayerID);
		std::vector<float>  calculateArea(int x, int y, int anyPlayerID);
		int dfs(int x, int y, std::vector<std::vector<bool>>& visited, int anyPlayerID, int originX, int originY, bool nineNine);
		int bfs(int x, int y, std::vector<std::vector<bool>>& visited, int anyPlayerID, bool (*areaType)(int, int, int, int[MAXGRID][MAXGRID]));

		// 產生新的GameState
		GameState applyMove(Move move, GameState state, int anyPlayerID);

		// minimax
		float minimax(int depth, float alpha, float beta, int playerID);

		// MCTS
		float mcts(MCTSNode* node, int playerID);
		float simulate(int playerID);
		inline bool isTerminal(std::vector<bool> isNoMove);

		
		// evaluate
		float evaluate();
		float evaluateByUnionFind();

		// 返回最佳 move
		Move getBestMove(int depth, int playerID);
};



/* 找可以走的地方 
	1. 找出我有羊群的位置（>1）
	2. 找出所有羊群各自可以走的地方
	3. 分隔羊群
	4. 儲存分隔後的羊群可以走的地方 
*/
// 找出anyPlayer的羊群可以走的地方 (該位置有羊群，之後走到底)
std::vector<Move> GameState::getWhereToMoves(int anyPlayerID, bool everyPosibility){
	std::vector<Move> moves;
	for(auto& sheepBlock : this->sheepBlocks[anyPlayerID]){
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
			int subSheepNumber = this->getSheepNumberToDivide(xMove, yMove, x, y, anyPlayerID);
			int displacement = std::max(abs(xMove - x), abs(yMove - y));
			if(everyPosibility) for(int subSheepNumber = 1; subSheepNumber < sheepNumber; ++subSheepNumber) moves.emplace_back(Move{x, y, subSheepNumber, direction, displacement});
			else moves.emplace_back(Move{x, y, subSheepNumber, direction, displacement});
		}
	}
	return moves;
}

// 這裡的函數算法會依據地圖特性為"給定玩家選定位置中"的羊群進行分割
// 使用 DFS 兩邊的連通面積得分 再根據得分比例決定要分割多少羊群
int GameState::getSheepNumberToDivide(int xMove, int yMove, int x, int y, int anyPlayerID){
	int sheepNumber = this->sheepState[x][y];

	std::vector<float> totalAreaMove = this->calculateArea(xMove, yMove, anyPlayerID);
	std::vector<float> totalArea = this->calculateArea(x, y, anyPlayerID);
	
	//parameter0
	float dfsAreaMove = totalAreaMove[0];
	float dfsArea = totalArea[0];
	//parameter1
	float opponentNumMove = 1 - totalAreaMove[1];
	float opponentNum = 1 - totalArea[1];
	//parameter2
	float opponentSheepMove = 1 - totalAreaMove[2];
	float opponentSheep = 1- totalArea[2];

	float sheepDivideRatio = 
		weightDfsArea * (dfsAreaMove / (dfsAreaMove + dfsArea)) + 
		weightOpponentNum * (opponentNumMove / (opponentNumMove + opponentNum)) +
		weightOpponentSheep * (opponentSheepMove / (opponentSheepMove + opponentSheep)
	);
	// fprintf(outfile, "dfsAreaMove, dfsArea, opponentNumMove, opponentNum, opponentSheepMove, opponentSheep, sheepDivideRatio\n");
	// fprintf(outfile, "%f, %f, %f, %f, %f, %f, %f\n", dfsAreaMove, dfsArea, opponentNumMove, opponentNum, opponentSheepMove, opponentSheep, sheepDivideRatio);

	int sheepNumberToDivide = std::min(std::max(int(sheepNumber * sheepDivideRatio), 1) , sheepNumber - 1);
	return sheepNumberToDivide;			
}

std::vector<float> GameState::calculateArea(int x, int y, int anyPlayerID){
	std::vector<std::vector<bool>> visited(MAXGRID, std::vector<bool>(MAXGRID, true));
	for(int i = x - 2 ; i <= x + 2 ; ++i){
		for(int j = y - 2 ; j <= y + 2 ; ++j){
			if(isPositionValid(i, j)) visited[i][j] = false;
		}
	}

	std::vector<float> totalArea(3, 0);
	for(int i = 1 ; i <= 9 ; ++i){
		float area = 0;
		int xMove = x + dx[i];
		int yMove = y + dy[i];
		//(0)加總連通面積1.25次方 DFS
		if(isPositionValidForOccupying(xMove, yMove, this->mapState)) area = pow(this->bfs(xMove, yMove, visited, anyPlayerID, isPositionValidForOccupyingOrBelongToPlayer), exponentDFSArea);
		totalArea[0] += area;

		if(this->mapState[xMove][yMove] > 0) {
			// 這裡的 1/7 是因為 8個方向中有一個是自己
			// (1)對手數量
			totalArea[1] += (1/7.0);
			// (2)對手sheep數量
			totalArea[2] += this->sheepState[xMove][yMove] / (16.0 * 7);
		}else if(this->mapState[xMove][yMove] == -1){
			// (1)障礙數量
			totalArea[1] += 1/7.0;
			// (2)障礙sheep數量(算為1)
			totalArea[2] += 1 / (16.0 * 7);
		}
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

int GameState::bfs(int x, int y, std::vector<std::vector<bool>>& visited, int anyPlayerID, bool (*areaType)(int, int, int, int[MAXGRID][MAXGRID])) {
    int area = 0;
    std::queue<std::pair<int, int>> bfsQueue;
    bfsQueue.push({x, y});
    visited[x][y] = true;

    while (!bfsQueue.empty()) {
        auto [curX, curY] = bfsQueue.front();
        bfsQueue.pop();
        area++;

        for (int i = 0; i < 4; ++i) {
            int newX = curX + dxx[i];
            int newY = curY + dyy[i];
            if (areaType(newX, newY, anyPlayerID, this->mapState) && !visited[newX][newY]) {
                bfsQueue.push({newX, newY});
                visited[newX][newY] = true;
            }
        }
    }

    return area;
}

// 當前狀態下，anyPlayerID的人動了一步
GameState GameState::applyMove(Move move, GameState state, int anyPlayerID){
	GameState newState(state.myPlayerID, state.mapState, state.sheepState, state.sheepBlocks);
	int x = move.x, y = move.y;
	int xMove = x + dx[move.direction] * move.displacement, yMove = y + dy[move.direction] * move.displacement;
	newState.mapState[xMove][yMove] = anyPlayerID;
	newState.sheepState[x][y] -= move.subSheepNumber;
	newState.sheepState[xMove][yMove] = move.subSheepNumber;
	newState.sheepBlocks[anyPlayerID].emplace_back(std::make_pair(xMove, yMove));
	return newState;
}

// 輪到誰走了
// 如果換我走，就是最大值
// 如果換對手走，就是最小值
// 下一個玩家是誰(anyPlayerID % 4 + 1)
float GameState::minimax(int depth, float alpha, float beta, int anyPlayerID){
	std::vector<Move> availableMoves = this->getWhereToMoves(anyPlayerID, isEveryPosibility);

	// if(depth == 0 or availableMoves.empty()) return this->evaluate();
	if(availableMoves.empty()) return this->evaluate();
	if(depth == 0){
		MCTSNode root(availableMoves);
        for (int i = 0; i < MCTS_SIMULATIONS; ++i) {
            this->mcts(&root, anyPlayerID);
        }
		// fprintf(outfile, "root.value, root.visits = (%f, %d)\n", root.value, root.visits);
        return root.value / root.visits;
	}

    // std::sort(availableMoves.begin(), availableMoves.end(), [this, anyPlayerID](const Move& a, const Move& b) {
    //     GameState stateA = this->applyMove(a, *this, anyPlayerID);
    //     GameState stateB = this->applyMove(b, *this, anyPlayerID);
	// 	return stateA.evaluate() > stateB.evaluate();
    // });

	if(anyPlayerID == this->myPlayerID){
        float maxEvaluation = FLT_MIN;
		for(auto& move : availableMoves){
			GameState newState = this->applyMove(move, *this, anyPlayerID);
			float evaluation = newState.minimax(depth - 1, alpha, beta, (anyPlayerID % 4) + 1);
			maxEvaluation = std::max(maxEvaluation, evaluation);
			alpha = std::max(alpha, maxEvaluation);
			if(beta <= alpha) break;
		}
		return maxEvaluation;
	}
	if(anyPlayerID != this->myPlayerID){
        float minEvaluation = FLT_MAX;
		for(auto& move : availableMoves){
			GameState newState = this->applyMove(move, *this, anyPlayerID);
			float evaluation = newState.minimax(depth - 1, alpha, beta, (anyPlayerID % 4) + 1);
			minEvaluation = std::min(minEvaluation, evaluation);
			beta = std::min(beta, minEvaluation);
			if(beta <= alpha) break;
		}
		return minEvaluation;
	}
}

// 計算面積起始點要是某個玩家的地盤，並且沒有走過
float GameState::evaluate(){
	std::vector<float> playerArea(5, 0);
	playerArea[0] = -1;
	std::vector<std::vector<bool>> visited(MAXGRID, std::vector<bool>(MAXGRID, false));
	for(int y = 0 ; y < MAXGRID ; ++y){
		for(int x = 0 ; x < MAXGRID ; ++x){
			int anyPlayerID = this->mapState[x][y];
			if(visited[x][y] or anyPlayerID == 0 or anyPlayerID == -1) continue;
			float area = pow(this->bfs(x, y, visited, anyPlayerID, isPositionBelongToPlayer), exponentEvaluate);
			playerArea[anyPlayerID] += area;
		}
	}
	return playerArea[this->myPlayerID];
	// int rank = 1;
	// for(int i = 1 ; i <= 4 ; ++i) if(playerArea[i] < playerArea[this->myPlayerID]) ++rank;
	// return rank;
}

float GameState::evaluateByUnionFind(){
	std::vector<float> playerArea(5, 0);
	playerArea[0] = -1;
	std::vector<rootSize> rootSizes = this->unionFind.returnRootSizes();
	for(auto& rootSize : rootSizes){
		int anyPlayerID = this->mapState[rootSize.x][rootSize.y];
		if(anyPlayerID == 0 or anyPlayerID == -1) continue;
		playerArea[anyPlayerID] += pow(rootSize.size, exponentEvaluate);
	}
	return playerArea[this->myPlayerID];
}

Move GameState::getBestMove(int depth, int playerID){
	clock_t start, end;
	start = clock();

	float bestScore = FLT_MIN;
	Move bestMove = Move{-1, -1, -1, -1, -1};
	std::vector<Move> availableMoves = this->getWhereToMoves(playerID, isEveryPosibility);
	if(availableMoves.empty()) return bestMove;
	for(auto& move : availableMoves){
		GameState newState = this->applyMove(move, *this, playerID);
        int score = newState.minimax(depth - 1, FLT_MIN, FLT_MAX, (playerID % 4) + 1);
		if(score > bestScore){
			bestScore = score;
			bestMove = move;
		}
	}
	// printMapAndSheep(this->mapState, this->sheepState);

	end = clock();
	float time = (float)(end - start) / CLOCKS_PER_SEC;
	// fprintf(outfile, "\nBestmove: (x, y) = (%d, %d), (xMove, yMove, direction) = (%d, %d, %d), sheepNum = %d, bestscore = %f, time = %f\n", bestMove.x, bestMove.y, bestMove.x + dx[bestMove.direction] * bestMove.displacement, bestMove.y + dy[bestMove.direction] * bestMove.displacement, bestMove.direction, bestMove.subSheepNumber, bestScore, time);
	return bestMove;
}

float GameState::mcts(MCTSNode* node, int playerID) {
    if (node->untriedMoves.empty() && node->children.empty()) {
        return node->value;
    }
    if (!node->untriedMoves.empty()) {
        int idx = rand() % node->untriedMoves.size();
        Move move = node->untriedMoves[idx];
        node->untriedMoves.erase(node->untriedMoves.begin() + idx);
        
        GameState newState = this->applyMove(move, *this, playerID);
        MCTSNode* newNode = new MCTSNode(newState.getWhereToMoves((playerID % 4) + 1, isEveryPosibility));
        node->children.push_back(newNode);
        
        float value = newState.simulate((playerID % 4) + 1);
        newNode->value = value;
        newNode->visits = 1;

		node->visits++;
		node->value += value;
        
        return value;
    }
    else {
        float bestValue = FLT_MIN;
        MCTSNode* bestChild = nullptr;
        
        for (MCTSNode* child : node->children) {
            float ucb = child->value / child->visits + sqrt(2 * log(node->visits) / child->visits);
			if (ucb > bestValue) {
                bestValue = ucb;
                bestChild = child;
            }
        }
        float value = this->mcts(bestChild, (playerID % 4) + 1);
        node->visits++;
        node->value += value;
        
        return value;
    }
}

float GameState::simulate(int playerID) {
    GameState state = *this;
	std::vector<bool> isNoMove(4, false);
    while (!state.isTerminal(isNoMove)) {
        std::vector<Move> availableMoves = state.getWhereToMoves(playerID, isEveryPosibility);
        if (!availableMoves.empty()) {
            int idx = rand() % availableMoves.size();
            state = state.applyMove(availableMoves[idx], state, playerID);
        }else{
			isNoMove[playerID - 1] = true;
		}
        playerID = (playerID % 4) + 1;
    }
    return state.evaluate();
}

inline bool GameState::isTerminal(std::vector<bool> isNoMove) {
	for(int i = 0 ; i < 4 ; i++) if(!isNoMove[i]) return false;
    return true;
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
std::vector<int> GetStep(int playerID, int mapStat[MAXGRID][MAXGRID], int sheepStat[MAXGRID][MAXGRID], std::vector<NewMapBlock> newMapBlocks){
	std::vector<int> step(5, 0);
	GameState initState(playerID, mapStat, sheepStat, newMapBlocks);
	Move bestMove = initState.getBestMove(minimaxDepth, playerID);

    // 返回最佳移動
    step[0] = bestMove.x;
    step[1] = bestMove.y;
    step[2] = bestMove.subSheepNumber;
    step[3] = bestMove.direction;
	step[4] = bestMove.displacement;
    return step;    
}

int main()
{
	// std::string filename = "output.txt";
	// outfile = fopen(filename.c_str(), "w");

	int id_package;
	int playerID;
	int mapStat[MAXGRID][MAXGRID];
	int lastMapStat[MAXGRID][MAXGRID];
	int sheepStat[MAXGRID][MAXGRID];

	GetMap(id_package, playerID, mapStat);
	std::vector<int> init_pos = InitPos(playerID, mapStat);
	SendInitPos(id_package,init_pos);

	while (true){
		if(GetBoard(id_package, mapStat, sheepStat)) break;
		std::vector<NewMapBlock> newMapBlocks = compareChangesInMapState(mapStat, lastMapStat);
		std::vector<int> bestMove = GetStep(playerID, mapStat, sheepStat, newMapBlocks);
		std::vector<int> step = {bestMove[0], bestMove[1], bestMove[2], bestMove[3]};
		SendStep(id_package, step);
	}

	// fclose(outfile);
}
