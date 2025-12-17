#include <cstdint>
#ifndef DEALER_HPP
#define DEALER_HPP
#include "rng.hpp"
#include "game_def.hpp"

void swap(uint8_t& a, uint8_t& b){
    uint8_t c = b;
    b = a;
    a = c;
}

GameState deal_initial_state(RNG& rng){
    uint8_t deck[6] = {
        Leduc::JACK,Leduc::JACK,
        Leduc::QUEEN,Leduc::QUEEN,
        Leduc::KING,Leduc::KING
    };
    for(uint8_t i = 5; i>0; i--){
        uint8_t j = rng.next_int(i+1);
        swap(deck[i],deck[j]);
    }
    

    GameState gs;
    
        gs.p1_card = deck[0];
        gs.p2_card = deck[1];
        gs.public_card = deck[2];

        gs.pot = 2.0f;
        gs.history_len = 0;
        gs.current_player = 0;
        gs.round = false;
        gs.is_terminal= false;
        for(uint8_t k = 0; k<16; k++){
            gs.history[k] = Action::NONE;
        }
    
    return gs;
}
#endif