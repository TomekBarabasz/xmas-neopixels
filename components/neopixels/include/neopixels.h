#include <driver/gpio.h>

namespace Neopixel
{

struct RGB
{
    uint8_t r,g,b;
};

#define MAKE_RGB(R,G,B) { uint8_t((R)/100), uint8_t((G)/100), uint8_t((B)/100) }
struct HSV
{
    uint16_t h;
    uint8_t s,v;
    inline RGB toRGB() const
    {
        const uint16_t rgb_max = v * uint16_t(256) - v;
        const uint16_t rgb_min = rgb_max * ( uint16_t(100) - s);
        const uint8_t i = uint8_t(h / 60);
        const uint8_t diff = uint8_t(h % 60);
        // RGB adjustment amount by hue
        const uint16_t rgb_adj = (rgb_max - rgb_min) * diff / 60;
        switch (i) {
        case 0: return MAKE_RGB( rgb_max,           rgb_min + rgb_adj, rgb_min );
        case 1: return MAKE_RGB( rgb_max - rgb_adj, rgb_max,           rgb_min );
        case 2: return MAKE_RGB( rgb_min,           rgb_max,           rgb_min + rgb_adj );
        case 3: return MAKE_RGB( rgb_min,           rgb_max - rgb_adj, rgb_max );
        case 4: return MAKE_RGB( rgb_min + rgb_adj, rgb_min,           rgb_max );
        default:return MAKE_RGB( rgb_max,           rgb_min,           rgb_max - rgb_adj );
        }
    }
};

struct Driver
{
    virtual void write(int size, RGB* data) = 0;
};

class WS2811 : public Driver
{
public:
    enum Channel { CH0=0, CH1, CH2, CH3, CH4, CH5, CH6, CH7 };
    WS2811(gpio_num_t gpio_port, Channel chn, int mem_block_num);
    virtual void write(int size, RGB* data) override;
};

class LedStrip
{
public:
    struct Config {
        int size;
        Driver *drv;
    };
    LedStrip(int ndrivers, Config*);
    int getLength() const { return _totSize; }
    RGB& operator[](int i) { return _back[i];}
    const RGB& operator[](int i) const { return _back[i];}
    void setRGB(int first, int count, RGB*);
    void setHSV(int first, int count, HSV*);
    int refresh();
private:
    int _ndrivers, _totSize;
    const Config* _cfg;
    RGB *_front, *_back;
    bool _newData;
};

}