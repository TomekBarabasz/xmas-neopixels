#pragma once
#include <animation.hpp>
#include <cstdint>

namespace Neopixel
{
struct LedStrip;
struct Reel100 : Animation
{
    Reel100(LedStrip *strip_, int data_size, void *data);
    void step() override;
    uint16_t get_delay_ms() { return hue_update_delay_ms;}

protected: 
    void rainbow();
    void addGlitter(uint8_t chance);
    void rainbowWithGlitter();
    void confetti();

protected:
    LedStrip *strip;
    uint16_t hue_update_delay_ms;
    uint16_t anim_update_delay_s;
    uint8_t hue_inc;
    uint8_t glitter_chance;
    uint8_t fade = 245;
    uint8_t anim_idx = 0;
    uint16_t hue;
    uint16_t size;
    int32_t anim_time;
};
}