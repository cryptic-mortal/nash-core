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
#include <thread>
#include <functional>
#include <atomic>
#include <chrono>
#include <fstream>
#include <ctime>

std::vector<double> global_regret_sum;
std::vector<double> global_strategy_sum;

void atomic_add_float(std::atomic<float>& target, float val){
    float current = target.load(std::memory_order_relaxed);
    while(!target.compare_exchange_weak(current, current + val, std::memory_order_relaxed, std::memory_order_relaxed)){

    }
}
Leduc::Indexer global_indexer;
Node* game_tree_roots[3][3];
extern Node* build_tree(Leduc::GameState state, Leduc::Indexer& indexer);

std::vector<float> get_current_strategy(Node* node){
    std::vector<float> strategy(3,0.0f);
    float sum = 0.0f;
    int offset = node->info_set_id*3;

    for(int a = 0; a < 3; a++){
        if(node->action_children[a] == nullptr) continue;
        float r = std::atomic_ref<double>(global_regret_sum[offset + a]).load(std::memory_order_relaxed);
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

float cfr(Node* node, int traversing_player, RNG& rng, std::vector<float>& local_regret, std::vector<float>& local_strategy){

    if(node->is_terminal){
        return (traversing_player == 0) ? node->payoff : -node->payoff;
    }

    if(node->is_chance) {
        int num_choices= node->chance_children.size();
        int rand_idx = rng.next_int(num_choices);
        return cfr(node->chance_children[rand_idx], traversing_player ,rng, local_regret, local_strategy);
    }
    uint8_t player = node->player;
    int id = node->info_set_id;

    std::vector<float> strategy = get_current_strategy(node);
    int offset = id*3;

    if(player == traversing_player){
        float action_utils[3] = {0.0f};
        float node_util = 0.0f;
        for(int action = 0; action < 3; action++){
            if(node->action_children[action] == nullptr) continue;
            action_utils[action] = cfr(node->action_children[action], traversing_player, rng, local_regret, local_strategy);
            node_util += strategy[action] * action_utils[action];
        }

        for(int a = 0; a<3; a++){
            if(node->action_children[a] == nullptr) continue;
            float regret = action_utils[a] - node_util;
            // std::atomic_ref<float>(global_regret_sum[offset + a]).fetch_add(regret,std::memory_order_relaxed);
            local_regret[offset+a] += regret;
        }
        return node_util;
    }else{
        for(int a = 0; a<3; a++){
            if(node->action_children[a]!=nullptr){
                //std::atomic_ref<float>(global_strategy_sum[offset + a]).fetch_add(strategy[a],std::memory_order_relaxed);
                local_strategy[offset + a] += strategy[a];
            }
        }

        float r = rng.next_float();
        float cum_prob= 0.0f;
        int sampled_action = -1;

        for(int a = 0; a<3; a++){
            if(node->action_children[a] == nullptr) continue;
            cum_prob += strategy[a];
            if(r <= cum_prob){
                sampled_action = a;
                break;
            }
        }

        if(sampled_action == -1){
            for(int a = 0; a<3; a++){
                if(node->action_children[a]!=nullptr){
                    sampled_action = a;
                    break;
                }
            }
        }
        return cfr(node->action_children[sampled_action], traversing_player, rng, local_regret, local_strategy);
    }
}
// void worker_thread(int thread_id, int iterations, std::vector<float>& local_regret, std::vector<float>& local_strategy){
//     RNG rng(100+thread_id);

//     for(int i = 0; i<iterations; i++){
//         Leduc::GameState state = Leduc::deal_initial_state(rng);
//         int r1 = state.p1_card/2;
//         int r2 = state.p2_card/2;
//         int traversing_player = i%2;

//         cfr(game_tree_roots[r1][r2], traversing_player,rng);
//     }
// }
void print_strategy() {

    std::cout << "\n=== LEARNED STRATEGY (Atomic Based) ===\n";
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
        for(int i = 0; i < 3; i++){
            sum += std::atomic_ref<double>(global_strategy_sum[offset+i]).load(std::memory_order_relaxed);
        }
        std::cout << std::left << std::setw(25) << key;
        
        for (int i = 0; i < 3; i++) {
            float prob = (sum > 0) ? (std::atomic_ref<double>(global_strategy_sum[offset+i]).load(std::memory_order_relaxed)/ sum) : (1.0f / 3.0f);
            if (prob < 0.001f) prob = 0.0f; 
            
            std::cout << std::fixed << std::setprecision(3) << std::setw(15) << prob;
        }
        std::cout << std::endl;
    }
}

void measure_convergence() {
    std::string key_nodes[] = {"J:*:r", "K:K:cc", "Q:*:cr"};
    std::cout << "   Convergence Check (Key GTO Inflection Points):\n";
    
    int num_sets = global_indexer.get_size();
    for(int i = 0; i < num_sets; i++) {
        std::string key = global_indexer.get_key(i);
        for(const std::string& target : key_nodes) {
            if(key == target) {
                float sum = 0.0f;
                int offset = i * 3;
                for(int j = 0; j < 3; j++) sum += std::atomic_ref<double>(global_strategy_sum[offset+j]).load(std::memory_order_relaxed);
                
                std::cout << "   -> " << key << " Strategy: ";
                for (int j = 0; j < 3; j++) {
                    float prob = (sum > 0) ? (std::atomic_ref<double>(global_strategy_sum[offset+j]).load(std::memory_order_relaxed) / sum) : (1.0f / 3.0f);
                    std::cout << std::fixed << std::setprecision(3) << prob << " ";
                }
                std::cout << "\n";
            }
        }
    }
}

float best_response(Node* node, int br_player, float opp_reach){
    if(node -> is_terminal){
        return (br_player == 0) ? (node->payoff * opp_reach) : (-node->payoff * opp_reach);
    }

    if(node -> is_chance){
        float expected_val = 0.0f;
        int num_choices = node->chance_children.size();
        for(int i = 0; i<num_choices; i++){
            expected_val += best_response(node->chance_children[i], br_player, opp_reach * (1.0f/num_choices));
        }
        return expected_val;
    }

    if(node->player == br_player){
        float max_val = -1e9;
        for(int a = 0; a<3; a++){
            if(node->action_children[a]!=nullptr){
                float val = best_response(node->action_children[a], br_player, opp_reach);
                if(val > max_val) max_val = val;
            }
        }
        return max_val;
    }else{
        int offset = node->info_set_id*3;
        float sum = 0.0f;
        for(int a = 0; a<3; a++){
            if(node->action_children[a]!=nullptr){
                sum += std::atomic_ref<double>(global_strategy_sum[offset+a]).load(std::memory_order_relaxed);
            }
        }

        float expected_val = 0.0f;
        for(int a = 0; a<3; a++){
            if(node->action_children[a]!=nullptr){
                float prob = 0.0f;
                if(sum > 0){
                    prob = std::atomic_ref<double>(global_strategy_sum[offset+a]).load(std::memory_order_relaxed)/sum;
                }else{
                    int valid = 0;
                    for(int b = 0; b<3; b++){
                        if(node->action_children[b]){
                            valid++;
                        }
                    }
                    prob = 1.0f/valid;
                }
                expected_val += best_response(node->action_children[a], br_player, opp_reach * prob);
            }
        }
        return expected_val;
    }
}

float calc_exploitability() {
    float br_value_0 = 0.0f;
    float br_value_1 = 0.0f;

    for(int r1 = 0; r1 < 3; r1++){
        for(int r2 = 0; r2 < 3; r2++){
            float prob = (r1 == r2) ? (1.0f / 15.0f) : (2.0f/15.0f);

            br_value_0 += prob * best_response(game_tree_roots[r1][r2], 0, 1.0f);
            br_value_1 += prob * best_response(game_tree_roots[r1][r2], 1, 1.0f);
        }
    }

    return (br_value_0 + br_value_1)/2;
}

void log_exploitability_csv(int iterations, float exploitability){
    std::ofstream file("exploitability_log.csv", std::ios::app);

    file.seekp(0, std::ios::end);
    if(file.tellp() == 0){
        file << "timestamp,iterations,exploitability\n";
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    file << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << ","
         << iterations << ","
         << std::fixed << std::setprecision(6) << exploitability << "\n";
         
    file.close();
}

void run_training(int total_iterations, int num_threads) {
    int num_info_sets = global_indexer.get_size();
    
    global_regret_sum.assign(num_info_sets * 3, 0.0f);
    global_strategy_sum.assign(num_info_sets * 3, 0.0f);

    int iterations_per_thread = total_iterations / num_threads;
    std::cout << "--------------------------------------------------------\n";
    std::cout << "Training Iterations: " << total_iterations << " (" << iterations_per_thread << " per thread)\n";

    std::vector<std::thread> threads;
    auto train_start = std::chrono::high_resolution_clock::now();

    for(int t = 0; t < num_threads; t++){
        threads.push_back(std::thread([&](int id){
            RNG rng(100 + id);
            
            std::vector<float> local_regret(num_info_sets * 3, 0.0f);
            std::vector<float> local_strategy(num_info_sets * 3, 0.0f);
            int BATCH_SIZE = 64; 

            for(int i = 0; i < iterations_per_thread; i++){
                Leduc::GameState state = Leduc::deal_initial_state(rng);
                int r1 = state.p1_card/2;
                int r2 = state.p2_card/2;
                int traversing_player = i % 2;
                
                cfr(game_tree_roots[r1][r2], traversing_player, rng, local_regret, local_strategy);
                if ((i + 1) % BATCH_SIZE == 0 || i == iterations_per_thread - 1) {
                    for (int j = 0; j < num_info_sets * 3; j++) {
                        if (local_regret[j] != 0.0f) {
                            std::atomic_ref<double>(global_regret_sum[j]).fetch_add(local_regret[j], std::memory_order_relaxed);
                            local_regret[j] = 0.0f; 
                        }
                        if (local_strategy[j] != 0.0f) {
                            std::atomic_ref<double>(global_strategy_sum[j]).fetch_add(local_strategy[j], std::memory_order_relaxed);
                            local_strategy[j] = 0.0f; 
                        }
                    }
                }
            }
        }, t));   
    }
    
    for(int t = 0; t < num_threads; t++) threads[t].join();
    
    auto train_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = train_end - train_start;
    double seconds = elapsed.count();
    
    float exploitability = calc_exploitability();

    log_exploitability_csv(total_iterations, exploitability);
    
    std::cout << "Time: " << seconds << " seconds | Speed: " << (total_iterations / seconds) << " games/sec\n";
    std::cout << "Exploitability: " << std::fixed << std::setprecision(5) << exploitability << " chips/game\n";
}
int main(){
    // for(int t = 0; t < num_threads; t++){
    //     for(size_t i = 0; i < global_strategy_sum.size(); i++){
    //         global_strategy_sum[i] += thread_strategies[t][i];
    //         global_regret_sum[i] += thread_regrets[t][i];
    //     }
    // }
    RNG rng(100);

    int num_info_sets = global_indexer.get_size();
    std::cout << "Indexer Initialized. Flattened " << num_info_sets << " sets.\n\n";

    std::cout << "Building Static Game Trees in RAM...\n";
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
    
    int num_threads = std::thread::hardware_concurrency();
    if(num_threads == 0) num_threads = 4;
    std::cout << "Detected " << num_threads << " CPU Cores.\n";

    std::vector<int> checkpoints = {1000000, 5000000, 10000000, 100000000};
    
    for (int iters : checkpoints) {
        run_training(iters, num_threads);
    }

    std::cout << "\nAll convergence tests complete!" << std::endl;
    return 0;
}