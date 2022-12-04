#include <gtest/gtest.h>
#include "gmock/gmock.h"
#include <collections.hpp>
#include <utils.hpp>
#include <iostream>
#include <tuple>

using namespace Neopixel;

TEST(findClosest, equal)
{
    int results[2];
    //neighbours are indices!
    int vector[] = {1,2,3};
    auto n = find_closest(vector,3,1,results);
    EXPECT_EQ(results[0],0);
    EXPECT_EQ(n,1);

    n = find_closest(vector,3,2,results);
    EXPECT_EQ(results[0],1);
    EXPECT_EQ(n,1);

    n = find_closest(vector,3,3,results);
    EXPECT_EQ(results[0],2);
    EXPECT_EQ(n,1);
}

TEST(findClosest, middle)
{
    int results[2];
    //neighbours are indices!
    int vector[] = {1,3,5};
    auto n = find_closest(vector,3,2,results);
    EXPECT_EQ(results[0],0);
    EXPECT_EQ(results[1],1);
    EXPECT_EQ(n,2);

    n = find_closest(vector,3,4,results);
    EXPECT_EQ(results[0],1);
    EXPECT_EQ(results[1],2);
    EXPECT_EQ(n,2);
}

TEST(findClosest, outside)
{
    int results[2];
    //neighbours are indices!
    int vector[] = {1,3,5};
    auto n = find_closest(vector,3,0,results);
    EXPECT_EQ(results[0],0);
    EXPECT_EQ(n,1);

    n = find_closest(vector,3,6,results);
    EXPECT_EQ(results[0],2);
    EXPECT_EQ(n,1);
}

TEST(pow2, basic)
{
    constexpr uint16_t values[]   = {1,2,3,4,5,6,7,8, 9,10,11,12,13,14,15,16,17,18,19,20};
    constexpr uint16_t expected[] = {2,2,4,4,8,8,8,8,16,16,16,16,16,16,16,16,32,32,32,32};
    constexpr size_t length = sizeof(values)/sizeof(values[0]);
    for (int i=0;i<length;++i)
    {
        EXPECT_EQ(next_pow2(values[i]), expected[i]);
    }
}

void compareNeighbours(const NeighboursMatrix& m, int idx, std::initializer_list<uint16_t> values, bool print=false)
{
    auto & ne = m.getNeighbours(idx);
    if (print)
    {
        std::cout << "index " << idx << " neigbours ";
        for (int i=0; i<ne.count; ++i) std::cout << ne.index[i] << ' ';
        std::cout << " expected ";
        for (auto v : values)
        {
            std::cout << v << ' ';
        }
        std::cout << std::endl;
    }
    EXPECT_EQ(ne.count,values.size());
    auto *index = ne.index;
    for (auto v : values)
    {
        EXPECT_EQ(*index++,v);
    }
}

void printNeighbours(const NeighboursMatrix& m, int idx)
{
    auto & ne = m.getNeighbours(idx);
    std::cout << "index " << idx << " neigbours ";
    for (int i=0; i<ne.count; ++i) std::cout << ne.index[i] << ' ';
    std::cout << std::endl;
}

TEST(Neghbours, t3x3_uniform)
{
    /*    
    0 5 6
    1 4 7
    2 3 8
    */
    Subset su[] = {{0,3,1},{3,3,-1},{6,3,1}};
    Strips *pstrips = makeStrips(su);
    auto *m = NeighboursMatrix::fromStrips(pstrips);
    auto & M = *m;
    EXPECT_EQ(M.count,9);

    compareNeighbours(M, 0, {5,1});
    compareNeighbours(M, 1, {0,4,2});
    compareNeighbours(M, 2, {1,3});

    compareNeighbours(M, 3, {2,4,8});
    compareNeighbours(M, 4, {1,5,7,3});
    compareNeighbours(M, 5, {0,6,4});

    compareNeighbours(M, 6, {5,7});
    compareNeighbours(M, 7, {4,6,8});
    compareNeighbours(M, 8, {3,7});

    release(m);
    release(pstrips);
}

TEST(Neghbours, t3_nonuniform)
{
    /*    
    0 7 8
    1 6 9
    2 5 10
    - 4 11
    - 3
    */

    /* positions:
    0	|	 |	 2	 |   |	4
    0	1	 |	 2	 |   3	4
    0	|	1.3	 |  2.6	 |	4
    indices:
    0 	|	 |	 1 	 |	 |  2
    7 	6 	 |	 5 	 | 	 4  3
    8 	|	 9 	 |	 10	 | 11
    */
    Subset su[] = {{0,3,1},{3,5,-1},{8,4,1}};
    Strips *pstrips = makeStrips(su);
    auto *m = NeighboursMatrix::fromStrips(pstrips);
    auto & M = *m;
    EXPECT_EQ(M.count,12);

    //for (int i=0;i<12;++i) printNeighbours(M,i);

    compareNeighbours(M, 0, {7,1});
    compareNeighbours(M, 1, {0,5,2});
    compareNeighbours(M, 2, {1,3});

    compareNeighbours(M, 3, {2,4,11});
    compareNeighbours(M, 4, {1,2,5,10,11,3});
    compareNeighbours(M, 5, {1,6,9,10,4});
    compareNeighbours(M, 6, {0,1,7,8,9,5});
    compareNeighbours(M, 7, {0,8,6});

    compareNeighbours(M, 8, {7,9});
    compareNeighbours(M, 9, {6,5,8,10});
    compareNeighbours(M, 10, {5,4,9,11});
    compareNeighbours(M, 11, {3,10});

    release(m);
    release(pstrips);
}