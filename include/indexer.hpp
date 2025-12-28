#include <unordered_map>
#include <vector>
#include <string>
#include <set>
#include "game_def.hpp"
#include "info_set.hpp"
#include "dealer.hpp"
#include "rng.hpp"
#ifndef INDEXER_HPP
#define INDEXER_HPP

namespace Leduc {

class Indexer {
    std::unordered_map<std::string,int> key_to_id;
    std::vector<std::string> id_to_key;
    std::set<std::string> sorted_key;
    void walk(Leduc::GameState state) {
        std::string info = Leduc::get_info_set_key(state);
        sorted_key.insert(info); 
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
    public:
    Indexer() {
        RNG rng(100);
        for(int r1 = 0; r1 < 3; r1++){
            for(int r2 = 0; r2 < 3; r2++){
                Leduc::GameState state = Leduc::deal_initial_state(rng);

                state.p1_card = r1*2;
                state.p2_card = (r1 == r2) ? (r2*2 + 1) : (r2*2);

                state.round = 0;
                state.raises_this_round = 0;
                state.history_len = 0;

                walk(state);
            }
        }
        for(auto key : sorted_key) {
            id_to_key.push_back(key);
            key_to_id[key] = id_to_key.size()-1;
        }
    }
    int get_id(std::string key) {
        if(key_to_id.count(key)>0) return key_to_id[key];
        else return -1;
    }
    std::string get_key(int id) {
        return id_to_key[id];
    }
    int get_size() {
        return id_to_key.size();
    }
};

}


#endif