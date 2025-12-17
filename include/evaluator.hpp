#include<cstdint>
#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP
#include "game_def.hpp"

uint8_t get_score(uint8_t private_card, uint8_t public_card){
    if(private_card == public_card){
        return private_card + 10;
    }else{
        return private_card;
    }
}

float get_payoff(uint8_t p1_card, uint8_t p2_card, uint8_t public_card){
    uint8_t p1_score = get_score(p1_card,public_card);
    uint8_t p2_score = get_score(p2_card,public_card);
    if(p1_score > p2_score){
        return 1.0;
    }else if(p2_score > p1_score){
        return -1.0;
    }else{
        return 0.0;
    }
}

#endif