#include <cstdint>
#include <color.hpp>
#include <led_strip.hpp>
#include <collections.hpp>
#include <utils.hpp>
#include <math_utils.h>
#include <random.hpp>
#include <cstring>
#ifndef UNIT_TEST
 #include <esp_log.h>
#else
 #define ESP_LOGI(tag,format,...)
#endif
#include <FireAnimation.hpp>

namespace Neopixel
{
RGB HeatColor(uint8_t temperature)
{
    // Scale 'heat' down from 0-255 to 0-191,
    // which can then be easily divided into three
    // equal 'thirds' of 64 units each.
    const uint8_t t192 = scale8_video( temperature, 191);

    // calculate a value that ramps up from
    // zero to 255 in each 'third' of the scale.
    const uint8_t heatramp = (t192 & 0x3F) << 2; // 0..63, scale up to 0..252

    // now figure out which third of the spectrum we're in:
    if( t192 & 0x80) {
        // we're in the hottest third
        return {255,255,heatramp};
    } else if( t192 & 0x40 ) {
        // we're in the middle third
        return {255,heatramp, 0};
    } else {
        // we're in the coolest third
        return {heatramp, 0, 0};
    }
}
FireAnimation::FireAnimation(LedStrip *strip, int datasize, void *data, const Strips* lines, RandomGenerator* rand) : 
    strip(strip),lines(lines),rand(rand)
{
    delay_ms = decode<uint16_t>(data);
    cooling = decode<uint8_t>(data);
    sparking = decode<uint8_t>(data);
    direction = decode<uint8_t>(data);

    totalPixels = strip->getLength();
    heat = new uint8_t[totalPixels];
    memset(heat,0,totalPixels*sizeof(uint8_t));
    ESP_LOGI("Fire-animation", "Fire animation : delay %d cooling %d sparking %d direction %d", delay_ms, cooling, sparking, direction);
}
FireAnimation::~FireAnimation()
{
    delete[] heat;
    heat = nullptr;
}
void FireAnimation::step()
{
    for (int i=0;i<lines->count;++i)
    {
        processSingleStrip(lines->element[i]);
    }
    // Map from heat cells to LED colors
    for( int j = 0; j < totalPixels; j++) 
    {
        strip->fillPixelsRGB(j, 1, HeatColor(heat[j]));
    }
    strip->refresh();
}
void FireAnimation::processSingleStrip(const Subset &s)
{
    const auto length = s.count;
    const uint8_t cooling_factor = (cooling*10) / length + 2;

    const auto first =  direction ? s.first : s.first + length - 1;
    const auto inc   =  direction ? s.dir : -(s.dir);
    const auto last  = !direction ? s.first : s.first + length - 1;
    const auto rinc  = !direction ? s.dir : -(s.dir);
    
    uint32_t rnd = 0;
    // Step 1.  Cool down every cell
    for(int i=0,idx=first;i<length;++i,idx+=inc)
    {
        if (0==rnd) rnd = rand->make_random();
        heat[idx] = saturated_sub(heat[idx], uint8_t((rnd&0xFF) % cooling_factor));
        rnd >>= 8;
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for(int k=length-1,idx=last; k >= 2; k--,idx+=rinc)
    {
        heat[idx] = (heat[idx - 1] + heat[idx - 2] + heat[idx - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    rnd = rand->make_random();
    if( (rnd&0xFF) < sparking ) 
    {
        rnd >>= 8;
        const uint16_t pos = uint16_t(rnd&0xFFFF) % length;
        const uint16_t idx = first + inc*pos;
        rnd >>= 16;
        const uint8_t const_add = 160;
        const uint8_t random_add = 255-const_add;
        heat[idx] = saturated_add( heat[idx], uint8_t(const_add + uint8_t(rnd&0xFF)%random_add));
    }
}
}
