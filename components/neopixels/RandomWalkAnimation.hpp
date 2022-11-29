#pragma once
#include <tuple>
#include "animation.hpp"

namespace Neopixel
{
struct LedStrip;
class RandomWalkAnimation : public Animation
{
public:
    RandomWalkAnimation(LedStrip *strip_, int datasize, void *data);
    ~RandomWalkAnimation();
    void step() override;
    uint16_t get_delay_ms() override { return delay_ms; }
private:    
    LedStrip* strip;
    uint16_t delay_ms;
};



}

extern "C"
{
    int find_closest_i(const int* vector, int size, int val, int result[2]);
    int find_closest_f(const float* vector, int size, float val, int result[2]);
}