#include <cstdint>
#include <color.hpp>
#include <led_strip.hpp>
#include <collections.hpp>
#include <utils.hpp>
#include <random.hpp>
#include <RandomWalkAnimation.hpp>

namespace Neopixel
{
RandomWalkAnimation::RandomWalkAnimation(LedStrip *strip, int datasize, void *data, const Strips* strips, RandomGenerator* rand) : strip(strip),rand(rand)
{
    delay_ms      = decode_safe<uint16_t>(data,datasize,20);
    fade_delay_ms = decode_safe<uint16_t>(data,datasize,60);
    hue_min       = decode_safe<uint16_t>(data,datasize,0);
    hue_max       = decode_safe<uint16_t>(data,datasize,360);
    hue_inc       = decode_safe<int8_t>(data,datasize,10);
    hue_wrap      = decode_safe<uint8_t>(data,datasize,0);
    hue_fade      = decode_safe<uint8_t>(data,datasize,200);
    neighbours    = NeighboursMatrix::fromStrips(strips);
    totalPixels   = strips->getTotalPixelsCount();
    brightness    = new uint8_t[totalPixels];
    std::fill(brightness,brightness+totalPixels,0);
    current_position = rand->make_random() % totalPixels;
}
RandomWalkAnimation::~RandomWalkAnimation()
{
    if (neighbours != nullptr) 
    {
        release(neighbours);
        neighbours = nullptr;
    }
    if (brightness)
    {
        delete[] brightness;
        brightness = nullptr;
    }
}
uint16_t RandomWalkAnimation::getNextHue()
{
    current_hue += hue_inc;
    if (current_hue < hue_min)
    {
        if (hue_wrap)
        {
            current_hue = hue_max;
        }
        else
        {
            current_hue = hue_min;
            hue_inc = -hue_inc;
        }
    }
    else if (current_hue > hue_max)
    {
        if (hue_wrap)
        {
            current_hue = hue_min;
        }
        else
        {
            current_hue = hue_max;
            hue_inc = -hue_inc;
        }
    }
    return current_hue;
}
void RandomWalkAnimation::setCurrentPixel(uint16_t idx)
{
    current_position = idx;
    HSV hsv {hue_min,255,255};
    strip->setPixelsHSV(current_position,1,&hsv);
    brightness[current_position] = 255;
}
uint16_t RandomWalkAnimation::calcNextPosition()
{
    auto & ne = neighbours->getNeighbours(current_position);
    constexpr size_t n_prob = 8;
    uint16_t prob[ n_prob ];
    uint16_t acc_prob = 0;
    for (uint16_t i=0;i < ne.count;++i)
    {
        acc_prob += 255 - brightness[ ne.index[i] ];
        prob[i] = acc_prob;
    }
    if (acc_prob > 0)
    {
        uint16_t sel =  rand->make_random() % acc_prob;
        for (uint16_t i=0;i < ne.count;++i)
        {
            if (sel < prob[i]) {
                return ne.index[i];
            }
        }
        return ne.index[ne.count-1];
    }
    else
    {
        //all neighbours have 0 brightness
        return ne.index[ rand->make_random() % ne.count ];
    }
}
void RandomWalkAnimation::step()
{
    setCurrentPixel( calcNextPosition() );
    time_to_fade_ms += delay_ms;
    if (time_to_fade_ms > fade_delay_ms)
    {
        time_to_fade_ms -= fade_delay_ms;
        auto *p = strip->getBuffer();
        const uint16_t fade = hue_fade + 1;
        for (int i=0;i<totalPixels;++i,p++)
        {
            if (i != current_position) {
                p->scale8(hue_fade);
                brightness[i] = (brightness[i] * fade) >> 8;
            }            
        }
    }
    strip->refresh(true);
}
}