#include <neopixels.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace Neopixel;

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
        for (int i = 0; i < 3; i++) 
        {
            for (int j = i; j < size; j += 3) 
            {
                // Build RGB values
                hsv.h = j * 360 / size + start_rgb;
                strip[j] = hsv.toRGB();
            }
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(strip.refresh());
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
extern "C" void neopixel_task(void* unused)
{
    auto strip1 = WS2811(GPIO_NUM_16, WS2811::CH0, 4);
    auto strip2 = WS2811(GPIO_NUM_17, WS2811::CH4, 4);
    LedStrip::Config cfg[] = {{100, &strip1},{200, &strip2}};
    auto strip = LedStrip(2, cfg);
    auto animation = get_animation_by_number(0);
    xTaskCreate(animation, "neopixels_animation", 4096, (void*)&strip, 5, NULL);

}