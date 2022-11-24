#include <gtest/gtest.h>
#include <tuple>
#include <RandomWalkAnimation.hpp>

using namespace ::testing;
using namespace Neopixel;

TEST(RandomWalk, Init) 
{
    LedStrip *p=nullptr;
    RandomWalkAnimation a(nullptr,0,nullptr);
}

TEST(RandomWalk, findClosest_equal)
{
    //neighbours are indices!
    int vector[] = {1,2,3};
    auto [a,b,n] = find_closest(vector,3,1);
    EXPECT_EQ(a,0);
    EXPECT_EQ(n,1);

    std::tie(a,b,n) = find_closest(vector,3,2);
    EXPECT_EQ(a,1);
    EXPECT_EQ(n,1);

    std::tie(a,b,n) = find_closest(vector,3,3);
    EXPECT_EQ(a,2);
    EXPECT_EQ(n,1);
}

TEST(RandomWalk, findClosest_middle)
{
    //neighbours are indices!
    int vector[] = {1,3,5};
    auto [a,b,n] = find_closest(vector,3,2);
    EXPECT_EQ(a,1);
    EXPECT_EQ(b,0);
    EXPECT_EQ(n,2);

    std::tie(a,b,n) = find_closest(vector,3,4);
    EXPECT_EQ(a,2);
    EXPECT_EQ(b,1);
    EXPECT_EQ(n,2);
}

TEST(RandomWalk, findClosest_outside)
{
    //neighbours are indices!
    int vector[] = {1,3,5};
    auto [a,b,n] = find_closest(vector,3,0);
    EXPECT_EQ(a,0);
    EXPECT_EQ(n,1);

    std::tie(a,b,n) = find_closest(vector,3,6);
    EXPECT_EQ(a,2);
    EXPECT_EQ(n,1);
}