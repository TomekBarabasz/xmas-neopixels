#include <gtest/gtest.h>
#include "gmock/gmock.h"
#include <tuple>
#include <RandomWalkAnimation.hpp>
#include <led_strip.hpp>

using namespace ::testing;
using namespace Neopixel;

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

TEST(RandomWalk, Init) 
{
    MockLedStrip strip;
    RandomWalkAnimation a(&strip,0,nullptr);
}

