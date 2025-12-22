#include <iostream>
#include <map>
#include <iomanip>
#include <algorithm>
#include "game_def.hpp"
#include "dealer.hpp"
#include "evaluator.hpp"
#include "info_set.hpp"

Leduc::CFRMap regret_map;

float cfr(Leduc::GameState state, float p0, float p1){
    if(state.is_terminal){
        std::pair<float,float> payoff = Leduc::get_payoff(state);
        return payoff.first;
    }
    uint8_t player = state.current_player;
    std::string key = Leduc::get_info_set_key(state);
    Leduc::CFRNode& node = regret_map[key];

    std::vector<float> strategy = node.get_strategy();

    float action_utils[3];
    bool is_valid[3];

    for(int action = 0; action<3; action++){
        Leduc::Action act = static_cast<Leduc::Action>(action);
        if(act == Leduc::Action::BET_RAISE && state.raises_this_round>=4) {
            is_valid[action] = false;
            continue;
        }
        is_valid[action] = true;
        
        Leduc::GameState next_state = Leduc::step(state,act);
        if(player == 0) action_utils[action] = cfr(next_state, p0*strategy[action], p1);
        else action_utils[action] = cfr(next_state, p0, p1*strategy[action]);
    }

    float node_util = 0.0f;
    for(int i = 0; i < 3; i++){
        if(is_valid[i]) {
            node_util += strategy[i]*action_utils[i];
        }
    }
    float opponent_prob = (player == 0) ? p1 : p0;
    float my_prob = (player == 0) ? p0 : p1;
    
    for(int i = 0; i<3; i++){
        if(!is_valid[i]) continue;

        float my_action_val = (player == 0) ? action_utils[i] : -action_utils[i];
        float my_node_val = (player == 0) ? node_util : -node_util;

        float regret = my_action_val - my_node_val;

        node.regret_sum[i] += regret*opponent_prob;
        node.strategy_sum[i] += strategy[i]*my_prob;
    }
    return node_util;
}
void print_strategy(const Leduc::CFRMap& unordered_map) {
    std::map<std::string, Leduc::CFRNode> sorted_map(unordered_map.begin(), unordered_map.end());

    std::cout << "\n=== LEARNED STRATEGY (Average) ===\n";
    std::cout << std::left << std::setw(25) << "Info Set (Card:Hist)" 
              << std::setw(15) << "Fold/Check" 
              << std::setw(15) << "Call" 
              << std::setw(15) << "Raise" << std::endl;
    std::cout << "----------------------------------------------------------------------" << std::endl;

    for (auto const& [key, node] : sorted_map) {
        float sum = 0.0f;
        for (float s : node.strategy_sum) sum += s;

        std::cout << std::left << std::setw(25) << key;
        
        for (int i = 0; i < 3; i++) {
            float prob = (sum > 0) ? (node.strategy_sum[i] / sum) : (1.0f / 3.0f);
            if (prob < 0.001f) prob = 0.0f; 
            
            std::cout << std::fixed << std::setprecision(3) << std::setw(15) << prob;
        }
        std::cout << std::endl;
    }
}
int main(){
    RNG rng(100);
    int ITERATIONS = 100000;

    std::cout << "Started CFR Training (" << ITERATIONS <<" games)..." << std :: endl;
    for(int i = 0; i < ITERATIONS; i++){
        Leduc::GameState state = Leduc::deal_initial_state(rng);
        cfr(state, 1.0f, 1.0f);
        if(i%10000 == 0) std::cout << "." << std::flush;
    }
    std::cout << "\nTraining Complete. Learned " << regret_map.size() << " info sets." << std::endl;

    print_strategy(regret_map);
    return 0;
}