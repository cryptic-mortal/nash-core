#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>
#include "game_def.hpp"
#include "rng.hpp"
#include "dealer.hpp"
#include "indexer.hpp"
#include "game_tree.hpp"

extern std::vector<double> global_regret_sum;
extern std::vector<double> global_strategy_sum;
extern Leduc::Indexer global_indexer;
extern Node* game_tree_roots[3][3];
extern Node* build_tree(Leduc::GameState state, Leduc::Indexer& indexer);

extern float cfr(Node* node, int traversing_player, RNG& rng, std::vector<float>& local_regret, std::vector<float>& local_strategy);

void run_silent_workload(int total_iterations, int num_threads) {
    int num_info_sets = global_indexer.get_size();
    
    // Reset global arrays so every thread test starts from a clean slate
    global_regret_sum.assign(num_info_sets * 3, 0.0);
    global_strategy_sum.assign(num_info_sets * 3, 0.0);

    int iterations_per_thread = total_iterations / num_threads;
    std::vector<std::thread> threads;

    for(int t = 0; t < num_threads; t++){
        threads.push_back(std::thread([&](int id){
            RNG rng(100 + id); // Thread-local RNG seed to prevent contention
            std::vector<float> local_regret(num_info_sets * 3, 0.0f);
            std::vector<float> local_strategy(num_info_sets * 3, 0.0f);
            int BATCH_SIZE = 64; 
            
            for(int i = 0; i < iterations_per_thread; i++){
                Leduc::GameState state = Leduc::deal_initial_state(rng);
                int r1 = state.p1_card / 2;
                int r2 = state.p2_card / 2;
                int traversing_player = i % 2;
                
                // Call the original batch-based CFR
                cfr(game_tree_roots[r1][r2], traversing_player, rng, local_regret, local_strategy);
                
                // Batch merge to global arrays
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
    
    // Barrier synchronization
    for(int t = 0; t < num_threads; t++) {
        threads[t].join();
    }
}

void benchmark_scalability() {
    const int TOTAL_ITERATIONS = 50000000; // 50 Million iterations for a heavy workload
    
    // Dynamically detect hardware
    int max_hardware_threads = std::thread::hardware_concurrency();
    if (max_hardware_threads == 0) max_hardware_threads = 8;
    
    // Test 1, 2, 4, 8... up to max cores
    std::vector<int> thread_counts;
    for (int i = 1; i <= max_hardware_threads; i *= 2) {
        thread_counts.push_back(i);
    }
    // Ensure the absolute max thread count is tested if it's not a power of 2
    if (std::find(thread_counts.begin(), thread_counts.end(), max_hardware_threads) == thread_counts.end()) {
        thread_counts.push_back(max_hardware_threads);
    }

    std::cout << "Total Iterations: " << TOTAL_ITERATIONS << " Iterations\n";
    std::cout << "Hardware: " << max_hardware_threads << " CPU Cores Detected\n";
    std::cout << "Locking: Lock-Free (std::memory_order_relaxed)\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(10) << "Threads" 
              << std::setw(15) << "Time (s)" 
              << std::setw(15) << "Speedup S(N)" 
              << std::setw(15) << "Effic. E(N)" 
              << std::setw(20) << "Throughput (iters/s)" << "\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    double base_time = 0.0;

    for (int cores : thread_counts) {
        auto start = std::chrono::high_resolution_clock::now();
        
        run_silent_workload(TOTAL_ITERATIONS, cores);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        double current_time = elapsed.count();
        
        // Base time (T1) is required to calculate Amdahl's Speedup
        if (cores == 1) {
            base_time = current_time;
        }

        // S(N) = T(1) / T(N)
        double speedup = base_time / current_time;
        
        // E(N) = S(N) / N * 100
        double efficiency = (speedup / cores) * 100.0;
        
        // Throughput
        double throughput = TOTAL_ITERATIONS / current_time;

        std::cout << std::left << std::setw(10) << cores 
                  << std::setw(15) << std::fixed << std::setprecision(3) << current_time 
                  << std::fixed << std::setprecision(2) << speedup << "x" << std::setw(11) << " "
                  << std::fixed << std::setprecision(1) << efficiency << "%" << std::setw(9) << " "
                  << std::fixed << std::setprecision(0) << throughput << "\n";
    }
    std::cout << "================================================================================\n\n";
}

int main() {
    RNG rng(100);

    std::cout << "Initializing Indexer and Building Game Trees in RAM...\n";
    int num_info_sets = global_indexer.get_size();
    
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
    std::cout << "Initialization Complete. Total Info Sets: " << num_info_sets << "\n";

    // Run the HFT Benchmark
    benchmark_scalability();

    return 0;
}