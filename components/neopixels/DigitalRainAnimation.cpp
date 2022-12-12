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
    strip(strip),pixelLines(lines),rand(rand)
{
    delay_ms    = decode_safe<uint16_t>(data,datasize,20);
    hue_min     = decode_safe<uint16_t>(data,datasize,0);
    hue_max     = decode_safe<uint16_t>(data,datasize,360);
    hue_inc     = decode_safe<int8_t>(data,datasize,0);
    hue_mode    = decode_safe<uint8_t>(data,datasize,0);
    color_value = decode_safe<int16_t>(data,datasize,255);
    head_length = decode_safe<int8_t>(data,datasize,3);
    longest_line = pixelLines->getLongestLine();
    tail_length_min = decode_safe<uint8_t>(data,datasize,longest_line/3);
    tail_length_max = decode_safe<uint8_t>(data,datasize,longest_line);
    tail_length_range = tail_length_max - tail_length_min;
    totalPixels   = pixelLines->getTotalPixelsCount();
    ESP_LOGI("drain", "delay %d, hue %d hlen %d tlen %d - %d",delay_ms, hue, head_length, tail_length_min, tail_length_max);

    auto [ptr,rsize]  = pixelLines->makeIndicesMatrix();
    line_indices = ptr;
    indices_row_size = rsize;
    hue = hue_min;
    createRainLines();
}
DigitalRainAnimation::~DigitalRainAnimation()
{
    if (rain_lines) 
    {
        delete[] rain_lines;
        rain_lines = nullptr;
    }
    if (line_indices) 
    {
        delete[] line_indices;
        line_indices = nullptr;
    }
}
void DigitalRainAnimation::createRainLines()
{
    const size_t nLines = pixelLines->count;
    rain_lines = new Line[nLines];
    for (int i=0;i<nLines;++i)
    {
        restartLine(i);
    }
}
void DigitalRainAnimation::restartLine(int idx)
{
    auto & line = rain_lines[idx];
    const auto pos = static_cast<uint8_t>(pixelLines->element[idx].count-1);
    const auto tail_length = static_cast<uint8_t>( tail_length_min + rand->make_random()%tail_length_range);
    const auto delay = static_cast<uint8_t>( rand->make_random()%longest_line);

    line.state = 0;
    line.position = pos;
    line.length = tail_length;
    line.delay = delay;
    line.hue = hue = getNextHue(hue);
}
bool DigitalRainAnimation::drawLine(int idx)
{
    static constexpr auto zero = static_cast<int8_t>(0);
    const auto *li = line_indices + idx*indices_row_size;
    auto & line = rain_lines[idx];
    const int8_t line_size = pixelLines->element[idx].count;
    RGB white {255,255,color_value};
    const int8_t hmin = max(line.position,zero);
    const int8_t hmax = clamp(static_cast<int8_t>(line.position + head_length),zero,line_size);
    const auto cnt_white = hmax-hmin;
    if (cnt_white > 0) {
        strip->fillPixelsRGB(li[hmin],cnt_white,white);
    }
    /*for (int i=hmin;i<hmax;++i) {
        strip->setPixelsRGB(li[i],1,&white);
    }*/
    const int8_t tstart = line.position + head_length;
    const int8_t tend   = tstart + line.length;
    const int8_t tmin = clamp(tstart,zero,line_size);
    const int8_t tmax = clamp(tend,  zero,line_size);

    int8_t toffset = tmin - tstart;
    bool result = false;
    if (tmax > 0)
    {
        uint16_t dv = color_value / line.length;
        HSV tail_color {hue, 255, static_cast<uint8_t>(color_value - dv*toffset)};
        for(int i=tmin;i<tmax;++i,++toffset)
        {
            strip->setPixelsHSV(li[i],1,&tail_color);
            tail_color.v -= dv;
        }
        result = true;
    }
    if (tmax >=0 && tmax < line_size) {
        RGB black {0,0,0};
        strip->fillPixelsRGB(li[tmax],1,black);
    }
    return result;
}
int16_t DigitalRainAnimation::getNextHue(int16_t hue)
{
    if (hue_mode != 2)
    {
        hue += hue_inc;
        if (hue < hue_min)
        {
            if (1==hue_mode)
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
            if (1==hue_mode)
            {
                hue = hue_min;
            }
            else
            {
                hue = hue_max;
                hue_inc = -hue_inc;
            }
        }
    }
    else
    {
        hue = hue_min + rand->make_random() % (hue_max-hue_min);
    }
    return hue;
}
void DigitalRainAnimation::step()
{
    //auto *p = strip->getBuffer();
    //strip->refresh(true);
    for (int i=0; i<pixelLines->count; ++i)
    {
        auto & line = rain_lines[i];
        if (0==line.state)
        {
            if (0 == line.delay) line.state = 1;
            else --line.delay;
        }
        else
        {
            moveLine(i);
            if (!drawLine(i)) {
                restartLine(i);
            }
        }
    }
}
}