#include<cstdint>
#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP
#include "game_def.hpp"

namespace Leduc {

    inline uint8_t get_score(uint8_t private_card, uint8_t public_card){
        if((private_card >> 1) == (public_card >> 1)){
            return (private_card >> 1) + 10;
        } else {
            return (private_card >> 1);
        }
    }

    inline float get_payoff(const GameState& state){
        if(state.is_terminal) {
            if(state.history_len>0 && state.history[state.history_len-1] == Action::FOLD) {
                bool p1_folded = (state.current_player == 0);
                if(p1_folded) return -state.pot;
                else return state.pot;
            }
        }
        uint8_t p1_score = get_score(state.p1_card,state.public_card);
        uint8_t p2_score = get_score(state.p2_card,state.public_card);
        if(p1_score > p2_score){
            return state.pot;
        } else if (p2_score > p1_score){
            return -state.pot;
        } else {
            return 0;
        }
    }

};

#endif