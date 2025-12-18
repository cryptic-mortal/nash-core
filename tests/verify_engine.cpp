#include <iostream>
#include <string>
#include "rng.hpp"
#include "game_def.hpp"
#include "dealer.hpp"
#include "evaluator.hpp"
#include "game_utils.hpp"

std::string format_card(uint8_t card) {
    uint8_t rank = card >> 1;
    uint8_t suit = card & 1; 
    
    std::string r_str;
    if (rank == Leduc::JACK) r_str = "J";
    else if (rank == Leduc::QUEEN) r_str = "Q";
    else r_str = "K";
    
    return r_str + (suit ? "s" : "h");
}

std::string format_action(Leduc::Action action) {
    if (action == Leduc::Action::FOLD) return "FOLD";
    if (action == Leduc::Action::CHECK_CALL) return "CHECK/CALL";
    if (action == Leduc::Action::BET_RAISE) return "BET/RAISE";
    return "NONE";
}

int main() {
    RNG rng(1);
    const uint8_t NUM_GAMES = 5;
    printf("--------------------- GEOMETRIC LEDUC ENGINE TRACE ---------------------\n");
    printf("%-5s | %-10s | %-10s | %-10s | %s\n", "GAME", "POT", "P1 CHIPS", "P2 CHIPS", "ACTION LOG");
    printf("------------------------------------------------------------------------------------------------\n");
    for(uint8_t i = 0;i<NUM_GAMES;i++) {
        Leduc::GameState state = Leduc::deal_initial_state(rng);
        printf("G%-4d | %6.1f     | %6.1f     | %6.1f     | [DEAL] P1:%s P2:%s Pub:%s\n", i+1, state.pot, state.committed[0], state.committed[1], format_card(state.p1_card).c_str(), format_card(state.p2_card).c_str(), format_card(state.public_card).c_str());
        while(!state.is_terminal) {
            Leduc::Action action = get_random_move(state, rng);
            printf("      |            |            |            | > P%d %-5s", state.current_player + 1, format_action(action).c_str());
            state = Leduc::step(state, action);
            printf("\n      | %6.1f     | %6.1f     | %6.1f     |   (BetSz:%.0f Round:%d)\n",state.pot, state.committed[0], state.committed[1], state.current_bet_size, state.round);
        }
        float payoff = get_payoff(state);
        printf("------+------------+------------+------------+----------------------\n");
        printf("RESULT| P1: %+.1f   |            |            | Winner: %s\n", payoff, (payoff > 0 ? "P1" : (payoff < 0 ? "P2" : "Tie")));
        printf("\n");
    }
    return 0;
}