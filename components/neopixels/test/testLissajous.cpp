#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>
#include <LissajousAnimation.hpp>
#include <led_strip.hpp>
#include <collections.hpp>
#include <random.hpp>
#include <utils.hpp>
#include <math_utils.hpp>
#include <string>
#include <map>

using namespace Neopixel;
using namespace ::testing;
using ::testing::Return;
using ParamsMap = std::map<std::string,int>;

namespace
{
struct MockLedStrip : public LedStrip 
{
    MOCK_METHOD( int,  getLength,    (), (const));
    MOCK_METHOD( RGB*, getBuffer,    ());
    MOCK_METHOD( void, setPixelsRGB, (int first, int num, const RGB*));
    MOCK_METHOD( void, fillPixelsRGB,(int first, int num, const RGB&));
    MOCK_METHOD( void, setPixelsHSV, (int first, int num, const HSV*));
    MOCK_METHOD( void, refresh,      (bool wait));
    MOCK_METHOD( void, copyFrontToBack,());
    MOCK_METHOD( bool, waitReady,    (uint32_t timeout_ms));
    MOCK_METHOD( void, release,      ());
};

struct MockRandomGenerator : public RandomGenerator
{
    MOCK_METHOD( uint32_t, make_random, ());
    MOCK_METHOD( void,     make_random_n, (uint32_t *values, int length));
    MOCK_METHOD( void,     release, ());
};

template <typename T>
void encode_prm(const char *name,const ParamsMap& prms, void*& data, T default_value)
{
    auto it = prms.find(name);
    T p = it != prms.end() ? uint16_t(it->second) : default_value;
    encode<T>(data, p);
}

std::tuple<uint8_t*,int> encodeParams(const ParamsMap& prms)
{
    static uint8_t params[12];
    void *p = params;
    encode_prm<uint16_t>("delay_ms",prms,p,20);
    encode_prm<uint16_t>("fade_delay_ms",prms,p,60);

    return {params, (uint8_t*)p-params};
}

template <size_t N>
LissajousAnimation* makeAnimation(Subset (&su)[N], const ParamsMap& prms, MockLedStrip& led_strip, MockRandomGenerator& random)
{
    Strips *pstrips = makeStrips(su);
    auto [params,len] = encodeParams(prms);
    EXPECT_CALL(led_strip, getLength()).Times(1).WillOnce(Return(pstrips->count));
    auto *pa = new LissajousAnimation(&led_strip,len,params,pstrips,&random);
    release(pstrips);
    return pa;
}
}

TEST(Lissajous, create)
{
    MockLedStrip led_strip;
    MockRandomGenerator random;
    Subset su[] = {{0,10,1},{15,10,-1},{30,10,1}};
    LissajousAnimation *anim = makeAnimation(su,{},led_strip,random);
    delete anim;
}

TEST(Lissajous, getPoint1D_posdir)
{
    Subset s {10,5,1};
    //indices : 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(0,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s);
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(127,v);}
}

TEST(Lissajous, getPoint1D_negdir)
{    
    Subset s {10,5,-1};
    //indices : 14 13 12 11 10
    constexpr uint16_t nmax = 5;
    
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(0,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s);
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(127,v);}
}

TEST(Lissajous, getPoint1D_posdir_stepgr1)
{
    Subset s {10,5,1};
    //indices : 10 11 12 13 14
    constexpr uint16_t nmax = 4;
    /* pos -> idx
     * 0			1			2			3	nmax=4
     * idx
     * 0       1    |     2     |    3      4	n=5  d = (n-1) / (nmax-1) = (5-1)/(4-1)=4/3=1,33
     * 0       1    1.33        2.66        4
     */

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(255/3,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s); //1.498 = (1,127/256) = 1.498 * 1.333 = 1.997 = (1,254/256)
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(254,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(2,0),nmax,s);   //pos 2.0 - > local pos 2.66 -> 256 * .66 -> 170
    EXPECT_EQ(12,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(170,v);}
}

TEST(Lissajous, getPoint1D_posdir_stepless1)
{
    Subset s {10,5,1};
    //indices : 10 11 12 13 14
    constexpr uint16_t nmax = 7;
    /* pos -> idx
     * 0	1   2   3   4   5   6       nmax=7
     * idx
     * 0   1     2      3       4	    n=5  d = (n-1) / (nmax-1) = (5-1)/(7-1)=4/6=0.666
     * 0    1    1.33        2.66        4
     */

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);   //(1,0/256) * 0.66666 = 0.666666 = (0,170/256)
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(170,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s); //1.498 = (1,127/256) = 1.498 * 0.666 = 0.998 = (0,255/256)
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(255,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(2,0),nmax,s);   //pos (2,0/256) * 0.66666  -> 1.33333  = (1,85/256)
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(85,v);}
}

TEST(Lissajous, getPoint1D_negdir_stepgr1)
{
    Subset s {10,5,-1};
    //indices : 14 13 12 11 10
    constexpr uint16_t nmax = 4;
    /* pos -> idx
     * 0			1			2			3	nmax=4
     * idx
     * 0       1    |     2     |    3      4	n=5  d = (n-1) / (nmax-1) = (5-1)/(4-1)=4/3=1,33
     * 0       1    1.33        2.66        4
     */

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(255/3,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s); //1.498 = (1,127/256) = 1.498 * 1.333 = 1.997 = (1,254/256)
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(254,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(2,0),nmax,s);   //pos 2.0 - > local pos 2.66 -> 256 * .66 -> 170
    EXPECT_EQ(12,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(170,v);}
}

TEST(Lissajous, getPoint1D_negdir_stepless1)
{
    Subset s {10,5,-1};
    //indices : 14 13 12 11 10
    constexpr uint16_t nmax = 7;
    /* pos -> idx
     * 0	1   2   3   4   5   6       nmax=7
     * idx
     * 0   1     2      3       4	    n=5  d = (n-1) / (nmax-1) = (5-1)/(7-1)=4/6=0.666
     * 0    1    1.33        2.66        4
     */

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);   //(1,0/256) * 0.66666 = 0.666666 = (0,170/256)
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(170,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s); //1.498 = (1,127/256) = 1.498 * 0.666 = 0.998 = (0,255/256)
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(255,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(2,0),nmax,s);   //pos (2,0/256) * 0.66666  -> 1.33333  = (1,85/256)
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(85,v);}
}

TEST(Lissajous, getPoint2D_neqnmax)
{
    Subset su[] {{0,5,1},{5,5,-1},{10,5,1}};
    Strips *pstrips = makeStrips(su);
    //x=0 indices:  0  1  2  3  4
    //x=1 indices:  9  8  7  6  5
    //x=2 indices: 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
    {auto ip = pstrips->getPoint2D(makeFixpoint88(0,0),makeFixpoint88(0,0),nmax);
    EXPECT_EQ(0,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    {auto ip = pstrips->getPoint2D(makeFixpoint88(1,0),makeFixpoint88(0,0),nmax);
    EXPECT_EQ(9,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}
     
    
    {auto ip = pstrips->getPoint2D(makeFixpoint88(0,0),makeFixpoint88(1,0),nmax);
    EXPECT_EQ(1,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    {auto ip = pstrips->getPoint2D(makeFixpoint88(1,0),makeFixpoint88(1,0),nmax);
    EXPECT_EQ(8,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    delete pstrips;
}

TEST(Lissajous, getPoint2D_xmax)
{
    Subset su[] {{0,5,1},{5,5,-1},{10,5,1}};
    Strips *pstrips = makeStrips(su);
    //x=0 indices:  0  1  2  3  4
    //x=1 indices:  9  8  7  6  5
    //x=2 indices: 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
    {auto ip = pstrips->getPoint2D(makeFixpoint88(2,0),makeFixpoint88(0,0),nmax);
    EXPECT_EQ(10,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    {auto ip = pstrips->getPoint2D(makeFixpoint88(2,0),makeFixpoint88(1,0),nmax);
    EXPECT_EQ(11,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}
     
    
    {auto ip = pstrips->getPoint2D(makeFixpoint88(2,127),makeFixpoint88(2,0),nmax);
    EXPECT_EQ(12,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    {auto ip = pstrips->getPoint2D(makeFixpoint88(2,127),makeFixpoint88(2,127),nmax);
    EXPECT_EQ(12,ip.idx[0]);
    EXPECT_EQ(128,ip.value[0]);
    EXPECT_EQ(13,ip.idx[1]);
    EXPECT_EQ(127,ip.value[1]);
    EXPECT_EQ(2,ip.n_points);}

    delete pstrips;
}

TEST(Lissajous, getPoint2D_yconst)
{
    Subset su[] {{0,5,1},{5,5,-1},{10,5,1}};
    Strips *pstrips = makeStrips(su);
    //x=0 indices:  0  1  2  3  4
    //x=1 indices:  9  8  7  6  5
    //x=2 indices: 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
   {auto ip = pstrips->getPoint2D(makeFixpoint88(1,55),makeFixpoint88(1,0),nmax);
    EXPECT_EQ(8,ip.idx[0]);
    EXPECT_EQ(200,ip.value[0]);
    EXPECT_EQ(11,ip.idx[1]);
    EXPECT_EQ(55,ip.value[1]);
    EXPECT_EQ(2,ip.n_points);}

    delete pstrips;
}

TEST(Lissajous, getPoint2D_bilinear)
{
    Subset su[] {{0,5,1},{5,5,-1},{10,5,1}};
    Strips *pstrips = makeStrips(su);
    //x=0 indices:  0  1  2  3  4
    //x=1 indices:  9  8  7  6  5
    //x=2 indices: 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
   {auto ip = pstrips->getPoint2D(makeFixpoint88(1,55),makeFixpoint88(1,100),nmax);
    EXPECT_EQ(8,ip.idx[0]);
    EXPECT_EQ(200*155/256,ip.value[0]);

    EXPECT_EQ(7,ip.idx[1]);
    EXPECT_EQ(155,ip.value[1]);

    EXPECT_EQ(11,ip.idx[2]);
    EXPECT_EQ(200,ip.value[2]);

    EXPECT_EQ(12,ip.idx[3]);
    EXPECT_EQ(55*100/256,ip.value[3]);

    EXPECT_EQ(4,ip.n_points);}

    delete pstrips;
}