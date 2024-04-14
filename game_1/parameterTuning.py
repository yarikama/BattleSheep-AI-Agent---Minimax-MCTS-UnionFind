import subprocess
import re
import time
import os

num_runs = 5  # 執行次數

# 初始化統計結果
player_ranks = {i: [0] * 4 for i in range(1, 5)}

for i in range(num_runs):
    print(f"Running game {i + 1}/{num_runs}")
    
    # 執行遊戲並捕獲輸出
    process = subprocess.Popen(['.\AI_game.exe'], shell=True)
    time.sleep(25)
    subprocess.call(['taskkill', '/F', '/T', '/PID', str(process.pid)])
    
    # 從 result.txt 文件中讀取輸出
    with open('stderr.txt', 'r') as file:
        output = file.read()
    
    # 從輸出中提取分數資訊
    scores = re.findall(r'player (\d+)=team \d+ : (\[\d.\]+)', output)
    print("Extracted scores:")
    for player, score in scores:
        print(f"Player {player}: {score}")
    
    # 將分數轉換為浮點數並排序
    scores = [(int(player), float(score)) for player, score in scores]
    scores.sort(key=lambda x: x[1], reverse=True)
    
    # 更新統計結果
    for rank, (player, _) in enumerate(scores):
        player_ranks[player][rank] += 1
    
    # 等待 1 秒再進行下一次迭代
    time.sleep(1)

# 輸出統計結果
for player, ranks in player_ranks.items():
    print(f"Player {player}:")
    for rank, count in enumerate(ranks, start=1):
        print(f" Rank {rank}: {count}/{num_runs} ({count / num_runs * 100:.2f}%)")