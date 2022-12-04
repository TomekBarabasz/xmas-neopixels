#pragma once
#include <cstdint>

namespace Neopixel
{
    struct RandomGenerator
    {
        static RandomGenerator* createInstance();
        virtual uint32_t make_random() = 0;
        virtual void make_random_n(uint32_t *values, int length) = 0;
        virtual void release() = 0;
    protected:
        ~RandomGenerator(){}
    };
}
