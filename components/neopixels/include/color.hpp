#pragma once
#include <cstdint>
#include <math_utils.hpp>

namespace Neopixel
{
struct RGB
{
    uint8_t r,g,b;
    void scale8(uint8_t scale)
    {
        uint16_t scale_fixed = scale + 1;
        r = (((uint16_t)r) * scale_fixed) >> 8;
        g = (((uint16_t)g) * scale_fixed) >> 8;
        b = (((uint16_t)b) * scale_fixed) >> 8;
    }
    const RGB& operator+=(const RGB& other)
    {
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }
    const RGB& operator+(const RGB& other)
    {
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }
    RGB mix(const RGB& other, uint8_t alpha) const
    {
        const auto alpha16 = uint16_t(alpha);
        const auto not_alpha16 = uint16_t(255) - alpha16;
        uint8_t r_ = (uint8_t) ((r * alpha16 + other.r * not_alpha16) >> 8);
        uint8_t g_ = (uint8_t) ((g * alpha16 + other.g * not_alpha16) >> 8);
        uint8_t b_ = (uint8_t) ((b * alpha16 + other.b * not_alpha16) >> 8);
        return { r_,g_,b_ };
    }
};
inline RGB sat_add(const RGB& a, const RGB& b)
{
    return { (uint8_t)min( uint16_t(a.r) + uint16_t(b.r), 255 ),
             (uint8_t)min( uint16_t(a.g) + uint16_t(b.g), 255 ),
             (uint8_t)min( uint16_t(a.b) + uint16_t(b.b), 255 ) };
}
inline void fade_all(RGB *leds, int size, uint8_t scale)
{
    for (int i=0;i<size;++i) {
        leds[i].scale8(scale);
    }
}
struct HSV
{
    enum class Hue {
        RED = 0,
        ORANGE = 32,
        YELLOW = 64,
        GREEN = 96,
        AQUA = 128,
        BLUE = 160,
        PURPLE = 192,
        PINK = 224
    };

    uint16_t h;
    uint8_t s,v;
    RGB toRGB() const
    {
        const uint16_t h_ = h % 360;
        const uint16_t region = h_ / 60;
        const uint16_t remainder = (h_ - (region * 60)) * (256/60);

        const uint8_t p = (v * (255 - s)) >> 8;
        const uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        const uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

        switch (region)
        {
            case 0: return {v,t,p};
            case 1: return {q,v,p};
            case 2: return {p,v,t};
            case 3: return {p,q,v};
            case 4: return {t,p,v};
            default:return {v,p,q};
        }
    }
};
}