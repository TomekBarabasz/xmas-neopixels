#include <cstdint>
#include <color.hpp>
#include <led_strip.hpp>
#include <collections.hpp>
#include <utils.hpp>
#include <random.hpp>
#ifndef UNIT_TEST
 #include <esp_log.h>
#else
 #define ESP_LOGI(tag,format,...)
#endif
#include <DigitalRainAnimation.hpp>

namespace Neopixel
{
DigitalRainAnimation::DigitalRainAnimation(LedStrip *strip, int datasize, void *data, const Strips* lines, RandomGenerator* rand) : 
    strip(strip),lines(lines),rand(rand)
{
    delay_ms    = decode_safe<uint16_t>(data,datasize,20);
    hue         = decode_safe<uint16_t>(data,datasize,120);
    head_length = decode_safe<uint8_t>(data,datasize,3);
    auto longestLine = lines->getLongestLine();
    tail_length_min = decode_safe<uint8_t>(data,datasize,longestLine/3);
    tail_length_max = decode_safe<uint8_t>(data,datasize,longestLine);

    totalPixels   = lines->getTotalPixelsCount();
    ESP_LOGI("drain", "delay %d, hue %d hlen %d tlen %d - %d",delay_ms, hue, head_length, tail_length_min, tail_length_max);
}
DigitalRainAnimation::~DigitalRainAnimation()
{
}
void DigitalRainAnimation::step()
{
    auto *p = strip->getBuffer();
    strip->refresh(true);
}
}