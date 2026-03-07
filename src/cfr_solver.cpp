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
#include "game_tree.hpp"

std::vector<float> global_regret_sum;
std::vector<float> global_strategy_sum;
Leduc::Indexer global_indexer;

Node* game_tree_roots[3][3];
extern Node* build_tree(Leduc::GameState state, Leduc::Indexer& indexer);

std::vector<float> get_current_strategy(Node* node){
    std::vector<float> strategy(3,0.0f);
    float sum = 0.0f;
    int offset = node->info_set_id*3;

    for(int a = 0; a < 3; a++){
        if(node->action_children[a] == nullptr) continue;
        float r = global_regret_sum[offset + a];
        strategy[a] = (r > 0) ? r : 0.0f;
        sum += strategy[a];
    }

    if(sum > 0){
        for(int a = 0; a < 3; a++){
            if(node->action_children[a]!=nullptr) strategy[a]/=sum;   
        }
    }else{
        int valid_count = 0;
        for(int a = 0; a < 3; a++) {
            if (node->action_children[a] != nullptr) valid_count++;
        }
        for(int a = 0; a < 3; a++) {
            if (node->action_children[a] != nullptr) strategy[a] = 1.0f / valid_count;
        }
    }
    return strategy;
}

float cfr(Node* node, float p0, float p1, RNG& rng){

    if(node->is_terminal) return node->payoff;

    if(node->is_chance) {
        int num_choices= node->chance_children.size();
        int rand_idx = rng.next_int(num_choices);
        return cfr(node->chance_children[rand_idx],p0,p1,rng);
    }
    uint8_t player = node->player;
    int id = node->info_set_id;

    std::vector<float> strategy = get_current_strategy(node);

    float action_utils[3] = {0.0f};

    for(int action = 0; action<3; action++){

        Node* child = node->action_children[action];
        if(child == nullptr) continue;

        if(player == 0) action_utils[action] = cfr(child,p0*strategy[action],p1,rng);
        else action_utils[action] = cfr(child,p0,p1*strategy[action],rng);
    }

    float node_util = 0.0f;
    for(int i = 0; i < 3; i++){
        if(node->action_children[i]!=nullptr){
            node_util += strategy[i]*action_utils[i];
        }
    }
    float opponent_prob = (player == 0) ? p1 : p0;
    float my_prob = (player == 0) ? p0 : p1;
    int offset = id*3;
    
    for(int i = 0; i<3; i++){
        if(node->action_children[i] == nullptr) continue;

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
              << std::setw(15) << "Fold" 
              << std::setw(15) << "Call/Check" 
              << std::setw(15) << "Bet/Raise" << std::endl;
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
    auto start_time = std::chrono::high_resolution_clock::now();

    int num_info_sets = global_indexer.get_size();
    global_regret_sum.resize(num_info_sets*3, 0.0f);
    global_strategy_sum.resize(num_info_sets*3, 0.0f);

    std::cout << "Indexer Initialized. Flattened " << num_info_sets 
              << " sets into vectors of size " << global_regret_sum.size() << std::endl;
    std::cout<<std::endl;

    std::cout << "Building Static Game Trees in RAM..." << std::endl;
    // Build the 9 possible starting configurations
    for(int r1 = 0; r1 < 3; r1++) {
        for(int r2 = 0; r2 < 3; r2++) {
            
            Leduc::GameState root_state = Leduc::deal_initial_state(rng); 
            
            root_state.p1_card = r1 * 2;
            root_state.p2_card = (r1 == r2) ? (r2 * 2 + 1) : (r2 * 2);
            root_state.is_terminal = false;
            root_state.round = 0;
            root_state.current_player = 0;
            root_state.raises_this_round = 0;
            root_state.history_len = 0;
            
            game_tree_roots[r1][r2] = build_tree(root_state, global_indexer);
        }
    }
    int ITERATIONS = 100000;
    double total_val = 0.0;
    std::vector<double> evs;
    std::cout << "Started Fast CFR Training (" << ITERATIONS <<" games)..." << std :: endl;
    
    for(int i = 0; i < ITERATIONS; i++){
        Leduc::GameState state = Leduc::deal_initial_state(rng);
        int r1 = state.p1_card / 2;
        int r2 = state.p2_card / 2;
        
        total_val += cfr(game_tree_roots[r1][r2], 1.0f, 1.0f, rng);
        
        if(i % 10000 == 0) {
            std::cout << "." << std::flush;
            evs.push_back(total_val/(i+1));
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    double seconds = elapsed.count();
    double speed = ITERATIONS / seconds;
    std::cout << "Time:  " << seconds << " seconds\n";
    std::cout << "Speed: " << (speed) << " games/sec\n";
    std::cout << "\nTraining Complete." << std::endl;
    print_strategy();
    return 0;
}