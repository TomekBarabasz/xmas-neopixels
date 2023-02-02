#include <value_animation.hpp>
#include <utils.hpp>
#include <random.hpp>
#include <math_utils.hpp>

namespace Neopixel
{
template <typename T>
struct ConstAnimation : ValueAnimation_t<T>
{
    const T value;
    ConstAnimation(T value) : value(value){}
    T nextValue(RandomGenerator*) override
    {
        return value;
    }
};

template <typename T>
struct WrappedAnimation : ValueAnimation_t<T>
{
    using Signed_T = typename std::make_signed<T>::type;
    const T minv, maxv;
    Signed_T incv;
    T value;

    WrappedAnimation(T minv, T maxv,Signed_T incv) : minv(minv),maxv(maxv),incv(incv),value(minv){}
    T nextValue(RandomGenerator*) override
    {
        T ret = value;
        value += incv;
        if (value < minv) {
            value = maxv;
        }else if (value > maxv) {
            value = minv;
        }
        return ret;
    }
};

template <typename T>
struct PingpongAnimation : ValueAnimation_t<T>
{
    using Signed_T = typename std::make_signed<T>::type;
    const T minv, maxv;
    Signed_T incv;
    T value;

    PingpongAnimation(T minv,T maxv,Signed_T incv) : minv(minv),maxv(maxv),incv(incv),value(minv){}
    T nextValue(RandomGenerator*) override
    {
        T ret = value;
        value += incv;
        if (value < minv) {
            value = minv+1;
            incv = -incv;
        }else if (value > maxv) {
            value = maxv-1;
            incv = -incv;
        }
        return ret;
    }
};

template <typename T>
struct SinusAnimation : ValueAnimation_t<T>
{
    const T zlvl,ampl,omega;
    T time;

    SinusAnimation(T zlvl, T ampl, T omega) : zlvl(zlvl),ampl(ampl),omega(omega),time(0){}
    T nextValue(RandomGenerator*) override
    {
        time += 1;
        return zlvl + ampl * sin_16b(omega * time);
    }
};

template <typename T>
struct RandomAnimation : ValueAnimation_t<T>
{
    const T minv, deltav;

    RandomAnimation(T minv, T maxv) : minv(minv),deltav(maxv-minv+1){}
    T nextValue(RandomGenerator*rg) override
    {
        return minv+rg->make_random() % deltav;
    }
};

template <typename T>
ValueAnimation_t<T> *loadValueAnimation(void*& data)
{
    using Signed_T = typename std::make_signed<T>::type;
    int8_t type = decode<int8_t>(data);
    switch(type){
        case ValueAnimationType::inc_wrapped:{
            auto minv = decode<T>(data);
            auto maxv = decode<T>(data);
            auto inc = decode<Signed_T>(data);
            return new WrappedAnimation<T>(minv,maxv,inc);
        }
        case ValueAnimationType::inc_pingpong:{
            auto minv = decode<T>(data);
            auto maxv = decode<T>(data);
            auto inc  = decode<Signed_T>(data);
            return new PingpongAnimation<T>(minv,maxv,inc);
        }
        case ValueAnimationType::inc_sinus:{
            auto minv = decode<T>(data);
            auto maxv = decode<T>(data);
            auto omega = decode<Signed_T>(data);
            const T ampl = maxv-minv;
            const T zlvl = minv + ampl;
            return new SinusAnimation<T>(zlvl,ampl,omega);
        }
        case ValueAnimationType::random:{
            auto minv = decode<T>(data);
            auto maxv = decode<T>(data);
            return new RandomAnimation<T>(minv,maxv);
        }
        case ValueAnimationType::constant:{
            auto v = decode<T>(data);
            return new ConstAnimation<T>(v);
        }
        default:
            return nullptr;
    }
}
template U16ValueAnimation* loadValueAnimation<uint16_t>(void*&data);
template I16ValueAnimation* loadValueAnimation<int16_t>(void*&data);
}
