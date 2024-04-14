import subprocess
import re
import os
import signal

num_runs = 5

player_ranks = {i: [0] * 4 for i in range(1, 5)}

for i in range(num_runs):
    print(f"Running game {i + 1}/{num_runs}")

    process = subprocess.Popen(['./AI_game.exe'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        stdout, stderr = process.communicate(timeout=10) # 設置10秒超時
    except subprocess.TimeoutExpired:
        # 發送SIGINT訊號(模擬Control+C)
        os.kill(process.pid, signal.SIGINT)
        stdout, stderr = process.communicate() # 獲取殘留輸出

    output = stdout.decode('cp950')

    scores = re.findall(r'player (\d+)=team \d+ : (\[\d.\]+)', output)
    print("Extracted scores:")
    for player, score in scores:
        print(f"Player {player}: {score}")

    scores = [(int(player), float(score)) for player, score in scores]
    scores.sort(key=lambda x: x[1], reverse=True)

    for rank, (player, _) in enumerate(scores):
        player_ranks[player][rank] += 1

for player, ranks in player_ranks.items():
    print(f"Player {player}:")
    for rank, count in enumerate(ranks, start=1):
        print(f" Rank {rank}: {count}/{num_runs} ({count / num_runs * 100:.2f}%)")