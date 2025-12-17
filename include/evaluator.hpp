#include<cstdint>
#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP
#include "game_def.hpp"

namespace Leduc {

    uint8_t get_score(uint8_t private_card, uint8_t public_card){
        if((private_card >> 1) == (public_card >> 1)){
            return (private_card >> 1) + 10;
        } else {
            return (private_card >> 1);
        }
    }

    float get_payoff(const GameState& state){
        uint8_t p1_score = get_score(state.p1_card,state.public_card);
        uint8_t p2_score = get_score(state.p2_card,state.public_card);
        if(p1_score > p2_score){
            return state.pot;
        } else if (p2_score > p1_score){
            return 0;
        } else {
            return state.pot/2.0f;
        }
    }

};

#endif