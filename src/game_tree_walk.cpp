#include <iostream>
#include <set>
#include "game_def.hpp"
#include "info_set.hpp"
#include "rng.hpp"
#include "dealer.hpp"

std::set<std::string> info_sets;

void walk(Leduc::GameState state) {
    std::string info = Leduc::get_info_set_key(state);
    info_sets.insert(info); 
    if(state.is_terminal) return;
    for(int i = 0;i<3;i++) {
        Leduc::Action act = static_cast<Leduc::Action>(i);
        if(act==Leduc::Action::BET_RAISE && state.raises_this_round>=4) continue;
        Leduc::GameState next_state = Leduc::step(state,act);
        if(state.round==next_state.round) walk(next_state);
        else {
            next_state.public_card = Leduc::JACK*2;
            walk(next_state);
            next_state.public_card = Leduc::QUEEN*2;
            walk(next_state);
            next_state.public_card = Leduc::KING*2;
            walk(next_state);
        }
    }
}

int main() {
    RNG rng(100);
    Leduc::GameState state = Leduc::deal_initial_state(rng);
    state.p1_card = Leduc::JACK*2;
    state.p2_card = Leduc::JACK*2;
    walk(state);
    state.p1_card = Leduc::KING*2;
    state.p2_card = Leduc::KING*2;
    walk(state);
    state.p1_card = Leduc::QUEEN*2;
    state.p2_card = Leduc::QUEEN*2;
    walk(state);
    std::cout<<"Total INFO SETS: "<<info_sets.size();
    return 0;
}