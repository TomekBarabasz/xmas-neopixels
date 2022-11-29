#pragma once
#include <tuple>
#include <cstdint>
#include <cstddef>

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
    uint16_t count;
    Subset element[];
    //consecutive elements placed next
};
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