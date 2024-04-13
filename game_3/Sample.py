import STcpClient
import numpy as np
import random
import pickle
import torch
from train import ActorCritic
import os
import sys


def source_path(relative_path):
    if getattr(sys, 'frozen', False):
        base_path = sys._MEIPASS        
    else:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)


cd = source_path('')
os.chdir(cd)

# parameters definition
MAXGRID = 12
weightSurroundLen = 1
weightEmptyNum = 2
rewardOpponentNear = 10
rewardOpponentFar = 5

# 8個方向 
dx = [0, -1, 0, 1, -1, 0, 1, -1, 0, 1]
dy = [0, -1, -1, -1, 0, 0, 0, 1, 1, 1]
# 4個方向
dxx = [-1, 0, 1, 0]
dyy = [0, 1, 0, -1]

def isPositionValid(x, y):
    return 0 <= x < MAXGRID and 0 <= y < MAXGRID
def isPositionValidForOccupying(x, y, mapStat):
    return isPositionValid(x, y) and mapStat[x][y] == 0
def scorePosition(x, y, mapStat):
    scoreEmptyNum = 0
    scoreSurroundNum = 0
    for i in range(-2, 3):
        for j in range(-2, 3):
            if i == 0 and j == 0:
                continue
            if isPositionValidForOccupying(x + i, y + j, mapStat):
                scoreEmptyNum += 1
            if isPositionValid(x + i, y + j) and mapStat[x+i][y+j] > 0:
                scoreEmptyNum -= 2

    for i in range(1, 10):
        if i == 5:
            continue
        newX, newY = x + dx[i], y + dy[i]
        if not isPositionValidForOccupying(newX, newY, mapStat):
            continue
        lenSurround = 0
        while isPositionValidForOccupying(newX, newY, mapStat):
            lenSurround += 1
            newX += dx[i]
            newY += dy[i]
        scoreSurroundNum += pow(lenSurround, weightSurroundLen)

    return scoreEmptyNum, scoreSurroundNum
def scoreOpponentDistance(x, y, mapStat, playerID):
    opponentDistance = float('inf')
    haveOpponent = False
    for iy in range(MAXGRID):
        for ix in range(MAXGRID):
            if mapStat[ix][iy] != playerID and mapStat[ix][iy] > 0:
                haveOpponent = True
                distance = max(abs(ix - x), abs(iy - y))
                opponentDistance = min(opponentDistance, distance)
    if not haveOpponent:
        return -1
    return opponentDistance
'''
    選擇起始位置
    選擇範圍僅限場地邊緣(至少一個方向為牆)
    
    return: init_pos
    init_pos=[x,y],代表起始位置
    
'''
def InitPos(mapStat):
    init_pos = [0, 0]
    maxScore = 0
    for y in range(MAXGRID):
        for x in range(MAXGRID):
            if mapStat[x][y] != 0:
                continue
            isEdge = False
            for i in range(4):
                newX, newY = x + dxx[i], y + dyy[i]
                if not isPositionValid(newX, newY):
                    continue
                if mapStat[newX][newY] == -1:
                    isEdge = True
            scoreEmptyNum, scoreSurroundNum = scorePosition(x, y, mapStat)
            opponentDistance = scoreOpponentDistance(x, y, mapStat, playerID)
            if not isEdge:
                continue
            score = weightEmptyNum * scoreEmptyNum - weightSurroundLen * scoreSurroundNum / 8
            if 0 <= opponentDistance <= 2:
                score -= rewardOpponentNear
            elif opponentDistance >= 6:
                score += rewardOpponentFar
            if score > maxScore:
                maxScore = score
                init_pos[0] = x
                init_pos[1] = y
    return init_pos
def dfs(sheepStat, visited, x, y):
    if not (0 <= x < MAXGRID and 0 <= y < MAXGRID) or visited[x][y] or sheepStat[x][y] == 0:
        return 0
    visited[x][y] = True
    area = 1
    for i in range(4):
        area += dfs(sheepStat, visited, x + dxx[i], y + dyy[i])
    return area

def evaluate(sheepStat):
    visited = np.zeros((MAXGRID, MAXGRID), dtype=bool)
    score = 0

    for i in range(MAXGRID):
        for j in range(MAXGRID):
            if not visited[i][j] and sheepStat[i][j] > 0:
                area = dfs(sheepStat, visited, i, j)
                score += pow(area, 1.25)

    return score
def evaluateNextReward(mapStat, sheepStat, step):
    reward = 0
    x, y = step[0]
    m = step[1]
    dir = step[2]
    xMove = x + dx[dir]
    yMove = y + dy[dir]
    while mapStat[xMove][yMove] == 0:
        xMove += dx[dir]
        yMove += dy[dir]

    sheepStat[x][y] -= m
    sheepStat[xMove][yMove] += m
    reward = evaluate(sheepStat)
    return reward
'''
    產出指令
    
    input: 
    playerID: 你在此局遊戲中的角色(1~4)
    mapStat : 棋盤狀態(list of list), 為 12*12矩陣, 
              0=可移動區域, -1=障礙, 1~4為玩家1~4佔領區域
    sheepStat : 羊群分布狀態, 範圍在0~16, 為 12*12矩陣

    return Step
    Step : 3 elements, [(x,y), m, dir]
            x, y 表示要進行動作的座標 
            m = 要切割成第二群的羊群數量
            dir = 移動方向(1~9),對應方向如下圖所示
            1 2 3
            4 X 6
            7 8 9
'''
def GetStep(playerID, mapStat, sheepStat):
    step = [(0, 0), 0, 1]
    print("here")
    reward = evaluate(sheepStat)

    state = torch.from_numpy(np.concatenate((mapStat.flatten(), sheepStat.flatten()))).float()
    with torch.no_grad():
        x_coord, y_coord, m_split, dir_probs, value = model(state)
        # 轉回原始機率並取值
        x = torch.multinomial(torch.exp(x_coord), 1).item()
        y = torch.multinomial(torch.exp(y_coord), 1).item()
        m = torch.multinomial(torch.exp(m_split), 1).item() + 1
        dir = torch.multinomial(torch.exp(dir_probs), 1).item() + 1
        step = [(x, y), m, dir]
        # 記錄對數機率
        x_logprobs.append(x_coord[x-1].item())
        y_logprobs.append(y_coord[y-1].item())
        m_logprobs.append(m_split[m-1].item())
        dir_logprobs.append(dir_probs[dir-1].item())
        values.append(value.item())
        # 判斷選擇的動作是否有效
        if mapStat[x][y] != playerID or sheepStat[x][y] <= m:
            # 無效動作,給予負的reward
            next_reward = reward - 10
        else:
            # 有效動作,計算下一步的reward
            next_reward = evaluateNextReward(mapStat, sheepStat, step)
        rewards.append(next_reward - reward)
    
    return step




# player initial
(id_package, playerID, mapStat) = STcpClient.GetMap()
init_pos = InitPos(mapStat)
STcpClient.SendInitPos(id_package, init_pos)

# Load the trained model parameters
model = ActorCritic()
with open('model.pkl', 'rb') as f:
    model.load_state_dict(pickle.load(f))
model.eval()
# loss function parameters
x_logprobs = []
y_logprobs = []
m_logprobs = []
dir_logprobs = []
values = []
rewards = []

# start game
while (True):
    (end_program, id_package, mapStat, sheepStat) = STcpClient.GetBoard()
    sheepStat = np.where(mapStat == playerID, sheepStat, 0) # hide other player's sheep number
    # hide other player's sheep number
    if end_program:
        STcpClient._StopConnect()
        break
    Step = GetStep(playerID, mapStat, sheepStat)

    STcpClient.SendStep(id_package, Step)
# DON'T MODIFY ANYTHING IN THIS WHILE LOOP OR YOU WILL GET 0 POINT IN THIS QUESTION

# 儲存loss function參數
param = {
    'x_logprob': x_logprobs,
    'y_logprob': y_logprobs,
    'm_logprob': m_logprobs,
    'dir_logprob': dir_logprobs,
    'value': values,
    'reward': rewards
}
with open('param.pkl', 'wb') as f: 
    pickle.dump(param, f)