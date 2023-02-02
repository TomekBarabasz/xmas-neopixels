#include <math_utils.hpp>

int16_t sin_16b(uint16_t theta)
{
    constexpr uint16_t base[] = { 0, 6393, 12539, 18204, 23170, 27245, 30273, 32137 };
    constexpr uint8_t slope[] = { 49, 48, 44, 38, 31, 23, 14, 4 };

    uint16_t offset = (theta & 0x3FFF) >> 3; // 0..2047
    if( theta & 0x4000 ) offset = 2047 - offset;

    uint8_t section = offset / 256; // 0..7
    uint16_t b   = base[section];
    uint8_t  m   = slope[section];

    uint8_t secoffset8 = (uint8_t)(offset) / 2;

    uint16_t mx = m * secoffset8;
    int16_t  y  = mx + b;

    if( theta & 0x8000 ) y = -y;

    return y;
}

int8_t sin_8b(uint8_t theta)
{
    constexpr uint8_t b_m16_interleave[] = { 0, 49, 49, 41, 90, 27, 117, 10 };
    uint8_t offset = theta;
    if( theta & 0x40 ) {
        offset = (uint8_t)255 - offset;
    }
    offset &= 0x3F; // 0..63

    uint8_t secoffset  = offset & 0x0F; // 0..15
    if( theta & 0x40) ++secoffset;

    uint8_t section = offset >> 4; // 0..3
    uint8_t s2 = section * 2;
    const uint8_t* p = b_m16_interleave;
    p += s2;
    uint8_t b   =  *p;
    ++p;
    uint8_t m16 =  *p;

    uint8_t mx = (m16 * secoffset) >> 4;

    int8_t y = mx + b;
    if( theta & 0x80 ) y = -y;

    return y;
}
int16_t cos_16b(uint16_t theta)
{
    return sin_16b( theta + 16384);
}
int8_t  cos_8b(uint8_t theta)
{
    return sin_8b( theta + 64);
}