#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <neopixel.h>
#include <neopixel_app.h>

using namespace Neopixel;

namespace NeopixelApp 
{
ESP_EVENT_DEFINE_BASE(NEOPIXEL_EVENTS);
TaskHandle_t animationTask = NULL;
using AnimationFcn = void (*)(void*);
static const char* TAG = "npx-app";

template <typename T>
void encode(void*& data, T v)
{
    T* pv = reinterpret_cast<T*>(data);
    *pv++ = v;
    data = pv;
}

template <typename T>
T decode(void*& data)
{
    T* pv = reinterpret_cast<T*>(data);
    T v = *pv++;
    data = pv;
    return v;
}

static void default_animation(void *params)
{
    void *p = params;
    LedStrip* strip = decode<LedStrip*>(params);
    const uint16_t delay_ms = decode<uint16_t>(params);
    delete[] reinterpret_cast<uint8_t*>(p);
    const auto size = strip->getLength();

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
                strip->setPixelsHSV(j, 1, &hsv);
            }
            strip->refresh(true);
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
        start_rgb += 60;
    }
}

static void colortest_animation(void *params)
{
    void *p=params;
    auto * strip = decode<LedStrip*>(params);
    const uint16_t delay_ms = decode<uint16_t>(params);
    ESP_LOGI(TAG, "colortest_animation : delay %d strip %p", delay_ms, strip);
    delete[] reinterpret_cast<uint8_t*>(p);
    const auto size = strip->getLength();
    
    RGB black = {0,0,0};
    RGB rgb[] = { {255,0,0}, {0, 255,0}, {0,0,255} };
    strip->fillPixelsRGB(0,size,{0,0,0});
    for(;;)
    {
        for (int i=0;i<size;++i)
        {
            for (int j=0;j<3;++j)
            {
                strip->setPixelsRGB(i,1,rgb+j);
                strip->refresh(true);
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
            }
            strip->setPixelsRGB(i,1,&black);
        }
    }
}
static AnimationFcn get_animation_by_number(uint32_t animation_id)
{
    switch(animation_id)
    {
        default:
        case 0: return default_animation;
        case 1: return colortest_animation;
    }
}
esp_event_loop_handle_t create_event_loop()
{
    esp_event_loop_args_t loop_args = {
        .queue_size = 4,
        .task_name = "neopixels_event_loop",
        .task_priority = 5, //uxTaskPriorityGet(NULL),
        .task_stack_size = 2048,
        .task_core_id = 1
    };

    esp_event_loop_handle_t loop_handle;
    esp_event_loop_create(&loop_args, &loop_handle);
    return loop_handle;
}

static void execute_CmdSet(LedStrip *strip, void *data)
{
    uint16_t num_parts = decode<uint16_t>(data);
    uint8_t refresh = decode<uint8_t>(data);
    while(num_parts--)
    {
        RGB rgb = { decode<uint8_t>(data), decode<uint8_t>(data), decode<uint8_t>(data) };
        uint16_t first = decode<uint16_t>(data);
        uint16_t count = decode<uint16_t>(data);
        strip->fillPixelsRGB(first, count, rgb);
    }
    if (refresh > 0){
        strip->refresh( refresh==2 );
    }
}
static void execute_CmdStartAnimation(LedStrip *strip,void *data)
{
    const uint32_t animation_id = decode<uint16_t>(data);
    const uint16_t animation_data_size = decode<uint16_t>(data);
    auto animation = get_animation_by_number(animation_id);
    if (animationTask != NULL) {
        vTaskDelete(animationTask);
        animationTask = NULL;
    }
    if (animation) 
    {
        uint8_t *task_data = new uint8_t[sizeof(LedStrip*)+animation_data_size];
        void *p = task_data;
        encode<LedStrip*>(p,strip);
        memcpy(p,data,animation_data_size);
        ESP_LOGI(TAG, "execute_CmdStartAnimation : starting animation %d data size %d strip %p", animation_id, animation_data_size, strip);
        xTaskCreatePinnedToCore(animation, "neopixel_animation", 2048, task_data, 5, &animationTask, 1);
    }
}
static void execute_CmdReconfigure(LedStrip *strip,void *data)
{

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
    const auto command_id = decode<uint8_t>(event_data);

    switch(command_id)
    {
        case NeopixelApp::CmdSet:
            ESP_LOGI(TAG, "neopixel_event_handler : execute_CmdSet");
            execute_CmdSet(strip,event_data);
            break;
        case NeopixelApp::CmdStartAnimation:
            ESP_LOGI(TAG, "neopixel_event_handler : execute_CmdStartAnimation");
            execute_CmdStartAnimation(strip, event_data);
            break;
        case NeopixelApp::CmdReconfigure:
            ESP_LOGI(TAG, "neopixel_event_handler : execute_CmdStartAnimation");
            execute_CmdReconfigure(strip,event_data);
            break;
        default:
            ESP_LOGE(TAG, "neopixel_event_handler : invalid command id %d", command_id);
    }
}
extern "C" void neopixel_main(void* params)
{
    auto loop_handle = reinterpret_cast<esp_event_loop_handle_t>(params);
#if 0    
    RMTDriverConfig rmt[] = {{GPIO_NUM_16, RMT_CHANNEL_0, 4}, {GPIO_NUM_17, RMT_CHANNEL_4, 4}};
    LedStripConfig cfg = {2,1,
                            {
                                {100, SegmentType::WS2811, DriverType::RMT, &rmt[0]},
                                {200, SegmentType::WS2811, DriverType::RMT, &rmt[1]}
                            }
    };
#else
    RMTDriverConfig rmt = {GPIO_NUM_16, RMT_CHANNEL_0, 8};
    LedStripConfig cfg = {1,1,
                            {
                                {50, SegmentType::WS2811, DriverType::RMT, &rmt}
                            }
    };
#endif
    auto *strip = LedStrip::create(cfg);
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_handle, NEOPIXEL_EVENTS, ESP_EVENT_ANY_ID, neopixel_event_handler, strip, NULL));

    uint8_t default_animation_params[7];
    void *p = default_animation_params;
    encode<uint8_t>(p, NeopixelApp::CmdStartAnimation);
    encode<uint16_t>(p, 1);
    encode<uint16_t>(p, 2);
    encode<uint16_t>(p, 500);
    ESP_LOGI(TAG, "neopixel_main : starting default animation, params ptr %p", default_animation_params);
    ESP_ERROR_CHECK(esp_event_post_to(loop_handle, NEOPIXEL_EVENTS, 0, default_animation_params, sizeof(default_animation_params), portMAX_DELAY));
    vTaskDelete(NULL);
}
}