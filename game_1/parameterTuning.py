import subprocess
import pyautogui
import time
import re

output_file = "AI_game_output.txt"
summary_file = "AI_game_summary.txt"
num_games = 20
num_players = 4

# 初始化記錄每個玩家獲得各名次的次數
rank_counts = [[0] * num_players for _ in range(num_players)]

for i in range(num_games):
    # 執行 AI_game.exe 並將標準輸出重定向到檔案
    print(f"Running game {i + 1}/{num_games}")
    with open(output_file, "w") as file:
        exe_process = subprocess.Popen(["AI_game.exe"], stdout=file, stdin=subprocess.PIPE)

    # 等待Battle Sheep視窗出現
    while True:
        battle_sheep_window = pyautogui.getWindowsWithTitle("battle sheep")
        if not battle_sheep_window:
            time.sleep(1)
        else:
            window = battle_sheep_window[0]
            window_x, window_y = window.left, window.top
            # 將焦點移動到Battle Sheep視窗
            pyautogui.click(window_x + 10, window_y + 10)
            # 發送Alt+F4關閉視窗
            pyautogui.hotkey('alt', 'f4')
            break

    exe_process.stdin.write('q'.encode('utf-8'))
    exe_process.stdin.flush()

    # 讀取輸出檔案的內容
    with open(output_file, "r") as file:
        output = file.read()

    # 使用正則表達式提取玩家分數
    scores = re.findall(r'player \d+=team \d+ : ([\d.]+)', output)
    # 將分數轉換為浮點數
    scores = [float(score) for score in scores]
    # 根據分數排序並生成名次
    sorted_scores = sorted(scores, reverse=True)
    ranks = [sorted_scores.index(score) + 1 for score in scores]

    # 更新每個玩家獲得各名次的次數
    for player, rank in enumerate(ranks):
        rank_counts[player][rank - 1] += 1

    time.sleep(1.5)

    # 計算每個玩家獲得各名次的比率
    with open(summary_file, "w") as file:
        file.write(f"Summary of {num_games} games:\n")
        for player in range(num_players):
            file.write(f"Player {player + 1}:\n")
            for rank in range(num_players):
                count = rank_counts[player][rank]
                ratio = count / num_games
                file.write(f"  Rank {rank + 1}: {count} times, Ratio: {ratio:.2f}\n")
            file.write("\n")