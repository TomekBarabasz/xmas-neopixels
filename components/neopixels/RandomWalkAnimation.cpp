#include <cstdint>
#include <color.hpp>
#include <led_strip.hpp>
#include "RandomWalkAnimation.hpp"

namespace Neopixel
{
RandomWalkAnimation::RandomWalkAnimation(LedStrip *strip_, int datasize, void *data) : strip(strip_)
{
}
void RandomWalkAnimation::step()
{
    const auto size = strip->getLength();
    strip->fillPixelsRGB(0,size,{0,0,0});
    uint16_t delay_ms = 100;
    strip->refresh(true);
}
}