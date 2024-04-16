#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include "game_data.h"


#define MAXGRID 12
#define powSurroundLen 1.5
#define weightSurroundLen 1
#define weightEmptyNum 2
#define rewardOpponentNear 10
#define rewardOpponentFar 5

// 模型
ActorCritic model;
// 8個方向 
constexpr int dx[] = { 0, -1, 0, 1, -1, 0, 1, -1, 0, 1 };
constexpr int dy[] = { 0, -1, -1, -1, 0, 0, 0, 1, 1, 1 };
// 4個方向
constexpr int dxx[] = { -1, 0, 1, 0 };
constexpr int dyy[] = { 0, 1, 0, -1 };
// 判斷位置所屬及合法
inline bool isPositionValid(int x, int y) { return x >= 0 && x < MAXGRID && y >= 0 && y < MAXGRID; }
inline bool isPositionValidForOccupying(int x, int y, int mapState[MAXGRID][MAXGRID]) { return isPositionValid(x, y) && mapState[x][y] == 0; }


void scorePosition(int x, int y, int mapStat[MAXGRID][MAXGRID], int& scoreEmptyNum, double& scoreSurroundNum) {
    scoreEmptyNum = 0;
    scoreSurroundNum = 0;

    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            if (i == 0 && j == 0) continue;
            if (isPositionValidForOccupying(x + i, y + j, mapStat)) scoreEmptyNum++;
            if (isPositionValid(x + i, y + j) && mapStat[x + i][y + j] > 0) scoreEmptyNum -= 2;
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
    if (!haveOpponent) return -1;
    return opponentDistance;
}
/*
    選擇起始位置
    選擇範圍僅限場地邊緣(至少一個方向為牆)

    return: init_pos
    init_pos=<x,y>,代表你要選擇的起始位置

*/
std::vector<int> InitPos(int playerID, int mapStat[12][12]) {
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
            }
            else if (opponentDistance >= 6) {
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


// DFS 算連通面積，不可以走過，並且要是自己地盤或是空地
int dfs(int sheepStat[12][12], std::vector<std::vector<bool>>& visited, int x, int y) {
    if (!(0 <= x && x < MAXGRID && 0 <= y && y < MAXGRID) || visited[x][y] || sheepStat[x][y] == 0) {
        return 0;
    }
    visited[x][y] = true;
    int area = 1;
    for (int i = 0; i < 4; ++i) {
        area += dfs(sheepStat, visited, x + dxx[i], y + dyy[i]);
    }
    return area;
}
float evaluateArea(int sheepStat[12][12]) {
    std::vector<std::vector<bool>> visited(MAXGRID, std::vector<bool>(MAXGRID, false));
    int score = 0;

    for (int i = 0; i < MAXGRID; ++i) {
        for (int j = 0; j < MAXGRID; ++j) {
            if (!visited[i][j] && sheepStat[i][j] > 0) {
                int area = dfs(sheepStat, visited, i, j);
                score += std::pow(area, 1.25);
            }
        }
    }
    return score;
}
float evaluateNextReward(int mapStat[12][12], int sheepStat[12][12], std::vector<int>& step) {
    float reward = 0;
    int x = step[0];
    int y = step[1];
    int m = step[2];
    int dir = step[3];
    int xMove = x + dx[dir];
    int yMove = y + dy[dir];
    while (mapStat[xMove][yMove] == 0) {
        xMove += dx[dir];
        yMove += dy[dir];
    }

    sheepStat[x][y] -= m;
    sheepStat[xMove][yMove] += m;
    reward = evaluateArea(sheepStat);
    return reward;
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
std::vector<int> GetStep(int playerID, int mapStat[12][12], int sheepStat[12][12]) {
    std::vector<int> step;
    step.resize(4);
    float rewards = evaluateArea(sheepStat);

    auto mapStatTensor = torch::from_blob(mapStat, { MAXGRID, MAXGRID }, torch::kInt).clone();
    auto sheepStatTensor = torch::from_blob(sheepStat, { MAXGRID, MAXGRID }, torch::kInt).clone();
    mapStatTensor = mapStatTensor.view({ -1 });
    sheepStatTensor = sheepStatTensor.view({ -1 });
    auto state = torch::cat({ mapStatTensor, sheepStatTensor }, 0).view({ 1, -1 }).to(torch::kFloat);

    auto [x_coord, y_coord, m_split, dir_probs, values] = model->forward(state);

    // 將張量轉換為一維張量
    x_coord = x_coord.view(-1);
    y_coord = y_coord.view(-1);
    m_split = m_split.view(-1);
    dir_probs = dir_probs.view(-1);

    // 轉回原始機率並取值
    int x = torch::multinomial(torch::exp(x_coord), 1).item<int>();
    int y = torch::multinomial(torch::exp(y_coord), 1).item<int>();
    int m = torch::multinomial(torch::exp(m_split), 1).item<int>() + 1;
    int dir = torch::multinomial(torch::exp(dir_probs), 1).item<int>() + 1;
    step[0] = x; step[1] = y; step[2] = m; step[3] = dir;

    // std::cout << "x_coord: " << x_coord << std::endl;
    std::cout << "x: " << x << std::endl;
    std::cout << "y: " << y << std::endl;
    std::cout << "m: " << m << std::endl;
    std::cout << "dir: " << dir << std::endl;

    // 記錄對數機率
    x_logprob.push_back(torch::exp(x_coord)[x].item<float>());
    y_logprob.push_back(torch::exp(y_coord)[y].item<float>());
    m_logprob.push_back(torch::exp(m_split)[m - 1].item<float>());
    dir_logprob.push_back(torch::exp(dir_probs)[dir - 1].item<float>());
    value.push_back(values.item<float>());

    // 判斷選擇的動作是否有效
    float next_reward;
    if (mapStat[x][y] != playerID || sheepStat[x][y] <= m) {
        // 無效動作,給予負的reward
        next_reward = rewards - 10;
    }
    else {
        // 有效動作,計算下一步的reward
        next_reward = evaluateNextReward(mapStat, sheepStat, step);
    }
    reward.push_back(next_reward - rewards);

    return step;
}

int main() {
    int id_package;
    int playerID;
    int mapStat[12][12];
    int sheepStat[12][12];

    // player initial
    GetMap(id_package, playerID, mapStat);
    std::vector<int> init_pos = InitPos(playerID, mapStat);
    SendInitPos(id_package, init_pos);

    // Load the trained model parameters
    torch::load(model, "model.pkl");
    model->eval();

    while (true)
    {
        if (GetBoard(id_package, mapStat, sheepStat))
            break;
        // hide other player's sheep number start
        for (int i = 0; i < 12; i++)
            for (int j = 0; j < 12; j++)
                if (mapStat[i][j] != playerID)
                    sheepStat[i][j] = 0;
        // hide other player's sheep number end 
        std::vector<int> step = GetStep(playerID, mapStat, sheepStat);
        SendStep(id_package, step);
    }
    // DON'T MODIFY ANYTHING IN THIS WHILE LOOP OR YOU WILL GET 0 POINT IN THIS QUESTION
}



// 執行程式: Ctrl + F5 或 [偵錯] > [啟動但不偵錯] 功能表
// 偵錯程式: F5 或 [偵錯] > [啟動偵錯] 功能表

// 開始使用的提示: 
//   1. 使用 [方案總管] 視窗，新增/管理檔案
//   2. 使用 [Team Explorer] 視窗，連線到原始檔控制
//   3. 使用 [輸出] 視窗，參閱組建輸出與其他訊息
//   4. 使用 [錯誤清單] 視窗，檢視錯誤
//   5. 前往 [專案] > [新增項目]，建立新的程式碼檔案，或是前往 [專案] > [新增現有項目]，將現有程式碼檔案新增至專案
//   6. 之後要再次開啟此專案時，請前往 [檔案] > [開啟] > [專案]，然後選取 .sln 檔案
