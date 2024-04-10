#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <utility>
#include <math.h>
#include <algorithm>
#include <limits.h>
#include <memory>
#include <chrono>
#include <limits>
#include <queue>
#include <functional>
#include <set>
#include <future>

#define MAXGRID 12
#define powSurroundLen 1.5
#define weightSurroundLen 1
#define weightEmptyNum 2
#define rewardOpponentNear 10
#define rewardOpponentFar 5
#define weightDfsArea 0.45
#define weightOpponentNum 0.28
#define weightOpponentSheep 0.27
#define exponentDFSArea 1.5
#define exponentEvaluate 1.25
#define MCTSSIMULATIONS 2
#define MCTSDEPTH 3
#define minimaxDepth 2
#define FLT_MAX std::numeric_limits<float>::max()
#define FLT_MIN std::numeric_limits<float>::min()
#define isEveryPosibility false

FILE* outfile;

// 8個方向 
constexpr int dx[] = {0, -1, 0, 1, -1, 0, 1, -1, 0, 1};
constexpr int dy[] = {0, -1, -1, -1, 0, 0, 0, 1, 1, 1};
// 4個方向
constexpr int dxx[] = {-1, 0, 1, 0};
constexpr int dyy[] = {0, 1, 0, -1};
// 判斷位置所屬及合法
inline bool isPositionValid(int x, int y) { return x >= 0 && x < MAXGRID && y >= 0 && y < MAXGRID; }
inline bool isPositionValidForOccupying(int x, int y, std::vector<std::vector<int>>& mapState) {return isPositionValid(x, y) && mapState[x][y] == 0; }
inline bool isPositionBelongToPlayer(int x, int y, int playerID, std::vector<std::vector<int>>& mapState) { return isPositionValid(x, y) && mapState[x][y] == playerID; }
inline bool isPositionValidForOccupyingOrBelongToPlayer(int x, int y, int playerID, std::vector<std::vector<int>>& mapState) { return isPositionValid(x, y) && (mapState[x][y] == 0 or mapState[x][y] == playerID); }
// 可移動方向以及移動距離，羊群
struct Move{
	int x;
	int y;
	int subSheepNumber;
	int direction;
	int displacement;
};
// UnionFind 中所有 root 的位置及大小
struct rootSize{
	int x;
	int y;
	int size;
};
// 羊群位置，不用慢慢從 mapState 中找
typedef std::pair<int, int> sheepBlock; 
// 檢查回傳地圖狀態是否有變化，適用在 unionFind
struct NewMapBlock{
	int x;
	int y;
	int playerID;
};
class UnionFind;
class GameState;
class MCTS;

void printMapAndSheep(const std::vector<std::vector<int>>& mapStat, const std::vector<std::vector<int>>& sheepStat){
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
class UnionFind{
	friend class GameState;
	private:
		std::vector<int> parent;
		std::vector<int> size;
		std::set<int> rootSet;
	public:
		UnionFind(){}
		UnionFind(int unionSize);
		UnionFind(const UnionFind& lastUnionFind);
		~UnionFind(){}
		inline int unionFindIndex(int x, int y);
		int findUnionRoot(int x, int y);
		void unionSet(int x, int y, int xMove, int yMove);
		inline void addRoot(int x, int y);
		inline int getUnionSize(int x, int y);
		inline std::vector<rootSize> returnRootSizes();
		void expandUnionFind(int x, int y, int anyPlayerID, std::vector<std::vector<int>>& mapState);
};
class GameState{
	friend class UnionFind;
	friend class MCTSNode;
	private:
		bool isCopy;
		int myPlayerID;
		UnionFind unionFind;
		std::vector<std::vector<int>> mapState;
		std::vector<std::vector<int>> sheepState;
		std::vector<std::vector<sheepBlock>> sheepBlocks;
	public:
		GameState(){};
		GameState(int playerID, std::vector<std::vector<int>>& mapState, std::vector<std::vector<int>>& sheepState, UnionFind& lastUnionFind, std::vector<std::vector<sheepBlock>>& lastSheepBlocks);
		GameState(const GameState& lastGameState);
		GameState(const GameState& lastGameState, bool noCopy);
		~GameState(){};
		inline int getPlayerID() { return this->myPlayerID; }
		inline std::vector<std::vector<int>> getMapState() { return this->mapState; }
		inline std::vector<std::vector<int>> getSheepState() { return this->sheepState; }
		inline std::vector<std::vector<sheepBlock>> getMySheepBlocks() { return this->sheepBlocks; }
		std::vector<Move> getWhereToMoves(int anyPlayerID, bool everyPosibility);
		int getSheepNumberToDivide(int xMove, int yMove, int x, int y, int anyPlayerID);
		std::vector<float>  calculateArea(int x, int y, int anyPlayerID);
		int dfs(int x, int y, std::vector<std::vector<bool>>& visited, int anyPlayerID, int originX, int originY, bool nineNine);
		int bfs(int x, int y, std::vector<std::vector<bool>>& visited, int anyPlayerID, bool (*areaType)(int, int, int, std::vector<std::vector<int>>&));
		GameState applyMove(Move move, const GameState& state, int anyPlayerID);
		GameState applyMoveForRollout(Move move, const GameState& state, int anyPlayerID);
		float minimax(int depth, float alpha, float beta, int playerID, Move* bestMove = nullptr);
		float evaluateByUnionFind();
		inline bool isTerminal();
		// float mcts(MCTSNode* node, int playerID, int depth, int maxDepth);
		// float simulate(int playerID, int simulateDepth);
};
struct MCTSNode {
    int visits;
    double value;
	int roundPlayerID;
	MCTSNode* parent;
	GameState state;
    std::vector<Move> untriedMoves;
    std::vector<MCTSNode*> children;
    
	MCTSNode(const GameState &state, int roundPlayerID, MCTSNode* lastParent = nullptr){
		this->parent = lastParent;
		this->roundPlayerID = roundPlayerID;
		this->state = GameState(state);
		this->untriedMoves = this->state.getWhereToMoves(roundPlayerID, isEveryPosibility);
		this->visits = 0;
		this->value = 0;
	}
};
class MCTS{
	private:
		int simulationNumber;
		double explorationConstant;
		int MCTS_DEPTH;
		MCTSNode* root;
	public:
		MCTS(int numSimulations, double explorationConstant, int MCTS_DEPTH, const GameState& state, int roundPlayerID);
		MCTSNode* mctsExpansion(MCTSNode* parent);
		MCTSNode* mctsSelection(MCTSNode* parent);
		float mctsRollout(GameState& state, int anyPlayerID, int depth);
		inline void mctsBackpropagation(MCTSNode* node, float value);
		float mctsSimulation(MCTSNode* node, int anyPlayerID, int depth, int maxDepth);
		double mcts();
		MCTSNode* getRoot(){ return this->root; }
};

// MCTS constructor
MCTS::MCTS(int numSimulations, double explorationConstant, int MCTS_DEPTH, const GameState& state, int roundPlayerID){
	this->MCTS_DEPTH = MCTS_DEPTH;
	this->simulationNumber = numSimulations;
	this->explorationConstant = explorationConstant;
	this->root = new MCTSNode(state, roundPlayerID);
}
// 選擇要進行展開下一層的葉節點(從動作中選擇)
MCTSNode* MCTS::mctsSelection(MCTSNode* parent){
	MCTSNode* bestChild = nullptr;
	double bestValue = FLT_MIN;
	for(auto& child : parent->children){
		float ucb = child->value / child->visits + this->explorationConstant * sqrt(log(parent->visits) / child->visits);
		if(ucb > bestValue){
			bestValue = ucb;
			bestChild = child;
		}
	}
	return bestChild;
}
// 擴展節點
MCTSNode* MCTS::mctsExpansion(MCTSNode* parent){
    int index = rand() % parent->untriedMoves.size(); 
    Move move = parent->untriedMoves[index]; 
    parent->untriedMoves.erase(parent->untriedMoves.begin() + index); 
    MCTSNode* child = new MCTSNode(parent->state.applyMove(move, parent->state, parent->roundPlayerID), (parent->roundPlayerID % 4) + 1, parent); 
    parent->children.emplace_back(child);
    return child;
}
// 模擬遊戲直到遊戲結束
float MCTS::mctsRollout(GameState &state, int anyPlayerID, int depth){
	while(!state.isTerminal() && depth > 0){
		std::vector<Move> moves = state.getWhereToMoves(anyPlayerID, isEveryPosibility);
		fprintf(outfile, "============= roundPlayerID = %d ===============\n", anyPlayerID);
		if(!moves.empty()){
			for(auto& move : moves){
				fprintf(outfile, "x = %d, y = %d, subSheepNumber = %d, direction = %d, displacement = %d\n", move.x, move.y, move.subSheepNumber, move.direction, move.displacement);
			}
			int index = rand() % moves.size();
			fprintf(outfile, "move[%d] = x = %d, y = %d, subSheepNumber = %d, direction = %d, displacement = %d\n", index, moves[index].x, moves[index].y, moves[index].subSheepNumber, moves[index].direction, moves[index].displacement);
			state = state.applyMoveForRollout(moves[index], state, anyPlayerID);
		}
		printMapAndSheep(state.getMapState(), state.getSheepState());
		anyPlayerID = (anyPlayerID % 4) + 1;
		depth--;
	}
	return state.evaluateByUnionFind();
}
// 更新節點以及所有父母親的值
inline void MCTS::mctsBackpropagation(MCTSNode* node, float value){
	while(node != nullptr){
		node->visits++;
		node->value += value;
		node = node->parent;
	}
}
// MCTS 主要函數
float MCTS::mctsSimulation(MCTSNode* node, int anyPlayerID, int depth, int maxDepth){
	printf("mctsSimulation\n");
	// 選擇: 沒有可以嘗試的動作，且沒有子節點，且深度不到最大(非葉節點)
	if(node->untriedMoves.empty() && !node->children.empty() && depth < maxDepth){
		MCTSNode* bestChild = this->mctsSelection(node);
		return this->mctsSimulation(bestChild, anyPlayerID, depth + 1, maxDepth);
	}
	// 擴展: 有可以嘗試的動作，且深度不到最大(非葉節點)
	if(!node->untriedMoves.empty() && depth < maxDepth){
		MCTSNode* child = this->mctsExpansion(node);
		// 模擬直到結束
		float value = this->mctsRollout(child->state, anyPlayerID, maxDepth - depth - 1);
		// 更新節點以及所有父母親的值
		mctsBackpropagation(child, value);
		return value;
	}
	return node->state.evaluateByUnionFind();
	// return node->value / node->visits;
}

double MCTS::mcts(){
	for(int i = 0 ; i < this->simulationNumber ; ++i){
		this->mctsSimulation(this->root, this->root->roundPlayerID, 0, MCTS_DEPTH);
	}
	return this->root->value / this->root->visits;
}


// 版面資訊：棋盤狀態、羊群分布狀態、玩家ID、我的羊群位置（Stack）


UnionFind::UnionFind(int unionSize){
	parent.resize(unionSize);
	size.resize(unionSize, 1);
	for(int i = 0 ; i < unionSize ; ++i) parent[i] = i;
}

UnionFind::UnionFind(const UnionFind& lastUnionFind){
	this->parent = std::vector<int>(lastUnionFind.parent);
	this->size = std::vector<int>(lastUnionFind.size);
	this->rootSet = std::set<int>(lastUnionFind.rootSet);
}

inline int UnionFind::unionFindIndex(int x, int y){
	return x * MAXGRID + y;
}

int UnionFind::findUnionRoot(int x, int y){
	int index = unionFindIndex(x, y);
	while(parent[index] != index){
		parent[index] = parent[parent[index]];
		index = parent[index];
	}
	return index;
}

void UnionFind::unionSet(int x, int y, int xMove, int yMove){
	int root = findUnionRoot(x, y);
	int rootMove = findUnionRoot(xMove, yMove);
	// xMove 的 root 必然在 rootSet 中
	if(root != rootMove){
		if(size[rootMove] < size[root]) std::swap(rootMove, root);
		size[rootMove] += size[root];
		parent[root] = rootMove;
		if(rootSet.count(root)) rootSet.erase(root);
	}
}

inline void UnionFind::addRoot(int x, int y){
	rootSet.insert(unionFindIndex(x, y));
}

inline int UnionFind::getUnionSize(int x, int y){
	return size[findUnionRoot(x, y)];
}

inline std::vector<rootSize> UnionFind::returnRootSizes(){
	std::vector<rootSize> rootSizes;
	for(auto& root : rootSet){
		rootSizes.emplace_back(rootSize{root/MAXGRID, root%MAXGRID, size[root]});
	}
	return rootSizes;
}

void UnionFind::expandUnionFind(int x, int y, int anyPlayerID, std::vector<std::vector<int>>& mapState){
	bool anyConnect = false;
	for(int i = 0 ; i < 4 ; ++i){
		int xRound = x + dxx[i];
		int yRound = y + dyy[i];
		if(isPositionBelongToPlayer(xRound, yRound, anyPlayerID, mapState)){
			this->unionSet(x, y, xRound, yRound);
			anyConnect = true;
		}
	}
	if(!anyConnect) this->addRoot(x, y);	
}

void printMapAndSheepAndUnion(std::vector<std::vector<int>>& mapStat, std::vector<std::vector<int>>& sheepStat, UnionFind& unionFind){
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
			else if(mapStat[x][y] == 0)
				fprintf(outfile, "   O    |");
            else
                fprintf(outfile, "%d,%d,%d,%d|", mapStat[x][y], sheepStat[x][y], unionFind.findUnionRoot(x, y)/MAXGRID, unionFind.findUnionRoot(x, y)%MAXGRID);
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

void NotfprintMapAndSheep(std::vector<std::vector<int>>& mapStat, std::vector<std::vector<int>>& sheepStat){
    // Print column headers
	printf("y\\x ");
	for (int x = 0; x < MAXGRID; x++) {
		printf("  x=%-3d  ", x);    
	}
	printf("\n");

	// Print separator line
	printf("   +");
	for (int x = 0; x < MAXGRID; x++) {
		printf("--------+");
	}
	printf("\n");

	// Print map and sheep data
	for (int y = 0; y < MAXGRID; y++) {
		printf("y=%-2d|", y);
		for (int x = 0; x < MAXGRID; x++) {
			if (mapStat[x][y] == -1)
				printf("   X    |");
			else
				printf(" %d, %-3d |", mapStat[x][y], sheepStat[x][y]);
		}
		printf("\n");

		// Print separator line
		printf("   +");
		for (int x = 0; x < MAXGRID; x++) {
			printf("--------+");
		}
		printf("\n");
	}
	printf("\n");
}

void printsb(std::vector<sheepBlock> sb){
	for(auto& block : sb){
		fprintf(outfile, "(%d %d), ", block.first, block.second);
	}
}

void scorePosition(int x, int y, std::vector<std::vector<int>>& mapStat, int& scoreEmptyNum, double& scoreSurroundNum) {
    scoreEmptyNum = 0;
    scoreSurroundNum = 0;

	for(int i = -2 ; i <= 2 ; ++i){
		for(int j = -2 ; j <= 2 ; ++j){
			if(i == 0 and j == 0) continue;
			if(isPositionValidForOccupying(x + i, y + j, mapStat)) scoreEmptyNum++;
			if(isPositionValid(x + i, y + j) and mapStat[x+i][y+j] > 0) scoreEmptyNum -= 2;
		}
	}
    for (int i = 1; i <= 9; i++) {
        if (i == 5) continue;
        int newX = x + dx[i], newY = y + dy[i];

        if (!isPositionValidForOccupying(newX, newY, mapStat)) continue;
        // 計算周圍空格數
		// scoreEmptyNum++;
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

int scoreOpponentDistance(int x, int y, std::vector<std::vector<int>>& mapStat, int playerID) {
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

// 比較地圖狀態是否有變化
template <typename T>
std::vector<NewMapBlock> compareChangesInMapState(std::vector<std::vector<T>>& newMapState, std::vector<std::vector<T>>& lastMapState){
	std::vector<NewMapBlock> changes;
	for(int y = 0 ; y < MAXGRID ; ++y){
		for(int x = 0 ; x < MAXGRID ; ++x){
			if(newMapState[x][y] != lastMapState[x][y]){
				changes.emplace_back(NewMapBlock{x, y, newMapState[x][y]});
				lastMapState[x][y] = newMapState[x][y];
			}
		}
	}
	return changes;
}
// 選擇起始位置
std::vector<int> InitPos(int playerID, std::vector<std::vector<int>>& mapStat) {
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
// 不用複製版
GameState::GameState(int playerID, std::vector<std::vector<int>>& mapState, std::vector<std::vector<int>>& sheepState, UnionFind& lastUnionFind, std::vector<std::vector<sheepBlock>>& lastSheepBlocks){
	this->isCopy = false;
	this->myPlayerID = playerID;
	this->mapState = mapState;
	this->sheepState = sheepState;
	this->unionFind = lastUnionFind;
	this->sheepBlocks = lastSheepBlocks;
}
// 從 applyMove 或是 MCTS 取得的 GameState，需要進行複製
GameState::GameState(const GameState& lastGameState){
	this->isCopy = true;
	this->myPlayerID = lastGameState.myPlayerID;
	this->mapState = std::vector<std::vector<int>>(lastGameState.mapState);
	this->sheepState = std::vector<std::vector<int>>(lastGameState.sheepState);
	this->sheepBlocks = std::vector<std::vector<sheepBlock>>(lastGameState.sheepBlocks);
	this->unionFind = UnionFind(lastGameState.unionFind);
}
GameState::GameState(const GameState& lastGameState, bool noCopy){
	this->isCopy = false;
	this->myPlayerID = lastGameState.myPlayerID;
	this->mapState = lastGameState.mapState;
	this->sheepState = lastGameState.sheepState;
	this->sheepBlocks = lastGameState.sheepBlocks;
	this->unionFind = lastGameState.unionFind;
}
// 找出給定玩家所有可以走的地方
std::vector<Move> GameState::getWhereToMoves(int anyPlayerID, bool everyPosibility){
	std::vector<Move> moves;
	// 找出anyPlayer的羊群可以走的地方 (該位置有羊群，之後走到底)
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
			// 分隔羊群
			int subSheepNumber = this->getSheepNumberToDivide(xMove, yMove, x, y, anyPlayerID);
			int displacement = std::max(abs(xMove - x), abs(yMove - y));
			// 儲存分隔後的羊群可以走的地方 
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
// 計算面積(使用bfs)
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
	for(int i = 0 ; i < 4 ; ++i){
		int xMove = x + dxx[i];
		int yMove = y + dyy[i];
		if(isPositionValidForOccupying(xMove, yMove, this->mapState)) area += this->dfs(xMove, yMove, visited, anyPlayerID, originX, originY, nineNine);
	}
	return area;
}
// BFS 算連通面積，不可以走過，並且要是自己地盤或是空地
int GameState::bfs(int x, int y, std::vector<std::vector<bool>>& visited, int anyPlayerID, bool (*areaType)(int, int, int, std::vector<std::vector<int>>&)) {
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
// 採取該動作後的新版面
GameState GameState::applyMove(Move move, const GameState& lastState, int anyPlayerID){
	GameState newState(lastState);
	int x = move.x, y = move.y;
	int xMove = x + dx[move.direction] * move.displacement, yMove = y + dy[move.direction] * move.displacement;
	newState.mapState[xMove][yMove] = anyPlayerID;
	newState.sheepState[x][y] -= move.subSheepNumber;
	newState.sheepState[xMove][yMove] += move.subSheepNumber;
	newState.sheepBlocks[anyPlayerID].emplace_back(std::make_pair(xMove, yMove));
	newState.unionFind.expandUnionFind(xMove, yMove, anyPlayerID, newState.mapState);
	return newState;
}
GameState GameState::applyMoveForRollout(Move move, const GameState& lastState, int anyPlayerID){
	printMapAndSheep(lastState.mapState, lastState.sheepState);
	GameState newState(lastState, false);
	printMapAndSheep(newState.mapState, newState.sheepState);
	int x = move.x, y = move.y;
	int xMove = x + dx[move.direction] * move.displacement, yMove = y + dy[move.direction] * move.displacement;
	fprintf(outfile, "sheepState[%d][%d] = %d\n", x, y, newState.sheepState[x][y]);
	fprintf(outfile, "sheepState[%d][%d] = %d\n", xMove, yMove, newState.sheepState[xMove][yMove]);
	newState.sheepState[x][y] -= move.subSheepNumber;
	newState.mapState[xMove][yMove] = anyPlayerID;
	newState.sheepState[xMove][yMove] += move.subSheepNumber;
	newState.sheepBlocks[anyPlayerID].emplace_back(std::make_pair(xMove, yMove));
	fprintf(outfile, "sheepState[%d][%d] = %d\n", x, y, newState.sheepState[x][y]);
	fprintf(outfile, "sheepState[%d][%d] = %d\n", xMove, yMove, newState.sheepState[xMove][yMove]);
	newState.unionFind.expandUnionFind(xMove, yMove, anyPlayerID, newState.mapState);
	return newState;
}
// 輪到下一個人決定該怎麼動作
// 如果換我走，就是最大值
// 如果換對手走，就是最小值
// 下一個玩家是誰(anyPlayerID % 4 + 1)
float GameState::minimax(int depth, float alpha, float beta, int anyPlayerID, Move* bestMove){
	// 找所有可以動的地方
	std::vector<Move> availableMoves = this->getWhereToMoves(anyPlayerID, isEveryPosibility);
	// 終止條件
	if(availableMoves.empty()) return this->evaluateByUnionFind();
	if(depth == 0){
		return this->evaluateByUnionFind();
		// MCTS M(10, 1.414, 60, *this, anyPlayerID);
		// double value = M.mcts();
		// printf("value = %f\n", value);
		// return value;
	}
	// 加快 minimax 速度，先評估好的走法排在前面
    std::sort(availableMoves.begin(), availableMoves.end(), [this, anyPlayerID](const Move& a, const Move& b) {
        GameState stateA = this->applyMove(a, *this, anyPlayerID);
        GameState stateB = this->applyMove(b, *this, anyPlayerID);
		return stateA.evaluateByUnionFind() > stateB.evaluateByUnionFind();
    });

	if(anyPlayerID == this->myPlayerID){
        float maxEvaluation = FLT_MIN;
		for(auto& move : availableMoves){
			GameState newState = this->applyMove(move, *this, anyPlayerID);
			float evaluation = newState.minimax(depth - 1, alpha, beta, (anyPlayerID % 4) + 1);
			// printf("last evaluation = %f\n", evaluation);
			// 第一層的時候，要找出最好的走法
			if(depth == minimaxDepth and evaluation >= maxEvaluation) *bestMove = move;
			maxEvaluation = std::max(maxEvaluation, evaluation);
			alpha = std::max(alpha, maxEvaluation);
			if(beta <= alpha) break;
		}
        // std::vector<std::future<float>> futures;
		// for(auto& move : availableMoves){
		// 	futures.emplace_back(std::async(std::launch::async, [this, move, depth, alpha, beta, anyPlayerID](){
		// 		GameState newState = this->applyMove(move, *this, anyPlayerID);
		// 		return newState.minimax(depth - 1, alpha, beta, (anyPlayerID % 4) + 1);
		// 	}));
		// }

		// for(size_t i = 0 ; i < futures.size() ; ++i){
		// 	float evaluation = futures[i].get();
		// 	if(depth == minimaxDepth and evaluation >= maxEvaluation) *bestMove = availableMoves[i];
		// 	maxEvaluation = std::max(maxEvaluation, evaluation);
		// 	alpha = std::max(alpha, maxEvaluation);
		// 	if(beta <= alpha) break;
		// }

		return maxEvaluation;
	}

	if(anyPlayerID != this->myPlayerID){
        float minEvaluation = FLT_MAX;

		for(auto& move : availableMoves){
			GameState newState = this->applyMove(move, *this, anyPlayerID);
			float evaluation = newState.minimax(depth - 1, alpha, beta, (anyPlayerID % 4) + 1);
			// printf("last evaluation = %f\n", evaluation);
			minEvaluation = std::min(minEvaluation, evaluation);
			beta = std::min(beta, minEvaluation);
			if(beta <= alpha) break;
		}

        // std::vector<std::future<float>> futures;
		// for(auto& move : availableMoves){
		// 	futures.emplace_back(std::async(std::launch::async, [this, move, depth, alpha, beta, anyPlayerID](){
		// 		GameState newState = this->applyMove(move, *this, anyPlayerID);
		// 		return newState.minimax(depth - 1, alpha, beta, (anyPlayerID % 4) + 1);
		// 	}));
		// }

		// for(size_t i = 0 ; i < futures.size() ; ++i){
		// 	float evaluation = futures[i].get();
		// 	minEvaluation = std::min(minEvaluation, evaluation);
		// 	beta = std::min(beta, minEvaluation);
		// 	if(beta <= alpha) break;
		// }

		return minEvaluation;
	}
}
// 用UnionFind評估版面，可以選要評估哪個玩家的面積或是排名
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
	// int rank = 1;
	// for(int i = 1 ; i <= 4 ; ++i) if(playerArea[i] < playerArea[this->myPlayerID]) ++rank;
	// return rank;
}

// float GameState::mcts(MCTSNode* node, int playerID, int depth, int maxDepth){
// 	printf("depth = %d\n", depth);
//     if (node->untriedMoves.empty() && node->children.empty()) {
// 		printf("I am here 2\n");
// 		return node->state.evaluateByUnionFind();
//     }
// 	if (depth >= maxDepth){
// 		printf("I am here 3\n");
// 		return node->state.evaluateByUnionFind();
// 	}
//     if (!node->untriedMoves.empty()) {
// 		printf("I am here 4\n");
//         int idx = rand() % node->untriedMoves.size();
//         Move move = node->untriedMoves[idx];
//         node->untriedMoves.erase(node->untriedMoves.begin() + idx);

//         MCTSNode* newNode = new MCTSNode(this->applyMove(move, *this, playerID), (playerID % 4) + 1);
        
// 		node->children.emplace_back(newNode);
// 		printf("I am here 1\n");        
//         float value = newNode->state.simulate((playerID % 4) + 1, maxDepth - depth - 1);
// 		printf("value = %f\n", value);
//         newNode->value = value;
//         newNode->visits = 1;

// 		node->visits++;
// 		node->value += value;
//     }
//     else {
//         float bestValue = FLT_MIN;
//         MCTSNode* bestChild = nullptr;
        
//         for (MCTSNode* child : node->children) {
//             float ucb = child->value / child->visits + sqrt(2 * log(node->visits) / child->visits);
// 			if (ucb > bestValue) {
//                 bestValue = ucb;
//                 bestChild = child;
//             }
//         }
//         float value = this->mcts(bestChild, (playerID % 4) + 1, depth + 1, maxDepth);
//         node->visits++;
//         node->value += value;
//         return value;
//     }
// }

// 隨機採取動作，直到結束
// float GameState::simulate(int playerID, int simulateDepth) {
//     GameState state = *this;
// 	std::vector<bool> isNoMove(5, false);

//     while (!state.isTerminal(isNoMove)) {
//         std::vector<Move> availableMoves = state.getWhereToMoves(playerID, isEveryPosibility);
//         if (!availableMoves.empty()) {
//             int idx = rand() % availableMoves.size();
//             state = state.applyMove(availableMoves[idx], state, playerID);
//         }else{
// 			isNoMove[playerID] = true;
// 		}
//         playerID = (playerID % 4) + 1;
//     }
//     return state.evaluateByUnionFind();
// }

inline bool GameState::isTerminal() {
	if(!getWhereToMoves(1, isEveryPosibility).empty()) return false;
	if(!getWhereToMoves(2, isEveryPosibility).empty()) return false;
	if(!getWhereToMoves(3, isEveryPosibility).empty()) return false;
	if(!getWhereToMoves(4, isEveryPosibility).empty()) return false;
	return true;	
}

std::vector<int> getStepMCTS(int playerID, std::vector<std::vector<int>>& mapStat, std::vector<std::vector<int>>& sheepStat, std::vector<NewMapBlock>& newMapBlocks, UnionFind& unionFind, std::vector<std::vector<sheepBlock>>& sheepBlocks){
    GameState initState(playerID, mapStat, sheepStat, unionFind, sheepBlocks);
    MCTS mcts(MCTSSIMULATIONS, 1.414, MCTSDEPTH, initState, playerID);
    mcts.mcts();

    MCTSNode* bestChild = nullptr;
    double bestValue = FLT_MIN;

    for(auto& child : mcts.getRoot()->children){
        double value = child->value / child->visits;
        if(value > bestValue){
            bestValue = value;
            bestChild = child;
        }
    }

    if(bestChild == nullptr){
        std::vector<Move> availableMoves = initState.getWhereToMoves(playerID, isEveryPosibility);
        if(availableMoves.empty()){
            return {-1, -1, -1, -1}; // 無法移動
        }else{
            Move randomMove = availableMoves[rand() % availableMoves.size()];
            return {randomMove.x, randomMove.y, randomMove.subSheepNumber, randomMove.direction};
        }
    }

    Move bestMove = bestChild->untriedMoves[0];
    return {bestMove.x, bestMove.y, bestMove.subSheepNumber, bestMove.direction};
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

std::vector<int> GetStep(int playerID, std::vector<std::vector<int>>& mapStat, std::vector<std::vector<int>>& sheepStat, std::vector<NewMapBlock>& newMapBlocks, UnionFind& unionFind, std::vector<std::vector<sheepBlock>>& sheepBlocks){
	Move *bestMove = new Move();
	GameState initState(playerID, mapStat, sheepStat, unionFind, sheepBlocks);
	initState.minimax(minimaxDepth, FLT_MIN, FLT_MAX, playerID, bestMove);
	int bestX = bestMove->x;
	int bestY = bestMove->y;
	int bestSubSheepNumber = bestMove->subSheepNumber;
	int bestDirection = bestMove->direction;
	delete bestMove;
    return {bestX, bestY, bestSubSheepNumber, bestDirection};
}

void mapStatToVector(int mapStat[MAXGRID][MAXGRID], std::vector<std::vector<int>>& mapStatVector){
	for(int i = 0 ; i < MAXGRID ; ++i){
		for(int j = 0 ; j < MAXGRID ; ++j){
			mapStatVector[i][j] = mapStat[i][j];
		}
	}
}

int main(){
	std::string filename = "output.txt";
	outfile = fopen(filename.c_str(), "w");

	auto start = std::chrono::high_resolution_clock::now();
	// ****************************************************
	int id_package;
	int playerID;
	// int (*mapStat)[MAXGRID] = new int[MAXGRID][MAXGRID];
	// int (*lastMapStat)[MAXGRID] = new int[MAXGRID][MAXGRID];
	int mapStat[MAXGRID][MAXGRID];
	int sheepStat[MAXGRID][MAXGRID];
	std::vector<std::vector<sheepBlock>> sheepBlocks(5, std::vector<sheepBlock>());
	UnionFind unionFind(MAXGRID * MAXGRID);

	GetMap(id_package, playerID, mapStat);

	std::vector<std::vector<int>> mapStatVector(MAXGRID, std::vector<int>(MAXGRID));
	std::vector<std::vector<int>> lastMapStatVector(MAXGRID, std::vector<int>(MAXGRID));
	std::vector<std::vector<int>> sheepStatVector(MAXGRID, std::vector<int>(MAXGRID));
	mapStatToVector(mapStat, mapStatVector);
	mapStatToVector(mapStat, lastMapStatVector);
	mapStatToVector(sheepStat, sheepStatVector);
	
	std::vector<int> init_pos = InitPos(playerID, mapStatVector);
	SendInitPos(id_package,init_pos);

	while (true){
		if(GetBoard(id_package, mapStat, sheepStat)) break;
		mapStatToVector(mapStat, mapStatVector);
		mapStatToVector(sheepStat, sheepStatVector);
		std::vector<NewMapBlock> newMapBlocks = compareChangesInMapState(mapStatVector, lastMapStatVector);
		for(auto &newMapBlock : newMapBlocks){
			sheepBlocks[newMapBlock.playerID].emplace_back(std::make_pair(newMapBlock.x, newMapBlock.y));
			unionFind.expandUnionFind(newMapBlock.x, newMapBlock.y, newMapBlock.playerID, mapStatVector);
		}
		std::vector<int> step = GetStep(playerID, mapStatVector, sheepStatVector, newMapBlocks, unionFind, sheepBlocks);
		SendStep(id_package, step);
	}
	// ****************************************************
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	printf("Time: %f\n", duration.count() / 1000.0);
	fclose(outfile);
}
