#include <stdint.h>

namespace Neopixel
{

struct RGB
{
    uint8_t r,g,b;
};

struct HSV
{
    uint16_t h;
    uint8_t s,v;
    RGB toRGB() const;
};

struct RMTDriverConfig
{
    int gpio, channel, mem_block_num;
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
    int num_segments;
    LedSegmentConfig segments[];
};
struct LedStrip
{
    static LedStrip* create(const LedStripConfig&);
    virtual int getLength() const = 0;
    virtual RGB* getBuffer() = 0;
    virtual void setPixelRGB(int first, int num, const RGB*) = 0;
    virtual void setPixelHSV(int first, int num, const HSV*) = 0;
    virtual void refresh() = 0;
    virtual void release() = 0;
protected:
    virtual ~LedStrip(){}
};
extern "C" void app_main(void*);
}
