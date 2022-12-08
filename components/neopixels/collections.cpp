#include <collections.hpp>
#include <cstdint>
#include <cstddef>
#include <utils.hpp>
#include <numeric>
#include <cstring>

namespace Neopixel
{
Strips* loadFromBuffer(char*buffer,size_t size)
{
    int nstrips = size / sizeof(Subset);
    uint8_t *raw_ptr = new uint8_t[ sizeof(Subset)*nstrips + 1];
    Strips *strips = reinterpret_cast<Strips*>(raw_ptr);
    strips->count = nstrips;
    for (int i=0;i<nstrips;++i)
    {
        auto & s = strips->element[i];
        s.first = *(reinterpret_cast<uint16_t*>(buffer)); buffer += 2;
        s.count = *(reinterpret_cast<uint8_t*>(buffer++));
        s.dir   = *(reinterpret_cast<uint8_t*>(buffer++));
    }
    return strips;
}
void release(Strips* s) { delete[] reinterpret_cast<uint8_t*>(s); }
void release(NeighboursMatrix* m) { delete[] reinterpret_cast<uint8_t*>(m); }
namespace
{
    int longestStrip(const Strips *s)
    {
        int longest = 0;
        for (int i=0;i<s->count;++i)
        {
            if (s->element[i].count > longest) {
                longest = s->element[i].count;
            }
        }
        return longest;
    }
    void initializeIndicesMatrix(const Strips* strips, uint16_t *indices, uint16_t row_size)
    {
        for (int i=0;i<strips->count;++i)
        {
            auto & s = strips->element[i];
            auto cnt = s.count;
            uint16_t idx = s.first;
            if (s.dir < 0) idx += s.count - 1;
            int pi = i * row_size;
            while (cnt-- >0 )
            {
                indices[pi++] = idx;
                idx += s.dir;
            }
        }
    }
    void initializePositionsMatrix(const Strips* strips, float* positions, uint16_t longest, uint16_t row_size)
    {
        for (int i=0; i<strips->count; ++i)
        {
            auto & s = strips->element[i];
            auto fcount = float(strips->element[i].count);
            int pi = i * row_size;
            float dv = float(longest-1) / (fcount-1);
            float p = 0.0;
            int cnt = s.count;
            while (cnt-- > 0)
            {
                positions[pi++] = p;
                p += dv;
            }
        }
    }
}
NeighboursMatrix* NeighboursMatrix::fromStrips(const Strips* strips)
{
    const uint16_t longest = longestStrip(strips);
    const auto row_size = next_pow2(longest);
    const size_t N = strips->count;
    const auto total_pixels = strips->getTotalPixelsCount();
    uint16_t *indices = new uint16_t[row_size * N];
    initializeIndicesMatrix(strips,indices,row_size);
    float *positions = new float[row_size * N];
    initializePositionsMatrix(strips,positions,longest,row_size);
    constexpr uint16_t MaxNeighboursCnt = 6;
    const size_t n_uint16 = 2 + total_pixels * (1 + MaxNeighboursCnt) ;
    auto *matrix = reinterpret_cast<NeighboursMatrix*>( new uint16_t[ n_uint16 ] );
    memset(matrix,0,n_uint16*sizeof(uint16_t));
    matrix->count = total_pixels;
    matrix->elem_size = MaxNeighboursCnt + 1;
    uint16_t * strip_indices   = indices;
    float    * strip_positions = positions;

    for (int i=0; i<N; ++i)
    {
        const uint16_t len = strips->element[i].count;
        uint16_t res[2];
        for (int j=0;j<len;++j)
        {
            auto & ne = matrix->getNeighbours(strip_indices[j]);
            ne.count = 0;
            const auto v = strip_positions[j];
            if (i>0)
            {
                const uint16_t len_l = strips->element[i-1].count;
                auto cnt =  find_closest(strip_positions - row_size,len_l,v,res);
                uint16_t *indices_l = strip_indices - row_size;
                if (cnt > 0) ne.index[ne.count++] = indices_l[ res[0] ];
                if (cnt > 1) ne.index[ne.count++] = indices_l[ res[1] ];
            }
            if (j>0)
            {
                ne.index[ne.count++] = strip_indices[j-1];
            }
            if (i<N-1)
            {
                const uint16_t len_r = strips->element[i+1].count;
                auto cnt =  find_closest(strip_positions + row_size,len_r,v,res);
                uint16_t *indices_r = strip_indices + row_size;
                if (cnt > 0) ne.index[ne.count++] = indices_r[ res[0] ];
                if (cnt > 1) ne.index[ne.count++] = indices_r[ res[1] ];
            }
            if (j<len-1)
            {
                ne.index[ne.count++] = strip_indices[j+1];
            }
        }
        strip_indices   += row_size;
        strip_positions += row_size;
    }
    delete[] indices;
    delete[] positions;
    return matrix;
}
}