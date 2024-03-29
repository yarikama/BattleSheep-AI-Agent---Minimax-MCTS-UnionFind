
#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <vector>

#define MAXGRID 15
#define powSurroundLen 1.5
#define weightSurroundLen1.5
#define weightEmpty 2

class GameInit{
	private:
		int8_t mapState[MAXGRID][MAXGRID];
	public:
		GameInit(int8_t mapState[MAXGRID][MAXGRID]){
			memcpy(this->mapState, mapState, sizeof(mapState));
		}
		std::vector<int> getPosition();
}

std::vector<int> GameInit::getPosition(){
	std::vector<int> position;
	int maxScore = 0;
	for(int y = 0; y < MAXGRID; y++){
		for(int x = 0; x < MAXGRID; x++){
			if(this->mapState[y][x] != 0) continue;
			int numEmpty = 0, numSurround = 0;
			for(int sy = -1 ; sy <= 1 ; sy++){
				for(int sx = -1 ; sx <= 1 ; sx++){
					if(sx == 0 && sy == 0) continue;
					if(y + sy < 0 || y + sy >= MAXGRID || x + sx < 0 || x + sx >= MAXGRID) continue;
					//周圍是否有空格
					if(this->mapState[y + sy][x + sx] != 0) continue;
					numEmpty++;
					//周圍延伸距離
					int newY = y + sy, newX = x + sx;
					int lenSurround = 0;
					while(newY >= 0 && newY < MAXGRID && newX >= 0 && newX < MAXGRID){
						if(this->mapState[newY][newX] != 0) break;
						lenSurround++;
						newY += sy;
						newX += sx;
					}
					numSurround = numSurround + pow(lenSurround, powSurroundLen);
				}
			}
			//目標周圍空格多、周圍延伸距離短
			int score = weightEmpty * numEmpty - weightSurroundLen * numSurround / 8;
			if(score > maxScore){
				maxScore = score;
				position.clear();
				position.push_back(x);
				position.push_back(y);
			}
		}
	}
	printf("position: %d %d\n", position[0], position[1]);

	return position;
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

	GameInit gameInit(mapStat);
    init_pos = gameInit.getPosition();
    
    return init_pos;
}

/*
	產出指令
    
    input: 
	playerID: 你在此局遊戲中的角色(1~4)
    mapStat : 棋盤狀態, 為 15*15, 
					0=可移動區域, -1=障礙, 1~4為玩家1~4佔領區域
    sheepStat : 羊群分布狀態, 範圍在0~32, 為 15*15矩陣

    return Step
    Step : <x,y,m,dir> 
            x, y 表示要進行動作的座標 
            m = 要切割成第二群的羊群數量
            dir = 移動方向(1~9),對應方向如下圖所示
            1 2 3
			4 X 6
			7 8 9
*/
std::vector<int> GetStep(int playerID,int mapStat[MAXGRID][MAXGRID], int sheepStat[MAXGRID][MAXGRID])
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
    int mapStat[15][15];
    int sheepStat[15][15];

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
