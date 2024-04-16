# AI Capstone Project #2

<aside>
⚙ **Team ID : 4**
**Team name : Oh Wonder**
**Team member :** **109704001 許恒睿、109704038 孫于涵**

</aside>

# **1. Agent 運作方式**

## 1.1 策略搜尋方式

我們的 AI agents 主要結合了 Minimax 和蒙地卡羅樹搜尋 (MCTS) 兩種搜尋演算法。Minimax 用於窮舉暴搜模擬遊戲的走向，並假設四個 Players 都會做出最優決策，來找出該回合的最佳行動。MCTS 則是基於統計的搜尋方法，透過不斷地隨機模擬遊戲過程，來估計每個決策的勝率，並選出最好的勝率的節點返回動作。

### 1.1.1 Minimax 和 MCTS 的使用時機

我們為兩種搜尋方式（Minimax、MCTS）制定最大深度，而每往下一層可以想像為某個玩家的回合做出了新決定。其中我們 AI agent 的評估順序為 Minimax 接到 MCTS，也就是當 Minimax 到達深度為 0 的葉節點後，才會再用 MCTS 去做不同評估，因此整體的時間與空間複雜度會變得非常高 。

![圖片2.png](AI%20Capstone%20Project%20#2%20c0df89d5a25d431f83d7f13c78d805ef/%25E5%259C%2596%25E7%2589%25872.png)

### 1.1.2 Minimax

在我們的程式碼當中 Minimax 實現在`class GameState` 中，以下是簡易的演算法說明：

```pascal
終止條件1：
if 該玩家沒有可以動的動作：
		true: 返回版面分數  -註1
		
終止條件2:
if minimax 已達到我們制定的最大深度：
		true: MCTS 以此狀態繼續模擬  

為每個可能動作對分數進行排序 (讓alpha beta pruning可以更快)  -註2

if 當前玩家 == 我：
		true ：實現所有可達成的動作（newState），從此動作去做新的 minimax，得到節點分數後取最高分數，和更新 alpha 的值，而若 alpha >= beta 則停止。 -註3
		false：實現所有可達成的動作（newState），從此動作去做新的 minimax，得到節點分數後取最低分數，和更新  beta 的值，而若 alpha >= beta 則停止。
----------------------------------------------------------------------------------------------------------------------------------------
-註1：這邊會因為某個玩家 pass 停止估算，實際情況應該要是所有玩家都不能動的時候才停止，但是在我們測試的時候結果分數差異不大，時間卻會差很多，所以選擇提早中止。
-註2：這邊會其實應該要是輪到當前玩家時，排序方向跟其他玩家要相反，也就是最大的在前面，但是我們沒有考量到這點。
-註3：這邊其實不是所有動作，假設目前該位置上有 16 隻羊 8 個方向都能動的話就會有 15 * 8 種版面，每個版面都進行 minimax，時間會超時，所以我們採取的方式是自己
		 制定方法算出 sheepNumber（15->1），所以其實最多也只有 8 種可能而已。
```

### 1.1.3 MCTS

在我們的程式碼當中 MCTS 實現在 `class MCTSNode` 和 `class MCTS` 中，以下是簡易的演算法說明：

```pascal
終止條件1：
if 沒有可以動的動作了 and 有子節點 and 深度還未超過設定深度
		true: 從 children 中挑出 UCB 分數最高的子節點，使這個子節點去做下一層的 MCTS
		
終止條件2:
if 還有可以動的動作 and 深度還未超過設定深度：
		true: 隨機從現在的可動動作選擇變成新的 state，放進 children 中。之後進行 rollout 讓它玩到遊戲結束，得出版面分數之後，更新它和它的所有父節點分數（backpropogation）。

終止條件3:
if 深度超過：
		true: 返回版面分數
```

## 1.2 初始位置選擇方式

這裡實現在`InitPos()`，初始位置選擇的目標為腹地大，有許多可以擴展佔領的區域，並且距離其他 Players 不要太近，以避免對手擴展時影響到自己。因此我們考量三個項目：可移動數量、可延伸距離、最近對手距離。

### 1.2.1 **可移動分數**

考慮 X 與 Y 座標距離 3 單位的範圍內，若是可移動格子則加分，若是其他 Players 則扣分，以此方式來得到可移動總分。

### 1.2.2 **可延伸距離**

考慮8個延伸方向，計算每個方向最遠延伸距離再總和，以此來評估可延伸與擴展的能力。

### 1.2.2 **最近對手距離**

找出一個最近的對手距離，當距離過近時則扣分，距離大於一定數值時則加分，而這裡距離遠距的標準依各遊戲設定不同，會在後面的項目說明詳細差異。

根據以上項目的計算與加權，選擇具有擴展潛能，且距離對手適當範圍的格子作為初始位置，可以提升最後達到更好佔領面積分數的機會，來幫助遊戲獲勝。

## **1. 3 可移動 Step 的列舉方式**

這裡實現在`GetWhereToMove()`，前面提到我們 AI Agents 主要以 Minimax 與 MCTS 作為搜尋方式，以下是我們用於四個 Agents 搜尋的 Step 元素 [(x,y), m, dir] 設定方式。

### 1.3.1 動作進行座標與移動方向

我們將各個 player 的羊群占有位置座標存放於 sheepBlock，每回合 Server 回傳 mapStat 時會遍歷一次地圖將新增的位置座標放入 sheepBlock。因此當 Minimax 與 MCTS 需要窮舉可移動的 Step 元素時，不需要不斷重複遍歷地圖來尋找各個 player 的羊群占有位置。而對於每個可移動的方向，會根據 mapStat 是否為可佔據位置，來找出分別可以到達的最遠的位置。

### 1.3.2 羊群切割數量

我們決定羊群切割數量取決於三個項目，**連通面積、阻礙數量、阻礙高度總和**，而這裡的阻礙包含障礙與其他玩家。每個項目會各有一個羊群切割比例，最後再調整三項的權重得到最終切割數量。

![圖片1.png](AI%20Capstone%20Project%20#2%20c0df89d5a25d431f83d7f13c78d805ef/%25E5%259C%2596%25E7%2589%25871.png)

1. **連通面積：**由於羊群擴展的目的是希望最後能得到較好的連通面積分數，若目標位置連通面積越高時會分割越多數量，使得有足夠的羊群能夠相連的擴展。遊戲設定將以X與Y座標距離2單位的範圍內來考慮。左圖以周圍九宮格範圍為例，動作進行位置 X 與可移動位置 XMove 的連通面積比例為3:4，則此項目顯示出分割57.1%數量羊群給目標位置。
2. **阻礙數量：**當可移動位置附近的阻礙較多時，將會考慮切割較少的數量。這裡我們考量扣掉移動進出的方向後，最多會有7個方向有阻礙，切割數量則根據兩者的阻礙數量比例來決定。因此當動作進行位置 X 與可移動位置 XMove 附近的狀態（如左圖），周圍阻礙數量同為 3 個，則此項目顯示出分割 50% 數量羊群給目標位置。
3. **阻礙高度總和：**除了切割數量外，其他 Player 的羊群數量仍有機會妨礙到擴展羊群佔據面積的表現。因此，此項目考慮了阻礙高度總和，計算方式各羊群高度即為羊群數量，障礙的高度則以 1 為計，當阻礙高度總和較多時，切割較少數量。以右圖為例，動作進行位置 X 與目標位置 XMove 附近的阻礙高度總和為 4:5，則此項目顯示出分割 55.5% 數量羊群給目標位置

得到以上三項個別的分割比例後，會在依四個 Agents 的特性，將此三個項目加權得到最終的羊群分割數量。另外我們也有考慮分割數量一定最少為 1，最多不等於動作進行位置原先有的羊群數量，以符合遊戲進行規則。

## 1.4 版面評分方式

評分方式至關重要，這邊我們 `evalutionByUnionFind()` 實現，尤其不只影響 Minimax 和 MCTS 的成效，同時也跟時間複雜度或是考量的關係十分有關。我們使用連通面積加總的方式與 Spec 一樣，使用 UnionFind 得出（下面說明），但是在 exponentConstant 這邊的設定會依照不同遊戲設定改變大小，在我們的假設中，當這裡的常數值越大，考量面積的連通性程度就會越大，若是過大，Minimax 反而會做出犧牲在新區塊下棋而喪失可能性，所以這邊都調在 1.25 ~ 2 之間。

我們的評分方式排序：

### 1.4.1 排名

首先我們會先追求排名（算法跟 Spec 一樣，使用連通面積的 1.25 次方排序）。原因是在實驗中，我們發現若是只追求面積最大化，就會喪失限制對手發育面積的能力，面積平均可能較大，但是排名可能反而降低。此外我們這邊是將排名倒過來作為評分，像是第一名就會是 4 分、第二名就是 3 分… 

### 1.4.2 面積

同時我們利用也將面積考量進去，但是權重會設很小，原因考量到面積能加快 Minimax 的 alpha beta pruning。可以想像若是每個玩家都擺放下一步時，排名很有可能不會有什麼變化，此時 Minimax 就沒辦法塞選掉同排名但是面積較小的選擇。另外，我們在實驗的過程中發現一旦 Minimax 目前都預測到差不多排名，它就會隨意擺放位置，不會追求面積的最大化，這樣一來若是考量深度較潛，就會犧牲掉很多可能性。 

## 1. 5 最佳化時間複雜度

這次 Project 最重要的因素在於要在時間內下好離手，而我們使用 Minimax + MCTS ，為了能讓兩者有更好的搜尋時間同時避免 Timeout，所以我們花費很大心力在如何最佳化時間複雜度：

### 1.5.1 更新地圖

除了 Game 3 以外，這邊更新地圖是採取對比上一次更新地圖之不一樣的 Blocks 在哪，而按照遊戲規則，每次更新地圖（`GetMap`）都會更新四個 Blocks ，在這些新的 Blocks，去更新我們已有的 Data Structure，如：`MapState`, `SheepState`, `UnionFind`, `SheepBlocks`…，如此一來，我們可以減低複製的成本，而這邊指的並不是 `mapState` 或是 `sheepState` 的複製成本，除非我們修改 `STchClient.h` ，指的是像是 `UnionFind`, `SheepBlocks` 等，原因是我們只要加入更新的 Blocks 就好，這樣不用刪除再複製相依在`mapState` 或是`sheepState` 的 Data Structure，也不用因為 mapState 改變而重新複製。

### 1.5.2 SheepBlocks 的必要

`SheepBlocks` 為一個 2 維的 `std::vector` 儲存四個玩家全部羊群的分佈位置，如：玩家 2 在 (5, 3) 上面有羊，`SheepBlocks[2]` 中就會有個 element 為 `std::pair(5, 3)` 。這麼實現的原因在於說，如果每次要找某玩家可以動的位置時，我們不用歷遍整個地圖，從`SheepBlocks` 中去得到現在位置並求出可以動的位置就會更快。想像 `SheepBlocks` 就是 `MapState` 的稀疏矩陣即可。

### 1.5.3 連通面積

我們這邊依照不同情境使用兩種算連通面積的方式，一個是 BFS，另一個則是 UnionFind。

**BFS**
適用在小面積（如：九宮格）上面的估計，像是我們決定 SheepNumber 的時候會看可執行動作的 Block 周遭是否有敵人的 Blocks 來決定投注的羊群數量為多少時，使用 BFS 就好。

**UnionFind**
不論是使用 BFS 或 DFS 運行，每次評估**總體面積**分數時皆會花費較多的時間。因此，我們採取 Union Find 來維護連通塊，最佳化面積計算的過程。首先，UnionFind 是依賴在 MapState 上面的，會將地圖上的每個格子視為一個節點，若是兩個塊屬於同一個 Set 時，會使用 Union 將兩個塊連在一起，也就是將另一方的節點更新較大面積塊的根節點，並創立 Size 在根節點之上。另外更新 MapState 時也須更新 UnionFind，也就是對新增的 Block 看看四個方向上面是不是有同個玩家的 Block 若是的話則 Union 起來。Union 操作的平均時間複雜度接近 O(1)，若是將所有連通塊的根節點記住，可以在 **O(m)** 的時間複雜度，m為連通塊數目**，**內算出全部玩家的面積，以及面積排名。

### 1.5.4 指定羊群數量

前面提過詳細算法，這邊的羊群數量若是將所有可能值都作為一個 Move，全部列舉可能性再延伸算出 Minimax 會超過時限，因此這邊只用單一數量作為衡量基準。

### 1.5.5 Deep Copy or Shallow Copy?

![圖片5.png](AI%20Capstone%20Project%20#2%20c0df89d5a25d431f83d7f13c78d805ef/%25E5%259C%2596%25E7%2589%25875.png)

Deep Copy 要花的時間會比 Shallow Copy 多上許多，盡可能使用 Shallow Copy 會更好，但是在 minimax 以及 MCTS 模擬時，都需要生成新的 state，在層數變深我們採用 Deep Copy，而在 Server 傳回新的移動資訊時，這邊只用 Shallow Copy 只須更新變動的 Blocks。

# **2. 不同遊戲間的設定異同**

我們以 Agent 1 作為基底，來設定 Agent 2 ~ Agent 4。

## 2.1 Agent 1 & Agent 2

### 2.1.1 初始位置選擇

此部份實現在`InitPos()`，由於 Agent 1 的遊戲版面為12x12，而 Agent 2的遊戲版面為 15 x 15比較大，因此在計算最近對手距離的標準有所不同。在 Agent 1 中若最近對手距離小於 2 則評估總分扣分，若大於 6 則評估總分加分；而在 Agent 2 中則是若最近對手距離小於 4 則評估總分扣分，若大於 7 則評估總分加分。此標準訂定方式是希望對手距離版面大小的一半以上，對於擴展效果較佳。

## 2.2 Agent 3

由於 Game 3 無法看到 SheepState，我們會依照 mapState 設立一個假的 sheepState，實現方式是在 `GetStep()` 將  16 / mapState 的所有玩家的累積面積（不乘上次方）得到平均值之後四捨五入，我們假設敵方的羊群數量都是最平均，之後的實現方法都一樣。

```cpp
	// 建立假的 SheepState
	int (*newSheepStat)[MAXGRID] = new int[MAXGRID][MAXGRID];
	for(int i = 1 ; i <= 4 ; ++i){
		if(i == playerID){
			for(auto& block : sheepBlocks[i]){
				newSheepStat[block.first][block.second] = sheepStat[block.first][block.second];
			}
		}else{
			int sheepAverageNumber = std::round(16.0 / sheepBlocks[i].size());
			for(auto& block : sheepBlocks[i]){
				newSheepStat[block.first][block.second] = sheepAverageNumber;
			}
		}
	}
```

## 2.3 Agent 4

Agent 4 需要同時考量到 PlayerID % 4 + 2 的另一個夥伴，因此在這部份，我們針對 2 個部份版面評分方式和 Minimax 下去做改變：

### 2.3.1 評分方式改變

 此部份實現在`evalutionByUnionFind()` ，當四個玩家在對連通塊進行面積累加的時候，我們會為夥伴的面積一樣加上同樣的值，同時排名的值我們使用排名平方，讓 MCTS 在進行運算的 BestChild Value / Visit 的時候會有較大的差距（兩組別共兩名，第一名拿四分，第二名拿一分），同樣這邊我們也會將面積加權進去，只是面積權重更小：

```python
# Agent 1
playerArea[anyPlayerID] += pow(rootSize.size, exponentEvaluate);
# Agent 4
playerArea[anyPlayerID] += pow(rootSize.size, exponentEvaluate);
playerArea[(anyPlayerID % 4) + 2] += pow(rootSize.size, exponentEvaluate);
```

### 2.3.2 Minimax 改變

原本在 Agent 1 是採一大三小的方式，這邊則是一大一小的輪替方式（同組大、敵人組小）進行我們的 Minimax 模擬。

```python
# Agent 1
if(anyPlayerID == this->myPlayerID)){
        ....
		return maxEvaluation;
}else{
	      ....
		return minEvaluation;
}
# Agent 4
if(anyPlayerID == this->myPlayerID or anyPlayerID == ((this->myPlayerID % 4) + 2)){
        ....
		return maxEvaluation;
}else{
	      ....
		return minEvaluation;
}
```

# **3. 實驗與分析**

## 3.1 參數一覽

```cpp
#define MAXGRID 12                            --地圖的長寬
#define powSurroundLen 1.5                    --
#define weightSurroundLen 1                   --
#define weightEmptyNum 2                      --
#define rewardOpponentNear 13                 --
#define rewardOpponentFar 5                   --
#define weightArea 0.45                       --    
#define weightOpponentNum 0.28                --
#define weightOpponentSheep 0.27              --
#define exponentArea 1.5                      --算BFS連通面積時的次方
#define exponentEvaluate 1.8                  --算UnionFind連通面積時的次方
#define MCTSSIMULATIONS 60                    --MCTS模擬的次數
#define MCTSDEPTH 9                           --MCTS的中止深度
#define minimaxDepth 2                        --minimax的中止深度
#define isEveryPosibilityMinimax 0            --Minimax的sheepNumber是否要全部可能性，還是指定一個
#define isEveryPosibilityMCTS 0               --MCTS的sheepNumber是否要全部可能性，還是指定一個
#define weightRatioOfArea 0.005               --EvaluationByUnionFind面積的權重要佔評分中的多少
```

## 3.2 實驗環境

確保盡可能跟助教環境相同，我們使用：

```makefile
OS: Windows 11
CPU: 11th Gen Intel i5-11400 (12) @ 4.400GHz 
GPU: NVIDIA GeForce RTX 3060 Lite Hash Rate 
Memory: 16 GB 
```

## 3.3 評估時間與表現

我們分為兩種方式調參數，一開始是與助教給的 Sample[1-4].exe，在程式碼中打印時間，並觀察成效直到我們對參數的調法有感覺。之後，我們撰寫一份 script `parameterTuning[.py](http://parametertuning.py/)` 以及利用  `Makefile` 設定不同的參數讓我們自己的 Agent 互相打架數個 iteration，實驗看看誰的分數較高。這麼做的原因是因為考量到 minimax 中我們中止條件是當有玩家無法動作時，但若是對手較強大時，因為遊戲的延續性較強，所以運算的時間也會被拉得較長，所以讓自己的 agent 們互相比較，較能確保實際情況下不會發生 Timeout。

```cpp
# 時間
int main(){
	auto start = std::chrono::high_resolution_clock::now();
	// ****************************************************
	...
	// ****************************************************
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	printf("Time: %f\n", duration.count() / 1000.0);
}
```

```makefile
# parameter tuning example
CXX = g++
CXXFLAGS = -Wall -std=c++17 -O3
LDFLAGS = -lws2_32

PARAMS1 = -DexponentEvaluate=1.8 -DMCTSSIMULATIONS=90 -DMCTSDEPTH=9 -DminimaxDepth=2 -DTEAM_ID=1
PARAMS2 = -DexponentEvaluate=1.7 -DMCTSSIMULATIONS=90 -DMCTSDEPTH=9 -DminimaxDepth=2 -DTEAM_ID=2
PARAMS3 = -DexponentEvaluate=1.6 -DMCTSSIMULATIONS=90 -DMCTSDEPTH=9 -DminimaxDepth=2 -DTEAM_ID=3
PARAMS4 = -DexponentEvaluate=1.5 -DMCTSSIMULATIONS=90 -DMCTSDEPTH=9 -DminimaxDepth=2 -DTEAM_ID=4
```

## 3. 4 最終參數與表現間關係

四個 Agent 之間除了遊戲設計設定有所不同，也有一些共同擁有的參數。以下為其中 5 項重要參數比較，其中一個關鍵參數是 MCTS 與 Minimax 的搜尋深度。一般而言，搜尋深度越深，模擬的效果會越好，但也會花費更多時間。然而經過許多實驗嘗試後，我們發現 MCTS 與 Minimax 搜尋深度的總和對 AI 決策的效果有一定規律。當總深度再加一為 4 的倍數時，AI 的決策效果會明顯提升，同時節省更多時間，其餘參數相同。

|  | exponentEvaluate | MCTSSIMULATIONS | MCTSDEPTH | minimaxDepth | weightRatioOfArea |
| --- | --- | --- | --- | --- | --- |
| Agent 1 | 1.5 | 90 | 9 | 2 | 0.01 |
| Agent 2 | 1.8 | 60 | 10 | 1 | 0.02 |
| Agent 3 | 1.8 | 60 | 9 | 2 | 0.005 |
| Agent 4 | 1.8 | 90 | 9 | 2 | 0.01 |

# **4. 延伸討論**

## 4.1 改正 Minimax 中 Sort 按我方或是敵方玩家的方向

```makefile
    if(anyPlayerID == this->playerID){
	    std::sort(availableMoves.begin(), availableMoves.end(), [this, anyPlayerID](const Move& a, const Move& b) {
	        GameState stateA = this->applyMove(a, *this, anyPlayerID);
	        GameState stateB = this->applyMove(b, *this, anyPlayerID);
					return stateA.evaluateByUnionFind() > stateB.evaluateByUnionFind();
	    });
	  }else{
	    std::sort(availableMoves.begin(), availableMoves.end(), [this, anyPlayerID](const Move& a, const Move& b) {
	        GameState stateA = this->applyMove(a, *this, anyPlayerID);
	        GameState stateB = this->applyMove(b, *this, anyPlayerID);
					return stateA.evaluateByUnionFind() < stateB.evaluateByUnionFind();
	    });
	  }
```

## 4.2 羊群數量

可以塞入多一點羊群的可能評估方式，或是為依照隨機變數產出不同的羊群分數，增添多一點羊群的可能性。

## 4.3 Deep Q-Network

在設計 Agent 3 的方法時，原先我們有實作 Deep Q-Network，並以 Actor-Critic model 的方式來訓練增強式遊戲模型，然而後來考量此遊戲 AI 須以執行檔方式繳交作業並運作，因此在設計完成至訓練前改為一樣採取 Minimax 加上 MCTS 的方式進行，而以下會說明原先所設計的DQN模型。

### 4.3.1 模型架構設計

![DQN.png](AI%20Capstone%20Project%20#2%20c0df89d5a25d431f83d7f13c78d805ef/DQN.png)

以 Actor-Critic model 的方式，首先將mapSta和sheepStat變為長度288的向量輸入模型，經過兩層ReLU後，複製分割為Actor端和Critic端。Actor端功能為產生Step需要的數值包含X, Y, m, dir，由於希望從一般0~1之間的狹窄機率轉換成較寬廣的隊數值範圍，因此這裡是用Log-softmax轉換後，再根據其數值範圍設定輸出。Critic端則是通過ReLU後，在經過Tanh轉換為-1~1之間的數值來作為價值評分。

### 4.3.2 Loss Function 計算方式

**a. Actor Loss**

$$
\sum_{t=0}^{n} -\log(\pi(a_t|s_t)) \cdot (R_t - V^{\pi}(s_t))
$$

**b. Critic Loss**

$$
\sum_{t=0}^{n} (R_t - V(s_t))^2
$$

Actor Loss 的目標是最小化`-1 * 某狀態下所執行動作的對數畫機率 * (Rewad - Value)`。而Critic Loss 的目標是最小化`Rewad - Value`，也就是Advantage的值。最後 Loss 計算方式為 Actor Loss + clc * Critic Loss，這裡 clc 設為 0.1。

### 4.3.3 未來展望

由於這次已經把模型架構都設計出來了，但未能在適合的環境上訓練，我們也另外寫了一個遊戲模擬環境，因此會希望未來有機會能將這個模型實際訓練並逐步調整，看看能不能訓練成一個好的Agent。

# **5. 團隊貢獻**

- 許恒睿(50%)：GameState, Minimax, UnionFind
- 孫于涵(50%)：MCTS, Deep Q-Learning, Parameter Tuning
- 共同負責：Parameter Tuning Script, Makefile, Report