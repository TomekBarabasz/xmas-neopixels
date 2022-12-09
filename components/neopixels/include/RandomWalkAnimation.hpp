#pragma once
#include <tuple>
#include "animation.hpp"

namespace Neopixel
{
struct LedStrip;
struct NeighboursMatrix;
struct Strips;
struct RandomGenerator;
class RandomWalkAnimation : public Animation
{
public:
    RandomWalkAnimation(LedStrip *strip_, int datasize, void *data,const Strips*, RandomGenerator*);
    ~RandomWalkAnimation();
    void step() override;
    uint16_t get_delay_ms() override { return delay_ms; }

    void initNeighboursMatrix();
    int16_t getNextHue(int16_t);
    uint16_t calcNextPosition();
    void setCurrentPixel(uint16_t idx,uint16_t hue);

    LedStrip* strip = {nullptr};
    const Strips* lines;
    RandomGenerator * rand;
    NeighboursMatrix *neighbours = {nullptr};
    uint16_t delay_ms, fade_delay_ms;
    uint16_t hue_min,hue_max;
    int8_t hue_inc;
    uint8_t hue_wrap, hue_fade;
    uint16_t totalPixels;

    int16_t current_hue = {0};
    uint8_t* brightness = {nullptr};
    uint16_t current_position = 0;
    uint16_t time_to_fade_ms = {0};
};

}
