#pragma once
#include <tuple>
#include "animation.hpp"

namespace Neopixel
{
struct LedStrip;
struct NeighboursMatrix;
struct Strips;
struct RandomGenerator;
class DigitalRainAnimation : public Animation
{
public:
    DigitalRainAnimation(LedStrip *strip_, int datasize, void *data,const Strips*, RandomGenerator*);
    ~DigitalRainAnimation();
    void step() override;
    uint16_t get_delay_ms() override { return delay_ms; }

    LedStrip* strip = {nullptr};
    const Strips* lines;
    RandomGenerator * rand;
    uint8_t longest_line;
    uint16_t totalPixels;
    uint16_t delay_ms;
    uint16_t hue;
    uint8_t head_length, tail_length_min, tail_length_max;
};
}
