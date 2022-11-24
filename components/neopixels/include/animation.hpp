#pragma once
namespace Neopixel
{
struct Animation
{
    static void main(void*param);
    virtual void step() = 0;
    virtual uint16_t get_delay_ms() = 0;
    virtual ~Animation(){}
};
}