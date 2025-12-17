#include<cstdint>
#ifndef RNG_HPP
#define RNG_HPP

struct RNG{
    uint32_t state;

    RNG(uint32_t seed){
        if(seed == 0){
            seed = 1;
        }
        state = seed;
    }

    uint32_t next_random() {
        state = (state << 13)^state;
        state = (state >> 17)^state;
        state = (state << 5)^state;

        return state;
    }

    uint32_t next_int(uint32_t max){
        return next_random()%max;
    }
};
#endif
