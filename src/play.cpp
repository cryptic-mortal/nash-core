#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include <fstream>
#include <algorithm>
#include "game_def.hpp"
#include "dealer.hpp"
#include "evaluator.hpp"
#include "info_set.hpp"
#include "indexer.hpp"
#include "rng.hpp"
#include "game_tree.hpp"

Leduc::Indexer global_indexer;
std::vector<double> loaded_strategy;
Node* game_tree_roots[3][3];
extern Node* build_tree(Leduc::GameState state, Leduc::Indexer& indexer);

bool load_strategy(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) return false;
    int num_sets;
    in.read(reinterpret_cast<char*>(&num_sets), sizeof(int));
    loaded_strategy.resize(num_sets * 3);
    in.read(reinterpret_cast<char*>(loaded_strategy.data()), num_sets * 3 * sizeof(double));
    in.close();
    return true;
}

std::string action_to_string(Leduc::Action a) {
    switch(a) {
        case Leduc::Action::FOLD: return "Fold";
        case Leduc::Action::CHECK_CALL: return "Check/Call";
        case Leduc::Action::BET_RAISE: return "Bet/Raise";
        default: return "None";
    }
}

std::string card_to_string(uint8_t card) {
    std::string rank;
    int r = card >> 1;
    if (r == Leduc::JACK) rank = "J";
    else if (r == Leduc::QUEEN) rank = "Q";
    else if (r == Leduc::KING) rank = "K";
    
    std::string suit = (card & 1) ? "s" : "h";
    return rank + suit;
}

int main() {
    std::cout << "=== Leduc Hold'em: Human vs AI ===\n";
    if (!load_strategy("strategy.dat")) {
        std::cerr << "Error: Could not load strategy.dat. Please run the solver first.\n";
        return 1;
    }
    
    RNG rng(time(0));
    for(int r1 = 0; r1 < 3; r1++) {
        for(int r2 = 0; r2 < 3; r2++) {
            Leduc::GameState root_state;
            root_state.p1_card = r1 * 2;
            root_state.p2_card = (r1 == r2) ? (r2 * 2 + 1) : (r2 * 2);
            root_state.is_terminal = false;
            root_state.round = 0;
            root_state.current_player = 0;
            root_state.raises_this_round = 0;
            root_state.history_len = 0;
            root_state.pot = 3.0f;
            root_state.committed[0] = 1.0f;
            root_state.committed[1] = 2.0f;
            root_state.current_bet_size = 1.0f;
            game_tree_roots[r1][r2] = build_tree(root_state, global_indexer);
        }
    }

    float total_profit = 0;
    std::string input;
    
    while (true) {
        Leduc::GameState state = Leduc::deal_initial_state(rng);
        int human_player = rng.next_int(2);
        std::cout << "\n----------------------------------------\n";
        std::cout << "New Round! You are Player " << human_player + 1 << "\n";
        
        while (!state.is_terminal) {
            std::cout << "\nRound: " << (int)state.round << " | Pot: " << state.pot << "\n";
            std::cout << "Your Card: " << card_to_string(human_player == 0 ? state.p1_card : state.p2_card) << "\n";
            if (state.round == 1) std::cout << "Public Card: " << card_to_string(state.public_card) << "\n";
            
            if (state.current_player == human_player) {
                std::cout << "Actions: [0] Fold, [1] Check/Call";
                if (state.raises_this_round < Leduc::MAX_RAISES_PER_ROUND) std::cout << ", [2] Bet/Raise";
                std::cout << "\nYour move: ";
                int move;
                std::cin >> move;
                Leduc::Action act = static_cast<Leduc::Action>(move);
                state = Leduc::step(state, act);
                std::cout << "You played: " << action_to_string(act) << "\n";
            } else {
                std::string key = Leduc::get_info_set_key(state);
                int id = global_indexer.get_id(key);
                int offset = id * 3;
                
                double sum = 0;
                std::vector<double> probs(3, 0);
                for (int a = 0; a < 3; a++) {
                    probs[a] = loaded_strategy[offset + a];
                    sum += probs[a];
                }
                
                int ai_move = 1;
                if (sum > 0) {
                    double r = (double)rng.next_float() * sum;
                    double cumulative = 0;
                    for (int a = 0; a < 3; a++) {
                        cumulative += probs[a];
                        if (r <= cumulative) {
                            ai_move = a;
                            break;
                        }
                    }
                }
                
                Leduc::Action act = static_cast<Leduc::Action>(ai_move);
                state = Leduc::step(state, act);
                std::cout << "AI played: " << action_to_string(act) << "\n";
            }
        }
        
        std::pair<float, float> payoffs = Leduc::get_payoff(state);
        float human_payoff = (human_player == 0) ? payoffs.first : payoffs.second;
        total_profit += human_payoff;
        
        std::cout << "\nGame Over!\n";
        std::cout << "Opponent's Card: " << card_to_string(human_player == 0 ? state.p2_card : state.p1_card) << "\n";
        std::cout << "Result: " << (human_payoff > 0 ? "WIN" : (human_payoff < 0 ? "LOSS" : "DRAW")) << " (" << human_payoff << " chips)\n";
        std::cout << "Total Profit: " << total_profit << " chips\n";
        
        std::cout << "\nPlay again? (y/n): ";
        std::cin >> input;
        if (input != "y") break;
    }
    
    return 0;
}
