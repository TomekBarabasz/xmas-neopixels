#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>
#include <DigitalRainAnimation.hpp>
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
    encode_prm<uint16_t>("hue",prms,p,120);
    encode_prm<uint16_t>("head_length",prms,p,3);
    encode_prm<uint16_t>("tail_length_min",prms,p,10);
    encode_prm<int8_t>("tail_length_max",prms,p,30);

    return {params, (uint8_t*)p-params};
}

template <size_t N>
DigitalRainAnimation* makeAnimation(Subset (&su)[N], const ParamsMap& prms, MockLedStrip& led_strip, MockRandomGenerator& random)
{
    Strips *pstrips = makeStrips(su);
    EXPECT_CALL(random, make_random()).Times(1).WillOnce(Return(0));
    auto [params,len] = encodeParams(prms);
    auto *pa = new DigitalRainAnimation(&led_strip,len,params,pstrips,&random);
    release(pstrips);
    return pa;
}
}
TEST(DigitalRain, create)
{
    Subset su[] = {{0,3,1},{3,3,-1},{6,3,1}};
    MockLedStrip led_strip;
    MockRandomGenerator random;
    
    auto *anim = makeAnimation(su,{},led_strip,random);
    delete anim;
}

