#include <cstdint>
#ifndef GAME_UTILS_HPP
#define GAME_UTILS_HPP
#include "game_def.hpp"
#include "rng.hpp"

inline Leduc::Action get_random_move(const Leduc::GameState& state, RNG& rng){
    uint8_t num_choices = (state.raises_this_round < Leduc::MAX_RAISES_PER_ROUND) ? 3 : 2;
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

#endif