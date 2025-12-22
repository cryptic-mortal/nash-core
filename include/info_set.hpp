#include <unordered_map>
#include <string>
#include <vector>
#include "game_def.hpp"
#ifndef INFO_SET_HPP
#define INFO_SET_HPP

namespace Leduc {

    class CFRNode {
    public:
        float regret_sum[3] = {0.0,0.0,0.0};
        float strategy_sum[3] = {0.0,0.0,0.0};
        std::vector<float> get_strategy() {
            std::vector<float> strategy(3);
            float sum = 0.0;
            for(int i = 0;i<3;i++) {
                strategy[i] = regret_sum[i] > 0 ? regret_sum[i] : 0.0;
                sum += strategy[i];
            }
            if(sum!=0) for(int i = 0;i<3;i++) strategy[i] /= sum;
            else for(int i = 0;i<3;i++) strategy[i] = 1.0f/3.0f;
            return strategy;
        }
    };

    using CFRMap = std::unordered_map<std::string,CFRNode>;

    std::string get_info_set_key(const Leduc::GameState& state) {
        std::string s = "";
        
        uint8_t rank;
        if(state.current_player) rank = state.p2_card >> 1;
        else rank = state.p1_card >> 1;
        if(rank==0) s += "J:";
        else if(rank==1) s += "Q:";
        else s += "K:";

        if(state.round) {
            uint8_t public_card_rank = state.public_card >> 1;
            if(public_card_rank==0) s += "J:";
            else if(public_card_rank==1) s += "Q:";
            else s += "K:";
        } else s += "*:";
        
        for (int i = 0; i < state.history_len; i++) {
            if(state.history[i]==Leduc::Action::FOLD) s += "f";
            else if(state.history[i]==Leduc::Action::CHECK_CALL) s += "c";
            else s += "r";
        }
        return s;
    }
}
#endif