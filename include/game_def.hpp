#include <cstdint>
#ifndef GAME_DEF_HPP
#define GAME_DEF_HPP

namespace Leduc {
    constexpr uint8_t JACK = 0;
    constexpr uint8_t QUEEN = 1;
    constexpr uint8_t KING = 2;
    constexpr uint8_t SUIT_0 = 0;
    constexpr uint8_t SUIT_1 = 1;
};

enum class Action : uint8_t {
    FOLD = 0,
    CHECK_CALL = 1,
    BET_RAISE = 2,
    NONE = 255,
};

struct GameState {
    float pot;
    Action history[16];
    uint8_t history_len;
    uint8_t p1_card;
    uint8_t p2_card;
    uint8_t public_card;
    bool current_player;
    bool round;
    bool is_terminal;
};

#endif