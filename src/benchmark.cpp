#include <iostream>
#include <chrono>
#include "game_def.hpp"
#include "rng.hpp"
#include "dealer.hpp"
#include "evaluator.hpp"

Leduc::Action get_random_move(const Leduc::GameState& state, RNG& rng){
    uint8_t num_choices = (state.raises_this_round < 4) ? 3 : 2;
    uint8_t choice_idx = rng.next_int(num_choices);
    Leduc::Action action;
    if(choice_idx == 0){
        action = Leduc::Action::FOLD;
    }else if (choice_idx == 1){
        action = Leduc::Action::CHECK_CALL;
    }else{
        action = Leduc::Action::BET_RAISE;
    }
    return action;
}

int main(){
    RNG rng(100);
    const uint32_t NUM_GAMES = 10000000;
    auto start_time = std::chrono::high_resolution_clock::now();
    volatile float total_payoff = 0;
    for(uint32_t i = 0; i < NUM_GAMES; i++){
        Leduc::GameState state = Leduc::deal_initial_state(rng);
        while (!state.is_terminal) {
            Leduc::Action action = get_random_move(state, rng);
            state = Leduc::step(state, action);
        }
        total_payoff += Leduc::get_payoff(state);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    double seconds = elapsed.count();
    double speed = NUM_GAMES / seconds;
    std::cout << "Time:  " << seconds << " seconds\n";
    std::cout << "Speed: " << (speed / 1000000.0) << " Million games/sec\n";
}