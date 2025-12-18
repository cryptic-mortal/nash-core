#include <cstdint>
#ifndef GAME_DEF_HPP
#define GAME_DEF_HPP

namespace Leduc {

    constexpr uint8_t MAX_RAISES_PER_ROUND = 4;

    constexpr uint8_t JACK = 0;
    constexpr uint8_t QUEEN = 1;
    constexpr uint8_t KING = 2;
    constexpr uint8_t SUIT_0 = 0;
    constexpr uint8_t SUIT_1 = 1;


    enum class Action : uint8_t {
        FOLD = 0,
        CHECK_CALL = 1,
        BET_RAISE = 2,
        NONE = 255,
    };

    struct GameState {
        float pot;
        float committed[2];
        float current_bet_size;
        Action history[16];
        uint8_t history_len;
        uint8_t p1_card;
        uint8_t p2_card;
        uint8_t public_card;
        uint8_t current_player;
        uint8_t round;
        uint8_t actions_this_round;
        uint8_t raises_this_round;
        bool is_terminal;
    };

    GameState step(GameState state, Action action);
};

#endif