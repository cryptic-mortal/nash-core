#ifndef GAME_TREE_HPP
#define GAME_TREE_HPP

#include<cstdint>
#include<vector>


struct Node{
    bool is_terminal;
    bool is_chance;
    uint8_t player;
    int info_set_id;
    float payoff;

    Node* action_children[3];
    std::vector<Node*> chance_children;

    Node(){
        is_terminal = false;
        player = 0;
        info_set_id = -1;
        payoff = 0.0f;
        for (int i = 0; i<3; i++){
            action_children[i] = nullptr;
        }
    }
};

#endif