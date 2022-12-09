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
#include <vector>
#include <color.hpp>

using namespace Neopixel;
using namespace ::testing;
using ::testing::Return;
using ParamsMap = std::map<std::string,int>;

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
DigitalRainAnimation* makeAnimation(const Strips* pstrips, const ParamsMap& prms, LedStrip* led_strip, MockRandomGenerator& random)
{
    EXPECT_CALL(random, make_random()).Times(2*pstrips->count).WillRepeatedly(Return(0));
    auto [params,len] = encodeParams(prms);
    auto *pa = new DigitalRainAnimation(led_strip,len,params,pstrips,&random);

    return pa;
}
struct MockLedStrip : public LedStrip 
{
    struct SetRGB
    {
        int first, num;
        RGB color;
    };
    struct SetHSV
    {
        int first, num;
        HSV color;
    };
    std::vector<SetRGB> rgb_calls;
    std::vector<SetHSV> hsv_calls;
    MOCK_METHOD( int,  getLength,    (), (const));
    MOCK_METHOD( RGB*, getBuffer,    ());
    MOCK_METHOD( void, fillPixelsRGB,(int first, int num, const RGB&));
    void setPixelsRGB(int first, int num, const RGB* color) override
    {
        rgb_calls.push_back( {first,num, *color});
    }
    void setPixelsHSV(int first, int num, const HSV* color) override
    {
        hsv_calls.push_back( {first,num, *color});
    }
    MOCK_METHOD( void, refresh,      (bool wait));
    MOCK_METHOD( void, copyFrontToBack,());
    MOCK_METHOD( bool, waitReady,    (uint32_t timeout_ms));
    MOCK_METHOD( void, release,      ());
};
}
TEST(DigitalRain, create)
{
    Subset su[] = {{0,10,1},{15,10,-1},{30,10,1}};
    Strips *pstrips = makeStrips(su);
    MockLedStrip led_strip;
    MockRandomGenerator random;
    
    auto *anim = makeAnimation(pstrips,{},&led_strip,random);
    auto & strips = *(anim->pixelLines);

    EXPECT_EQ(anim->rain_lines[0].state, 0);
    EXPECT_EQ(anim->rain_lines[1].state, 0);
    EXPECT_EQ(anim->rain_lines[2].state, 0);
    EXPECT_EQ(anim->rain_lines[0].position, strips.element[0].count-1);
    EXPECT_EQ(anim->rain_lines[1].position, strips.element[1].count-1);
    EXPECT_EQ(anim->rain_lines[2].position, strips.element[2].count-1);

    delete anim;
    release(pstrips);
}

bool operator==(const RGB&a, const RGB&b)
{
    return a.r==b.r && a.g==b.g && a.b==b.b;
}
TEST(DigitalRain, test_drawLine)
{
    Subset su[] = {{0,10,1},{15,10,-1},{30,10,1}};
    Strips *pstrips = makeStrips(su);
    MockLedStrip led_strip;
    MockRandomGenerator random;
    
    auto *anim = makeAnimation(pstrips,{},&led_strip,random);
    anim->head_length = 3;

    const auto l1_indices = anim->line_indices;
    const auto l2_indices = anim->line_indices + anim->indices_row_size;
    const auto l3_indices = anim->line_indices + anim->indices_row_size * 2;
    const auto l1_size = pstrips->element[0].count;

    RGB white {255,255,255};

    #if 0
    line.length = 5;
    bool result = anim->drawLine(0);
    EXPECT_TRUE(result);
    EXPECT_EQ(led_strip.rgb_calls.size(),1);
    EXPECT_EQ(led_strip.hsv_calls.size(),0);
    EXPECT_EQ(led_strip.rgb_calls[0].first,l1_indices[ l1_size-1 ]);
    EXPECT_EQ(led_strip.rgb_calls[0].num,1);
    EXPECT_TRUE(led_strip.rgb_calls[0].color == white);
    led_strip.rgb_calls.clear();

    result = anim->drawLine(0);
    EXPECT_TRUE(result);
    EXPECT_EQ(led_strip.rgb_calls.size(),1);
    EXPECT_EQ(led_strip.hsv_calls.size(),0);
    EXPECT_EQ(led_strip.rgb_calls[0].first,l1_indices[ l1_size-2 ]);
    EXPECT_EQ(led_strip.rgb_calls[0].num,2);
    EXPECT_TRUE(led_strip.rgb_calls[0].color == white);
    led_strip.rgb_calls.clear();

    result = anim->drawLine(0);
    EXPECT_TRUE(result);
    EXPECT_EQ(led_strip.rgb_calls.size(),1);
    EXPECT_EQ(led_strip.hsv_calls.size(),0);
    EXPECT_EQ(led_strip.rgb_calls[0].first,l1_indices[ l1_size-3 ]);
    EXPECT_EQ(led_strip.rgb_calls[0].num,3);
    EXPECT_TRUE(led_strip.rgb_calls[0].color == white);
    led_strip.rgb_calls.clear();
    #endif
    int line_idx = 1;

    auto & line = anim->rain_lines[line_idx];
    line.length = 5;

    std::cout << "test draw line " << line_idx << std::endl;
    bool result;
    do
    {
        std::cout << "\nposition " << (int)line.position << std::endl;
        result = anim->drawLine(line_idx);
        if(led_strip.hsv_calls.size())
        {
            std::cout << "hsv calls" << std::endl;
            for (auto & call : led_strip.hsv_calls) {
                std::cout << "first " << call.first << " num " << call.num << " color " << (int)call.color.h << ' ' << (int)call.color.s << ' ' << (int)call.color.v << std::endl;
            }
        }
        if (led_strip.rgb_calls.size())
        {
            std::cout << "rgb calls" << std::endl;
            for (auto & call : led_strip.rgb_calls) {
                std::cout << "first " << call.first << " num " << call.num << " color " << (int)call.color.r << ' ' << (int)call.color.g << ' ' << (int)call.color.b << std::endl;
            }
        }
        anim->moveLine(line_idx);
        led_strip.rgb_calls.clear();
        led_strip.hsv_calls.clear();
    } while (result);
    
    
    delete anim;
    release(pstrips);
}
