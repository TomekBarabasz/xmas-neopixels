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
    struct Line
    {
        uint8_t state;
        int8_t position,length;
        uint8_t delay;
    };
public:
    DigitalRainAnimation(LedStrip *strip_, int datasize, void *data,const Strips*, RandomGenerator*);
    ~DigitalRainAnimation();
    void step() override;
    uint16_t get_delay_ms() override { return delay_ms; }
    void createRainLines();
    void restartLine(int idx);
    void moveLine(int idx) {rain_lines[idx].position -= 1;}
    bool drawLine(int idx);

    LedStrip* strip {nullptr};
    const Strips* pixelLines;
    RandomGenerator * rand;
    uint8_t longest_line;
    uint16_t totalPixels;
    uint16_t delay_ms;
    uint16_t hue;
    int8_t head_length;
    uint8_t tail_length_min, tail_length_max, tail_length_range;
    Line *rain_lines {nullptr};
    uint16_t *line_indices {nullptr};
    uint16_t indices_row_size {};
};
}
