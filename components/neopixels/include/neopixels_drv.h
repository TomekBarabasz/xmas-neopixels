#include <stdint.h>
namespace Neopixel {
    struct RGB;
}

namespace NeopixelDrv
{
struct Timing
{
    int T0H_NS, T0L_NS, T1H_NS, T1L_NS, RESET_NS;
    static const Timing& ws2811();
    static const Timing& ws2812();
};

struct Driver
{
    virtual void write(int size, Neopixel::RGB* data) = 0;
protected:
    virtual ~Driver(){}
};
}