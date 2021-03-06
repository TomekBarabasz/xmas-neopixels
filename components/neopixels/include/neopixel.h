#include <stdint.h>
#include <driver/gpio.h>
#include <driver/rmt.h>
#include <esp_event.h>

namespace Neopixel
{

struct RGB
{
    uint8_t r,g,b;
    void scale8(uint8_t scale)
    {
         uint16_t scale_fixed = scale + 1;
        r = (((uint16_t)r) * scale_fixed) >> 8;
        g = (((uint16_t)g) * scale_fixed) >> 8;
        b = (((uint16_t)b) * scale_fixed) >> 8;
    }
    const RGB& operator+=(const RGB& other)
    {
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }
    const RGB& operator+(const RGB& other)
    {
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }
    RGB mix(const RGB& other, uint8_t alpha) const
    {
        const auto alpha16 = uint16_t(alpha);
        const auto not_alpha16 = uint16_t(255) - alpha16;
        uint8_t r_ = (uint8_t) ((r * alpha16 + other.r * not_alpha16) >> 8);
        uint8_t g_ = (uint8_t) ((g * alpha16 + other.g * not_alpha16) >> 8);
        uint8_t b_ = (uint8_t) ((b * alpha16 + other.b * not_alpha16) >> 8);
        return { r_,g_,b_ };
    }
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
