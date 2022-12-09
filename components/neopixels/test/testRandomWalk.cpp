#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>
#include <RandomWalkAnimation.hpp>
#include <led_strip.hpp>
#include <collections.hpp>
#include <random.hpp>
#include <utils.hpp>
#include <string>
#include <map>

using namespace Neopixel;
using namespace ::testing;
using ::testing::Return;
using ParamsMap = std::map<std::string,int>;

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
namespace
{
std::tuple<uint8_t*,int> encodeParams(uint16_t delay_ms,uint16_t fade_delay_ms,uint16_t hue_min,uint16_t hue_max, int8_t hue_inc,uint8_t hue_wrap, uint8_t hue_fade)
{
    static uint8_t params[12];
    void *p = params;
    encode<uint16_t>(p, delay_ms);
    encode<uint16_t>(p, fade_delay_ms);
    encode<uint16_t>(p, hue_min);
    encode<uint16_t>(p, hue_max);
    encode<int8_t>  (p, hue_inc);
    encode<uint8_t> (p, hue_wrap);
    encode<uint8_t> (p, hue_fade);
    return {params, (uint8_t*)p-params};
}
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
    encode_prm<uint16_t>("hue_min",prms,p,0);
    encode_prm<uint16_t>("hue_max",prms,p,360);
    encode_prm<int8_t>("hue_inc",prms,p,5);
    encode_prm<uint8_t>("hue_wrap",prms,p,0);
    encode_prm<uint8_t>("hue_fade",prms,p,200);

    return {params, (uint8_t*)p-params};
}
template <size_t N>
RandomWalkAnimation* makeAnimation(Subset (&su)[N], const ParamsMap& prms, MockLedStrip& led_strip, MockRandomGenerator& random)
{
    Strips *pstrips = makeStrips(su);
    EXPECT_CALL(random, make_random()).Times(1).WillOnce(Return(0));
    auto [params,len] = encodeParams(prms);
    auto *pa = new RandomWalkAnimation(&led_strip,len,params,pstrips,&random);
    release(pstrips);
    return pa;
}
}
TEST(RandomWalk, getNewHue_nowrap)
{
    Subset su[] = {{0,3,1},{3,3,-1},{6,3,1}};
    MockLedStrip led_strip;
    MockRandomGenerator random;
    
    RandomWalkAnimation *anim = makeAnimation(su,{},led_strip,random);

    anim->hue_min = 10;
    anim->hue_max = 100;
    anim->hue_wrap = 0;

    anim->hue_inc = 5;    
    auto next_hue = anim->getNextHue(95);
    EXPECT_EQ(next_hue,100);
    
    next_hue = anim->getNextHue(100);
    EXPECT_EQ(next_hue,100);
    EXPECT_EQ(anim->hue_inc,-5);

    anim->hue_inc = -5;
    next_hue = anim->getNextHue(15);
    EXPECT_EQ(next_hue,10);

    next_hue = anim->getNextHue(10);
    EXPECT_EQ(next_hue,10);
    EXPECT_EQ(anim->hue_inc,5);

    delete anim;
}
TEST(RandomWalk, getNewHue_wrap)
{
    Subset su[] = {{0,3,1},{3,3,-1},{6,3,1}};
    MockLedStrip led_strip;
    MockRandomGenerator random;
    
    RandomWalkAnimation *anim = makeAnimation(su,{},led_strip,random);

    anim->hue_min = 10;
    anim->hue_max = 100;
    anim->hue_wrap = 1;

    anim->hue_inc = 5;
    auto next_hue = anim->getNextHue(95);
    EXPECT_EQ(next_hue,100);

    next_hue = anim->getNextHue(100);
    EXPECT_EQ(next_hue,10);
    EXPECT_EQ(anim->hue_inc,5);

    anim->hue_inc = -5;
    next_hue = anim->getNextHue(15);
    EXPECT_EQ(next_hue,10);

    next_hue = anim->getNextHue(10);
    EXPECT_EQ(next_hue,100);
    EXPECT_EQ(anim->hue_inc,-5);

    delete anim;
}

void test_calcNextPosition(RandomWalkAnimation & anim, MockRandomGenerator & random, uint16_t start_position, uint16_t expected_next_position, uint16_t rnd_value)
{
    anim.current_position = start_position;
    EXPECT_CALL(random, make_random()).Times(1).WillOnce(Return(rnd_value));
    auto next_position = anim.calcNextPosition();
    EXPECT_EQ(next_position,expected_next_position);
}

TEST(RandomWalk, calcNextPosition_allblack)
{
    Subset su[] = {{0,3,1},{3,3,-1},{6,3,1}};
    MockLedStrip led_strip;
    MockRandomGenerator random;
    
    RandomWalkAnimation *anim = makeAnimation(su,{},led_strip,random);

    //position 0 : neighbours are {5,1}
    test_calcNextPosition(*anim,random,0,5,255*1-1);
    test_calcNextPosition(*anim,random,0,1,255*2-1);

    //position 5 : neighbours are {0,6,4}
    test_calcNextPosition(*anim,random,5,0,255*1-1);
    test_calcNextPosition(*anim,random,5,6,255*2-1);
    test_calcNextPosition(*anim,random,5,4,255*3-1);

    //position 4 : neighbours are  {1,5,7,3}
    test_calcNextPosition(*anim,random,4,1,255*1-1);
    test_calcNextPosition(*anim,random,4,5,255*2-1);
    test_calcNextPosition(*anim,random,4,7,255*3-1);
    test_calcNextPosition(*anim,random,4,3,255*4-1);

    delete anim;
}

TEST(RandomWalk, calcNextPosition_nonblack)
{
    Subset su[] = {{0,3,1},{3,3,-1},{6,3,1}};
    MockLedStrip led_strip;
    MockRandomGenerator random;
    
    RandomWalkAnimation *anim = makeAnimation(su,{},led_strip,random);

    //position 0 : neighbours are {5,1}
    anim->brightness[5] = 0;
    anim->brightness[1] = 255;
    test_calcNextPosition(*anim,random,0,5,0);
    test_calcNextPosition(*anim,random,0,5,125);
    test_calcNextPosition(*anim,random,0,5,254);

    anim->brightness[5] = 255;
    anim->brightness[1] = 0;
    test_calcNextPosition(*anim,random,0,1,0);
    test_calcNextPosition(*anim,random,0,1,125);
    test_calcNextPosition(*anim,random,0,1,254);
    anim->brightness[5] = 0;

    anim->brightness[0] = 100;
    anim->brightness[6] = 100;
    anim->brightness[4] = 100;
    //position 5 : neighbours are {0,6,4}
    test_calcNextPosition(*anim,random,5,0,(255-100)*1-1);
    test_calcNextPosition(*anim,random,5,6,(255-100)*2-1);
    test_calcNextPosition(*anim,random,5,4,(255-100)*3-1);
    anim->brightness[0] = 0;
    anim->brightness[6] = 0;
    anim->brightness[4] = 0;

    //position 4 : neighbours are  {1,5,7,3}
    anim->brightness[1] = 20;
    anim->brightness[5] = 50;
    anim->brightness[7] = 100;
    anim->brightness[3] = 200;
    int p1 = 255 - anim->brightness[1];
    int p5 = 255 - anim->brightness[5];
    int p7 = 255 - anim->brightness[7];
    int p3 = 255 - anim->brightness[3];
    test_calcNextPosition(*anim,random,4,1,p1-1);
    test_calcNextPosition(*anim,random,4,5,p1+p5-1);
    test_calcNextPosition(*anim,random,4,7,p1+p5+p7-1);
    test_calcNextPosition(*anim,random,4,3,p1+p5+p7+p3-1);

    delete anim;
}