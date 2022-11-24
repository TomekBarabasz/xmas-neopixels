#pragma once
#include <tuple>
#include "animation.hpp"

namespace Neopixel
{
struct LedStrip;
class RandomWalkAnimation : public Animation
{
public:
    RandomWalkAnimation(LedStrip *strip_, int datasize, void *data);
    void step() override;
    uint16_t get_delay_ms() override { return delay_ms; }
private:    
    LedStrip* strip;
    uint16_t delay_ms;
};

template <typename T>
std::tuple<int,int,int> find_closest(const T* vector, int size, T val)
{
    for(int i=0;i<size;++i,++vector)
    {
        if (*vector == val)
        {
            return {i,-1,1};
        }
        else if (*vector > val)
        {
            return {i,i-1,i > 0 ? 2 : 1};
        }
    }
    return {size-1,-1,1};
}
}

extern "C"
{
    int find_closest(const float* vector, int size, float val, int result[2])
    {
        auto [a,b,n] = Neopixel::find_closest(vector,size,val);
        result[0] = a;
        result[1] = b;
        return n;
    }
}