#pragma once
#include <tuple>
#include "animation.hpp"

namespace Neopixel
{
struct LedStrip;
struct Strips;
struct Subset;
struct RandomGenerator;
class FireAnimation : public Animation
{
public:
    FireAnimation(LedStrip *strip_, int datasize, void *data,const Strips*, RandomGenerator*);
    ~FireAnimation();
    void step() override;
    uint16_t get_delay_ms() override { return delay_ms; }

protected:
    void processSingleStrip(const Subset& s);
    LedStrip* strip = {nullptr};
    const Strips* lines;
    RandomGenerator * rand;
    uint16_t delay_ms;
    uint8_t sparking,cooling,direction;
    uint16_t totalPixels;
    uint8_t* heat = {nullptr};
};
}
