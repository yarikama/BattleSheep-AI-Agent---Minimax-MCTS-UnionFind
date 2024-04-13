#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <vector>
#include <torch/torch.h>

extern std::vector<float> x_logprob;
extern std::vector<float> y_logprob;
extern std::vector<float> m_logprob;
extern std::vector<float> dir_logprob;
extern std::vector<float> value;
extern std::vector<float> reward;

struct ActorCriticImpl : torch::nn::Module {
    ActorCriticImpl() {
        l1 = register_module("l1", torch::nn::Linear(288, 25));
        l2 = register_module("l2", torch::nn::Linear(25, 50));
        actor_lin1 = register_module("actor_lin1", torch::nn::Linear(50, 12));
        actor_lin2 = register_module("actor_lin2", torch::nn::Linear(50, 12));
        actor_lin3 = register_module("actor_lin3", torch::nn::Linear(50, 16));
        actor_lin4 = register_module("actor_lin4", torch::nn::Linear(50, 9));
        l3 = register_module("l3", torch::nn::Linear(50, 25));
        critic_lin1 = register_module("critic_lin1", torch::nn::Linear(25, 1));
    }
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> forward(torch::Tensor x) {
        auto y = torch::relu(l1->forward(x));
        y = torch::relu(l2->forward(y));

        auto x_coord = actor_lin1->forward(y);
        auto y_coord = actor_lin2->forward(y);
        auto m_split = actor_lin3->forward(y);
        auto dir_probs = torch::log_softmax(actor_lin4->forward(y), 0);

        auto c = torch::relu(l3->forward(y.detach()));
        auto critic = torch::tanh(critic_lin1->forward(c));

        return { x_coord, y_coord, m_split, dir_probs, critic };
    }
    
    torch::nn::Linear l1{ nullptr }, l2{ nullptr };
    torch::nn::Linear actor_lin1{ nullptr }, actor_lin2{ nullptr }, actor_lin3{ nullptr }, actor_lin4{ nullptr };
    torch::nn::Linear l3{ nullptr }, critic_lin1{ nullptr };
};
TORCH_MODULE(ActorCritic);
#endif