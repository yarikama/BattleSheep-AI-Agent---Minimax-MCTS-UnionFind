// ConsoleApplication2.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <process.h>
#include "game_data.h"

#define clc 0.1
#define gamma 0.95
#define epochNum 10


void run_game(ActorCritic& model) {
    // Save the model parameters to model.pkl
    torch::save(model, "model.pkl");

    intptr_t status = _spawnl(_P_WAIT, "AI_game.exe", "AI_game.exe", NULL);
    if (status == -1) {
        std::cerr << "Failed to execute AI_game.exe" << std::endl;
    }
}
void clearVectors() {
    x_logprob.clear();
    y_logprob.clear();
    m_logprob.clear();
    dir_logprob.clear();
    value.clear();
    reward.clear();
}
std::tuple<torch::Tensor, torch::Tensor, int> update_params(ActorCritic& model, torch::optim::Adam& optimizer) {
    // vectors from game_data.h
    auto rewards = torch::from_blob(reward.data(), { static_cast<long>(reward.size()) }, torch::kFloat).flip({ 0 });
    auto x_logprobs = torch::from_blob(x_logprob.data(), { static_cast<long>(x_logprob.size()) }, torch::kFloat).flip({ 0 });
    auto y_logprobs = torch::from_blob(y_logprob.data(), { static_cast<long>(y_logprob.size()) }, torch::kFloat).flip({ 0 });
    auto m_logprobs = torch::from_blob(m_logprob.data(), { static_cast<long>(m_logprob.size()) }, torch::kFloat).flip({ 0 });
    auto dir_logprobs = torch::from_blob(dir_logprob.data(), { static_cast<long>(dir_logprob.size()) }, torch::kFloat).flip({ 0 });
    auto values = torch::from_blob(value.data(), { static_cast<long>(value.size()) }, torch::kFloat).flip({ 0 });
    //清空vector
    clearVectors();

    std::vector<torch::Tensor> Returns;
    torch::Tensor ret_ = torch::zeros({ 1 });
    for (int r = 0; r < rewards.size(0); ++r) {
        ret_ = rewards[r] + gamma * ret_;
        Returns.push_back(ret_);
    }
    auto returns = torch::stack(Returns).view({ -1 });
    returns = torch::nn::functional::normalize(returns, torch::nn::functional::NormalizeFuncOptions().dim(0));

    auto x_actor_loss = -1 * x_logprobs * (returns - values.detach());
    auto y_actor_loss = -1 * y_logprobs * (returns - values.detach());
    auto m_actor_loss = -1 * m_logprobs * (returns - values.detach());
    auto dir_actor_loss = -1 * dir_logprobs * (returns - values.detach());
    auto actor_loss = x_actor_loss.sum() + y_actor_loss.sum() + m_actor_loss.sum() + dir_actor_loss.sum();

    auto critic_loss = torch::pow(values - returns, 2).sum();

    auto loss = actor_loss + clc * critic_loss;
    loss.backward();
    optimizer.step();

    return { actor_loss, critic_loss, static_cast<int>(rewards.size(0)) };
}

int main() {
    ActorCritic model;
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(1e-4));
    optimizer.zero_grad();

    for (int i = 0; i < epochNum; ++i) {
        run_game(model);
        auto [actor_loss, critic_loss, len] = update_params(model, optimizer);
        std::cout << "game: " << i << ", Actor Loss: " << actor_loss.item<float>()
            << ", Critic Loss: " << critic_loss.item<float>() << ", Number of steps: " << len << std::endl;
    }

    // Save the final model parameters
    torch::save(model, "model.pkl");

    return 0;
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
