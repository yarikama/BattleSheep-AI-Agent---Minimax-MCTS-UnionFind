
#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstdint>

#define MAXGRID 12

// 移動方式
struct Move{
	int8_t x;
	int8_t y;
	int8_t subSheepNumber;
	int8_t direction;
};

// 版面資訊：棋盤狀態、羊群分布狀態
class GameState{
	private:
		int8_t playerID;
		int8_t mapState[MAXGRID][MAXGRID];
		int8_t sheepState[MAXGRID][MAXGRID];
	public:
		GameState(int8_t playerID, int8_t mapState[MAXGRID][MAXGRID], int8_t sheepState[MAXGRID][MAXGRID]){
			this->playerID = playerID;
			memcpy(this->mapState, mapState, sizeof(mapState));
			memcpy(this->sheepState, sheepState, sizeof(sheepState));
		}
		int8_t getPlayerID() return this->playerID;
		int8_t getMapState() return this->mapState;
		int8_t getSheepState() return this->sheepState;
		std::vector<Move> getMoves();
}

GameState::std::vector<Move> getMoves(){
	std::vector<Move> moves;
	// 這裡要寫出所有可能的移動方式
	return moves;
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
