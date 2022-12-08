#include <cstdint>
#include <color.hpp>
#include <led_strip.hpp>
#include <collections.hpp>
#include <utils.hpp>
#include <random.hpp>
#include <esp_log.h>
#include <RandomWalkAnimation.hpp>

namespace Neopixel
{
RandomWalkAnimation::RandomWalkAnimation(LedStrip *strip, int datasize, void *data, const Strips* lines, RandomGenerator* rand) : 
    strip(strip),lines(lines),rand(rand)
{
    delay_ms      = decode_safe<uint16_t>(data,datasize,1000);
    fade_delay_ms = decode_safe<uint16_t>(data,datasize,2000);
    hue_min       = decode_safe<uint16_t>(data,datasize,0);
    hue_max       = decode_safe<uint16_t>(data,datasize,360);
    hue_inc       = decode_safe<int8_t>(data,datasize,10);
    hue_wrap      = decode_safe<uint8_t>(data,datasize,0);
    hue_fade      = decode_safe<uint8_t>(data,datasize,200);
    //neighbours    = NeighboursMatrix::fromStrips(lines);
    totalPixels   = lines->getTotalPixelsCount();
    ESP_LOGI("rwanim", "delay %d, fade delay %d",delay_ms, fade_delay_ms);
    ESP_LOGI("rwanim", "hue min %d max %d inc %d wrap %d", hue_min, hue_max, hue_inc, hue_wrap);
    brightness    = new uint8_t[totalPixels];
    std::fill(brightness,brightness+totalPixels,0);
    current_position = rand->make_random() % totalPixels;
    current_hue = hue_min;
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
int16_t RandomWalkAnimation::getNextHue(int16_t hue)
{
    hue += hue_inc;
    if (hue < hue_min)
    {
        if (hue_wrap)
        {
            hue = hue_max;
        }
        else
        {
            hue = hue_min;
            hue_inc = -hue_inc;
        }
    }
    else if (hue > hue_max)
    {
        if (hue_wrap)
        {
            hue = hue_min;
        }
        else
        {
            hue = hue_max;
            hue_inc = -hue_inc;
        }
    }
    return hue;
}
void RandomWalkAnimation::setCurrentPixel(uint16_t idx, uint16_t hue)
{
    current_position = idx;
    HSV hsv {hue,255,255};
    strip->setPixelsHSV(current_position,1,&hsv);
    brightness[current_position] = 255;
}
uint16_t RandomWalkAnimation::calcNextPosition()
{
    //char txt_neighbours[32];
    //char txt_brightnes[32];
    //char txt_prob[32];

    auto & ne = neighbours->getNeighbours(current_position);
    if (ne.count < 2 || ne.count > 6)
    {
        ESP_LOGI("rwanim", "invalid ne.count %d for position %d",ne.count,current_position);
        return rand->make_random() % totalPixels;
    }
    constexpr size_t n_prob = 8;
    uint16_t prob[ n_prob ];
    uint16_t acc_prob = 0;
    for (uint16_t i=0;i < ne.count;++i)
    {
        acc_prob += 255 - brightness[ ne.index[i] ];
        prob[i] = acc_prob;
    }
    /*
    auto *pn = txt_neighbours;
    auto *pb = txt_brightnes;
    auto *pp = txt_prob;
    for (int i=0;i<ne.count;++i) 
    {
        const auto ip = ne.index[i];
        pn = int_to_string(pn,ip); *pn++ = ' ';
        pb = int_to_string(pb,brightness[ip]); *pb++ = ' ';
        pp = int_to_string(pp,prob[i]); *pp++ =' ';
    }
    *pn++ = 0;
    *pb++ = 0;
    *pp++ = 0;
    uint16_t sel =  rand->make_random() % acc_prob;
    ESP_LOGI("rwanim", "step : pos %d ne %s br %s pr %s acc prob %d sel %d",current_position, txt_neighbours, txt_brightnes, txt_prob, acc_prob, sel);
    */
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
    if (!neighbours) 
    {
        neighbours = NeighboursMatrix::fromStrips(lines);
    }
    auto np = calcNextPosition();
    current_hue = getNextHue(current_hue);
    setCurrentPixel( np, current_hue);
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