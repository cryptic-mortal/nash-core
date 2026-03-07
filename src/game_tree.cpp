#include<iostream>
#include "game_def.hpp"
#include "dealer.hpp"
#include "evaluator.hpp"
#include "info_set.hpp"
#include "indexer.hpp"
#include "rng.hpp"
#include "game_tree.hpp"

Node* build_tree(Leduc::GameState state, Leduc::Indexer& indexer){
    Node* node = new Node();

    if(state.is_terminal){
        node->is_terminal = true;
        node->payoff = Leduc::get_payoff(state).first;
        return node;
    }

    node->is_terminal = false;
    node->is_chance = false;
    node->player = state.current_player;

    std::string key = Leduc::get_info_set_key(state);
    node->info_set_id = indexer.get_id(key);

    for(int a = 0; a < 3; a++){
        Leduc::Action act = static_cast<Leduc::Action>(a);

        if(act == Leduc::Action::BET_RAISE && state.raises_this_round>=4){
            node->action_children[a] = nullptr;
            continue;
        }

        Leduc::GameState next_state = Leduc::step(state,act);

        if(state.round == 0 && next_state.round == 1){
            Node* chance_node = new Node();
            chance_node->is_chance = true;

            for(int rank = 0; rank < 3; rank++){
                int used = 0;
                if(state.p1_card/2 == rank) used++;
                if(state.p2_card/2 == rank) used++;
                if(used < 2){
                    Leduc::GameState chance_state = next_state;
                    chance_state.public_card = (rank*2) + used;
                    chance_node->chance_children.push_back(build_tree(chance_state,indexer));
                }
            }
            node->action_children[a] = chance_node;

        }else{
            node->action_children[a] = build_tree(next_state,indexer);
        }
    }

    return node;
}