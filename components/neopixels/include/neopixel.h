#include <stdint.h>
#include <driver/gpio.h>
#include <driver/rmt.h>
#include <esp_event.h>

namespace Neopixel
{

struct RGB
{
    uint8_t r,g,b;
};

struct HSV
{
    enum class Hue {
        RED = 0,
        ORANGE = 32,
        YELLOW = 64,
        GREEN = 96,
        AQUA = 128,
        BLUE = 160,
        PURPLE = 192,
        PINK = 224
    };

    uint16_t h;
    uint8_t s,v;
    RGB toRGB() const;
};

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
struct LedStrip
{
    static LedStrip* create(const LedStripConfig&);
    virtual int getLength() const = 0;
    virtual RGB* getBuffer() = 0;
    virtual void setPixelsRGB(int first, int num, const RGB*) = 0;
    virtual void fillPixelsRGB(int first, int num, const RGB&) = 0;
    virtual void setPixelsHSV(int first, int num, const HSV*) = 0;
    virtual void refresh(bool wait=false) = 0;
    virtual void copyFrontToBack() = 0;
    virtual bool waitReady(uint32_t timeout_ms) = 0;
    virtual void release() = 0;
protected:
    virtual ~LedStrip(){}
};
}
