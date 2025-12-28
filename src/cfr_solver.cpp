#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include "game_def.hpp"
#include "dealer.hpp"
#include "evaluator.hpp"
#include "info_set.hpp"
#include "indexer.hpp"
#include "rng.hpp"

std::vector<float> global_regret_sum;
std::vector<float> global_strategy_sum;
Leduc::Indexer global_indexer;

std::vector<float> get_current_strategy(int id){
    std::vector<float> strategy(3,0.0f);
    float sum = 0.0f;
    int offset = id*3;

    for(int a = 0; a < 3; a++){
        float r = global_regret_sum[offset + a];
        strategy[a] = (r > 0) ? r : 0.0f;
        sum += strategy[a];
    }

    if(sum > 0){
        for(int a = 0; a < 3; a++){
            strategy[a]/=sum;
        }
    }else{
        for(int a = 0; a < 3; a++){
            strategy[a] = 1.0f/3.0f;
        }
    }
    return strategy;
}

float cfr(Leduc::GameState state, float p0, float p1, RNG& rng){
    if(state.is_terminal){
        std::pair<float,float> payoff = Leduc::get_payoff(state);
        return payoff.first;
    }
    uint8_t player = state.current_player;
    std::string key = Leduc::get_info_set_key(state);
    int id = global_indexer.get_id(key);

    if(id == -1){
        std::cerr << "Error : Unknown State " << key << std::endl;
        return 0.0f;
    }

    std::vector<float> strategy = get_current_strategy(id);

    float action_utils[3] = {0.0f};
    bool is_valid[3] = {false};

    for(int action = 0; action<3; action++){
        Leduc::Action act = static_cast<Leduc::Action>(action);
        if(act == Leduc::Action::BET_RAISE && state.raises_this_round>=4) {
            is_valid[action] = false;
            continue;
        }
        is_valid[action] = true;
        
        Leduc::GameState next_state = Leduc::step(state,act);

        if (state.round != next_state.round) {
            int valid_card = -1;
            
            for(int k=0; k<20; k++) {
                int r = rng.next_int(6);
                if(r != state.p1_card && r != state.p2_card) {
                    valid_card = r;
                    break;
                }
            }
            if(valid_card == -1) {
                 for(int c=0; c<6; c++) {
                     if(c != state.p1_card && c != state.p2_card) {
                         valid_card = c;
                         break;
                     }
                 }
            }

            next_state.public_card = valid_card;
        }
        if(player == 0) action_utils[action] = cfr(next_state, p0*strategy[action], p1, rng);
        else action_utils[action] = cfr(next_state, p0, p1*strategy[action], rng);
    }

    float node_util = 0.0f;
    for(int i = 0; i < 3; i++){
        if(is_valid[i]) {
            node_util += strategy[i]*action_utils[i];
        }
    }
    float opponent_prob = (player == 0) ? p1 : p0;
    float my_prob = (player == 0) ? p0 : p1;
    int offset = id*3;
    
    for(int i = 0; i<3; i++){
        if(!is_valid[i]) continue;

        float my_action_val = (player == 0) ? action_utils[i] : -action_utils[i];
        float my_node_val = (player == 0) ? node_util : -node_util;

        float regret = my_action_val - my_node_val;

        global_regret_sum[offset + i] += regret*opponent_prob;
        global_strategy_sum[offset + i] += strategy[i]*my_prob;
    }
    return node_util;
}
void print_strategy() {

    std::cout << "\n=== LEARNED STRATEGY (Vector Based) ===\n";
    std::cout << std::left << std::setw(25) << "Info Set (Card:Hist)" 
              << std::setw(15) << "Fold/Check" 
              << std::setw(15) << "Call" 
              << std::setw(15) << "Raise" << std::endl;
    std::cout << "----------------------------------------------------------------------" << std::endl;
    
    int num_sets = global_indexer.get_size();

    std::vector<std::pair<std::string,int>> sorted_keys;

    for(int i = 0; i < num_sets; i++){
        sorted_keys.push_back({global_indexer.get_key(i),i});
    }
    std::sort(sorted_keys.begin(),sorted_keys.end());

    for (auto const& [key, id] : sorted_keys) {
        float sum = 0.0f;
        int offset = id*3;
        for(int i = 0; i < 3; i++) sum += global_strategy_sum[offset+i];

        std::cout << std::left << std::setw(25) << key;
        
        for (int i = 0; i < 3; i++) {
            float prob = (sum > 0) ? (global_strategy_sum[offset+i]/ sum) : (1.0f / 3.0f);
            if (prob < 0.001f) prob = 0.0f; 
            
            std::cout << std::fixed << std::setprecision(3) << std::setw(15) << prob;
        }
        std::cout << std::endl;
    }
}
int main(){
    RNG rng(100);

    int num_info_sets = global_indexer.get_size();
    global_regret_sum.resize(num_info_sets*3, 0.0f);
    global_strategy_sum.resize(num_info_sets*3, 0.0f);

    std::cout << "Indexer Initialized. Flattened " << num_info_sets 
              << " sets into vectors of size " << global_regret_sum.size() << std::endl;

    int ITERATIONS = 100000;
    double total_val = 0.0;
    std::vector<double> evs;
    std::cout << "Started CFR Training (" << ITERATIONS <<" games)..." << std :: endl;
    for(int i = 0; i < ITERATIONS; i++){
        Leduc::GameState state = Leduc::deal_initial_state(rng);
        total_val += cfr(state, 1.0f, 1.0f, rng);
        if(i%10000 == 0) {
            std::cout << "." << std::flush;
            evs.push_back(total_val/(i+1));
        }
    }
    std::cout << "\nTraining Complete." << std::endl;
    // The values should converge approx to 0.0 which means that no player has any unfair advantage
    for(auto i : evs) {
        std::cout<<"Avg. EV (P1): "<<i<<"\n";
    }
    print_strategy();
    return 0;
}