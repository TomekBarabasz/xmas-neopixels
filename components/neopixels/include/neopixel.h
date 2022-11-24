#include <stdint.h>
#include <driver/gpio.h>
#include <driver/rmt.h>
#include <esp_event.h>
#include <color.hpp>
#include <led_strip.hpp>

namespace Neopixel
{
struct RMTDriverConfig
{
    gpio_num_t gpio;
    rmt_channel_t channel;
    int mem_block_num;
};

enum class SegmentType { WS2811,WS2812 };
enum class DriverType { RMT, I2C, BITBANG };
struct LedSegmentConfig
{
    int num_leds;
    SegmentType strip;
    DriverType driver;
    void* driver_config;
};
struct LedStripConfig
{
    int num_buffers;
    int num_segments;
    //LedSegmentConfig segments[];
    LedSegmentConfig *segments;
};
}
