#include <stdint.h>

template <typename T>
T min(T a, T b) { return a < b ? a : b; }

template <typename T>
T max(T a, T b) { return a > b ? a : b; }

template <typename T>
T saturated_sub(T a, T b) 
{
    return T(max(int(a)-int(b),0));
}
template <typename T>
T saturated_add(T a, T b) 
{
    return T(min(int(a)+int(b), int(T(-1))));
}
inline uint8_t scale8_video( uint8_t i, uint8_t scale)
{
    return (((int)i * (int)scale) >> 8) + ((i && scale) ? 1:0);
}
/* 16-bit sin(x) approximation: float s = sin(x) * 32767.0 +/- 0.69%
** @param theta input angle from 0-65535, 65535 is 2Pi
** @returns sin of theta, value between -32767 to 32767*/
int16_t sin_16b(uint16_t theta);

/* 8bit sin approximation: (sin(x) * 128.0) + 128 +/- 2%;
** @param theta input angle from 0-255, 255 is 2Pi
** @returns sin of theta, value between -127 and 128 */
int8_t  sin_8b(uint8_t theta);

int16_t cos_16b(uint16_t theta);
int8_t  cos_8b(uint8_t theta);
