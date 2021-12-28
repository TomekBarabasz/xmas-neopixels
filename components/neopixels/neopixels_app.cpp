#include <neopixels.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>

using namespace Neopixel;

namespace NeopixelApp
{
using AnimationFcn = void (*)(void*);

static void default_animation(void *prms)
{
    constexpr int CHASE_SPEED_MS = 500;
    auto & strip = *static_cast<LedStrip*>(prms);
    const auto size = strip.getLength();
    uint16_t start_rgb = 0;
    HSV hsv {0,100,100};

    while (true) 
    {
        RGB *data = strip.getBuffer();
        for (int i = 0; i < 3; i++) 
        {
            for (int j = i; j < size; j += 3) 
            {
                // Build RGB values
                hsv.h = j * 360 / size + start_rgb;
                data[j] = hsv.toRGB();
            }
            // Flush RGB values to LEDs
            strip.refresh();
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
            //strip->clear(strip, 50);
            //vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
        start_rgb += 60;
    }
}
static AnimationFcn get_animation_by_number(int animation_id)
{
    switch(animation_id)
    {
        default:
        case 0: return default_animation;
    }
}
extern "C" void app_main(void* unused)
{
    RMTDriverConfig rmt[] = {{GPIO_NUM_16, 0, 4}, {GPIO_NUM_17, 4, 4}};
    LedStripConfig cfg = {2,
                            {
                                {100, SegmentType::WS2811, DriverType::RMT, &rmt[0]},
                                {200, SegmentType::WS2811, DriverType::RMT, &rmt[1]}
                            }
    };
    auto *strip = LedStrip::create(cfg);
    auto animation = get_animation_by_number(0);
    xTaskCreate(animation, "neopixels_animation", 4096, (void*)strip, 5, NULL);
}
}