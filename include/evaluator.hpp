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

    inline std::pair<float,float> get_payoff(const GameState& state){

        if(!state.is_terminal){
            return {0.0f,0.0f};
        }

        float money_on_table = state.committed[0] + state.committed[1];
        float previous_rounds_pot = state.pot - money_on_table;

        float p1_invested = (previous_rounds_pot/2.0f) + state.committed[0];
        float p2_invested = (previous_rounds_pot/2.0f) + state.committed[1];
        
        bool p1_wins = false, p2_wins = false, draw = false;

        if(state.history_len>0 && state.history[state.history_len-1] == Action::FOLD) {
            if(state.current_player == 1) p1_wins = true;
            else p2_wins = true;
        }else{
            uint8_t p1_score = get_score(state.p1_card,state.public_card);
            uint8_t p2_score = get_score(state.p2_card,state.public_card);
            if(p1_score > p2_score) p1_wins = true;
            else if (p2_score > p1_score) p2_wins = true;
        }
        
        if(p1_wins){
            return {state.pot - p1_invested,-p2_invested};
        }else if(p2_wins){
            return {-p1_invested,state.pot - p2_invested};
        }else{
            return {(state.pot/2.0f)-p1_invested,(state.pot/2.0f) - p2_invested};
        }
    }

};

#endif