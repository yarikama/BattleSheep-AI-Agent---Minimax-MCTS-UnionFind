
#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <cstdint>

#define MAXGRID 15
#define powSurroundLen 1.5
#define weightSurroundLen 1.5
#define weightEmpty 2

// 8個方向 or 4個方向
int8_t dx[] = {0, -1, 0, 1, -1, 0, 1, -1, 0, 1};
int8_t dy[] = {0, 1, 1, 1, 0, 0, 0, -1, -1, -1};
int8_t dxx[] = {-1, 0, 1, 0};
int8_t dyy[] = {0, 1, 0, -1};

class GameInit{
	private:
		int mapState[MAXGRID][MAXGRID];
	public:
		GameInit(int mapStat[MAXGRID][MAXGRID]){
			memcpy(this->mapState, mapStat, sizeof(int) * MAXGRID * MAXGRID);
		}
		std::vector<int> getPosition();
};

std::vector<int> GameInit::getPosition(){
	printf("my gamestate\n");
	for(int y = 0; y < MAXGRID; y++){
		for(int x = 0; x < MAXGRID; x++){
			printf("%d ", this->mapState[x][y]);
		}
		printf("\n\n");
	}
	std::vector<int> position;
	int maxScore = 0;
	for(int y = 0; y < MAXGRID; y++){
		for(int x = 0; x < MAXGRID; x++){
			//檢查是否為障礙或其他player
			if(this->mapState[x][y] != 0) continue;
			//檢查是否為場地邊緣
			bool isEdge = false;
			for(int i = 0 ; i < 4 ; i++){
				int8_t newX = x + dxx[i], newY = y + dyy[i];
				if(newX < 0 || newX >= MAXGRID || newY < 0 || newY >= MAXGRID) continue;
				if(this->mapState[newX][newY] == -1) isEdge = true;
			}
			//周圍空格數、周圍延伸距離
			int numEmpty = 0;
			double numSurround = 0;
			for(int i = 1 ; i <= 9 && i != 5 ; i++){
				int newX = x + dx[i], newY = y + dy[i];
				if(newX < 0 || newX >= MAXGRID || newY < 0 || newY >= MAXGRID) continue;
				if(this->mapState[newX][newY] == 0) numEmpty++;
				//周圍延伸距離
				int lenSurround = 0;
				while(newY >= 0 && newY < MAXGRID && newX >= 0 && newX < MAXGRID){
					if(this->mapState[newX][newY] != 0) break;
					lenSurround++;
					newX += dx[i];
					newY += dy[i];
				}
				numSurround = numSurround + pow(lenSurround, powSurroundLen);
			}
			//限定場地邊緣
			if(isEdge == false) continue;
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
