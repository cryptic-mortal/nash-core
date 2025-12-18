#include <cstdint>
#ifndef DEALER_HPP
#define DEALER_HPP
#include "rng.hpp"
#include "game_def.hpp"

namespace Leduc{
    inline void swap(uint8_t& a, uint8_t& b){
        uint8_t c = b;
        b = a;
        a = c;
    }

    inline Leduc::GameState deal_initial_state(RNG& rng){
        uint8_t deck[6] = {
            (Leduc::JACK << 1) | Leduc::SUIT_0,
            (Leduc::JACK << 1) | Leduc::SUIT_1,
            (Leduc::QUEEN << 1) | Leduc::SUIT_0,
            (Leduc::QUEEN << 1) | Leduc::SUIT_1,
            (Leduc::KING << 1) | Leduc::SUIT_0,
            (Leduc::KING << 1) | Leduc::SUIT_1
        };
        for(uint8_t i = 5; i>0; i--){
            uint8_t j = rng.next_int(i+1);
            swap(deck[i],deck[j]);
        }
        

        Leduc::GameState state;
        
            state.p1_card = deck[0];
            state.p2_card = deck[1];
            state.public_card = deck[2];

            state.pot = 3.0f;
            state.committed[0] = 1.0f;
            state.committed[1] = 2.0f;
            state.current_bet_size = 1.0f;
            state.raises_this_round = 0;
            state.actions_this_round = 0;
            state.history_len = 0;
            state.current_player = 0;
            state.round = 0;
            state.is_terminal= false;
            for(uint8_t k = 0; k<16; k++){
                state.history[k] = Leduc::Action::NONE;
            }
        
        return state;
    }
}
#endif