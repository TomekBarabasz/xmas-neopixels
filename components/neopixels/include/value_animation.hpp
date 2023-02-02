#pragma once
#include <cstdint>

namespace Neopixel
{
struct RandomGenerator;
enum ValueAnimationType : int8_t
{
    inc_wrapped = 0,
    inc_pingpong,
    inc_sinus,
    random,
    constant
};

template <typename T>
struct ValueAnimation_t
{
    virtual ~ValueAnimation_t(){}
    virtual T nextValue(RandomGenerator*) = 0;
};

template <typename T>
ValueAnimation_t<T> *loadValueAnimation(void*& data);

using U16ValueAnimation = ValueAnimation_t<uint16_t>;
using I16ValueAnimation = ValueAnimation_t<int16_t>;
}