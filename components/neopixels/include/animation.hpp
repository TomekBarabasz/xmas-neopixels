#pragma once
#include <cstdint>

namespace Neopixel
{
struct LedStrip;
struct Strips;
struct RandomGenerator;
struct Animation
{
    static Animation* create(LedStrip*strip,int animation_id, void* data, const Strips*,RandomGenerator*);
    static void main(void*param);
    virtual void step() = 0;
    virtual uint16_t get_delay_ms() = 0;
    virtual ~Animation(){}
};

}