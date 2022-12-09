#pragma once
#include <tuple>
#include <cstdint>
#include <cstddef>
#include <numeric>
#include <tuple>

namespace Neopixel
{
struct Subset
{
    uint16_t first; 
    uint8_t count; 
    int8_t dir;
};
struct Strips
{
    static Strips* loadFromBuffer(char*buffer,size_t size);
    uint16_t getTotalPixelsCount() const
    {
        uint16_t max_index = 0;
        for(int i=0;i<count;++i)
        {
            auto & s = element[i];
            auto idx = s.first + s.count;
            if (idx > max_index) max_index = idx;
        }
        return max_index;
    }
    uint16_t getLongestLine() const
    {
        uint16_t max_cnt = 0;
        for(int i=0;i<count;++i)
        {
            auto & s = element[i];
            if (s.count > max_cnt) max_cnt = s.count;
        }
        return max_cnt;
    }
    std::tuple<uint16_t*,int> makeIndicesMatrix() const;
    uint16_t count;
    Subset element[];
    //consecutive elements placed next
};
template <size_t N>
Strips* makeStrips(Subset (&su)[N])
{
    auto * raw = new uint8_t[sizeof(uint16_t) + N*sizeof(Subset)];
    Strips *pstrips = reinterpret_cast<Strips*>(raw);
    pstrips->count = N;
    memcpy(pstrips->element,su,sizeof(su));
    return pstrips;
}
void release(Strips*);
struct Neighbours
{
    uint16_t count;
    uint16_t index[];
    //consecutive elements placed next
};
struct NeighboursMatrix
{
    static NeighboursMatrix* fromStrips(const Strips*);
    const Neighbours & getNeighbours(int index) const
    {
        const uint16_t * p = data + index * elem_size;
        return *reinterpret_cast<const Neighbours*>(p);
    }
    Neighbours & getNeighbours(int index)
    {
        uint16_t * p = data + index * elem_size;
        return *reinterpret_cast<Neighbours*>(p);
    }
    uint16_t count,elem_size;
    uint16_t data[];
    //consecutive elements placed next
};
void release(NeighboursMatrix*);
template <typename T, typename U>
inline int find_closest(const T* vector, U size, T val, U* result)
{
    for(U i=0;i<size;++i,++vector)
    {
        if (*vector == val)
        {
            *result = i;
            return 1;
        }
        else if (*vector > val)
        {
            if (i > 0)
            {
                *result++ = i-1;
                *result = i;
                return 2;
            }
            else
            {
                *result = i;
                return 1;
            }
        }
    }
    *result = size-1;
    return 1;
}
}