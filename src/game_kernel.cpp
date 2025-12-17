#include "game_def.hpp"

constexpr uint8_t MAX_RAISES_PER_ROUND = 4;

Leduc::GameState step(Leduc::GameState state, Leduc::Action action) {

    uint8_t active_player = state.current_player;
    uint8_t opponent = active_player ^ 1;

    float cost_to_call = state.committed[opponent] - state.committed[active_player];
    
    state.history[state.history_len] = action;
    state.history_len++;
    state.actions_this_round++;

    if(action == Leduc::Action::FOLD) {
        state.is_terminal = true;
        return state;
    } else if (action == Leduc::Action::CHECK_CALL) {
        state.pot += cost_to_call;
        state.committed[active_player] += cost_to_call;
    } else if(action == Leduc::Action::BET_RAISE) {
        if(state.raises_this_round < MAX_RAISES_PER_ROUND) {
            state.pot += cost_to_call + state.current_bet_size;
            state.committed[active_player] += cost_to_call + state.current_bet_size;
            state.current_bet_size *= 2;
            state.raises_this_round++;
        } else {
            state.pot += cost_to_call;
            state.committed[active_player] += cost_to_call;
        }
    }
    
    if(state.committed[0]==state.committed[1] && state.actions_this_round > 0) {
        if(state.round==0) {
            state.round = 1;
            state.committed[0] = state.committed[1] = 0;
            state.actions_this_round = 0;
            state.current_bet_size = 2.0;
            state.current_player = 0;
            state.raises_this_round = 0;
        } else {
            state.is_terminal = true;
        }
    } else {
        state.current_player ^= 1;
    }

    return state;
}