#include <neopixels.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>

namespace Neopixel
{
using AnimationFcn = void (*)(void*);
ESP_EVENT_DEFINE_BASE(NEOPIXEL_EVENTS);

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
            strip.refresh();
            vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
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
esp_event_loop_handle_t create_event_loop()
{
    esp_event_loop_args_t loop_args = {
        .queue_size = 16,
        .task_name = "neopixels_event_loop",
        .task_priority = 5, //uxTaskPriorityGet(NULL),
        .task_stack_size = 4096,
        .task_core_id = 1
    };

    esp_event_loop_handle_t loop_handle;
    esp_event_loop_create(&loop_args, &loop_handle);
    return loop_handle;
}
static void neopixel_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    // Two types of data can be passed in to the event handler: the handler specific data and the event-specific data.
    //
    // The handler specific data (handler_args) is a pointer to the original data, therefore, the user should ensure that
    // the memory location it points to is still valid when the handler executes.
    //
    // The event-specific data (event_data) is a pointer to a deep copy of the original data, and is managed automatically.
    auto * strip = reinterpret_cast<LedStrip*>(handler_args);
    switch(id)
    {
        case Neopixel::EvSetStatic:
            break;
        case Neopixel::EvStartAnimation:{
            auto animation_id = *reinterpret_cast<uint16_t*>(event_data);
            auto animation = get_animation_by_number(animation_id);
            if (animation) {
                xTaskCreate(animation, "neopixel_animation", 4096, (void*)strip, 5, NULL);
            }
            break;
        }
    }
}
extern "C" void neopixel_main(void* params)
{
    auto loop_handle = reinterpret_cast<esp_event_loop_handle_t>(params); 
    RMTDriverConfig rmt[] = {{GPIO_NUM_16, RMT_CHANNEL_0, 4}, {GPIO_NUM_17, RMT_CHANNEL_4, 4}};
    LedStripConfig cfg = {2,
                            {
                                {100, SegmentType::WS2811, DriverType::RMT, &rmt[0]},
                                {200, SegmentType::WS2811, DriverType::RMT, &rmt[1]}
                            }
    };
    auto *strip = LedStrip::create(cfg);
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_handle, NEOPIXEL_EVENTS, ESP_EVENT_ANY_ID, neopixel_event_handler, strip, NULL));

    uint16_t animation_id = 0;
    ESP_ERROR_CHECK(esp_event_post_to(loop_handle, NEOPIXEL_EVENTS, Neopixel::EvStartAnimation, &animation_id, sizeof(animation_id), portMAX_DELAY));
}
}