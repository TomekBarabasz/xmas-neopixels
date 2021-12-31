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
LedStrip *strip = nullptr;
struct Animation
{
    static void main(void*param)
    {
        auto anim = reinterpret_cast<Animation*>(param);
        anim->run();
    }
    virtual void run() = 0;
    virtual ~Animation(){}
};
Animation *currentAnimation = nullptr;
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

static void fade_all(RGB *leds, int size, uint8_t scale)
{
    for (int i=0;i<size;++i) {
        leds[i].scale8(scale);
    }
}

struct Colortest : public Animation
{
    LedStrip *strip;
    uint16_t delay_ms;
    Colortest(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms = decode<uint16_t>(data);
        ESP_LOGI(TAG, "colortest animation : delay %d", delay_ms);
    }
    void run() override
    {
        RGB black = {0,0,0};
        RGB rgb[] = { {255,0,0}, {0, 255,0}, {0,0,255} };
        const auto size = strip->getLength();

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
};

struct Cylon : Animation
{
    LedStrip* strip;
    uint16_t delay_ms;
    uint8_t inc;
    uint8_t fade;
    Cylon(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms = decode<uint16_t>(data);
        inc = decode<uint8_t>(data);
        fade = decode<uint8_t>(data);
        ESP_LOGI(TAG, "Cylon animation : delay %d inc %d face %d", delay_ms, inc, fade);
    }
    void run() override
    {
        const auto size = strip->getLength();
        HSV hsv {0,255,255};
        int count = size;
        int i = 0;
        int dir = +1;
        for(;;)
        {
            while(count-->0)
            {
                hsv.h += inc;
                strip->fillPixelsRGB(i,1,hsv.toRGB());
                strip->refresh();
                fade_all(strip->getBuffer(), size, fade);
                i += dir;
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
            }
            if (dir>0) {
                dir = -1;
                --i;
            } else {
                dir = +1;
                ++i;
            }
            count = size;
        }
    }
};

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
    uint16_t num_parts = decode<uint8_t>(data);
    uint8_t refresh = decode<uint8_t>(data);
    while(num_parts--)
    {
        auto r = decode<uint8_t>(data);
        auto g = decode<uint8_t>(data);
        auto b = decode<uint8_t>(data);
        RGB rgb = { r,g,b };
        uint16_t first = decode<uint16_t>(data);
        uint16_t count = decode<uint16_t>(data);
        ESP_LOGI(TAG, "execute_CmdSet : first %d count %d r %d g %d b %d", first, count, r, g, b);
        strip->fillPixelsRGB(first, count, rgb);
    }
    if (refresh > 0){
        ESP_LOGI(TAG, "execute_CmdSet : refresh");
        strip->refresh( refresh==2 );
    }
}
static Animation* create_animation(LedStrip*strip,int animation_id, void* data)
{
    const uint16_t animation_data_size = decode<uint16_t>(data);
    switch(animation_id)
    {
        default:
        case 0: return new Colortest(strip, animation_data_size, data);
        case 1: return new Cylon(strip, animation_data_size, data);
    }
}
static void execute_CmdStartAnimation(LedStrip *strip,void *data)
{
    const uint16_t animation_id = decode<uint16_t>(data);

    if (animationTask != NULL) {
        vTaskDelete(animationTask);
        animationTask = NULL;
    }
    if (currentAnimation) {
        delete currentAnimation;
        currentAnimation = nullptr;
    }
    currentAnimation = create_animation(strip, animation_id, data);
    strip->fillPixelsRGB(0,strip->getLength(),{0,0,0});
    if (currentAnimation)
    {
        xTaskCreatePinnedToCore(Animation::main, "neopixel_animation", 2048, currentAnimation, 5, &animationTask, 1);
    }
}
static LedStrip* execute_CmdReconfigure(LedStrip *strip,void *data)
{
#if 0
    strip->release();
    const uint8_t num_buffers = decode<uint8_t>(data);
    const uint8_t num_segments = decode<uint8_t>(data);

    for (int s=0;s<num_segments;++s)
    {
        const uint16_t num_leds = decode<uint16_t>(data);
        const uint8_t segment_type = decode<uint8_t>(data);
        const uint8_t driver_type = decode<uint8_t>(data);
        switch(driver_type)
        case DriverType::RMT: {
            const uint8_t gpio_num = decode<uint8_t>(data);
            const uint8_t channel_num = decode<uint8_t>(data);
            const uint8_t mem_block_num = decode<uint8_t>(data);
        }
    }
#endif
    return strip;
}
static void start_default_animation(esp_event_loop_handle_t loop_handle)
{
    uint8_t default_animation_params[9];
    void *p = default_animation_params;
    encode<uint8_t>(p, NeopixelApp::CmdStartAnimation);
    encode<uint16_t>(p, 1);
    encode<uint16_t>(p, 4);
    encode<uint16_t>(p, 10);
    encode<uint8_t>(p,  1);
    encode<uint8_t>(p,  250);
    ESP_LOGI(TAG, "neopixel_main : starting default animation, params ptr %p", default_animation_params);
    ESP_ERROR_CHECK(esp_event_post_to(loop_handle, NEOPIXEL_EVENTS, 0, default_animation_params, sizeof(default_animation_params), portMAX_DELAY));
}
static void neopixel_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    // Two types of data can be passed in to the event handler: the handler specific data and the event-specific data.
    //
    // The handler specific data (handler_args) is a pointer to the original data, therefore, the user should ensure that
    // the memory location it points to is still valid when the handler executes.
    //
    // The event-specific data (event_data) is a pointer to a deep copy of the original data, and is managed automatically.
    //auto * strip = reinterpret_cast<LedStrip*>(handler_args);
    const auto command_id = decode<uint8_t>(event_data);

    switch(command_id)
    {
        case NeopixelApp::CmdSet:
            execute_CmdSet(strip,event_data);
            break;
        case NeopixelApp::CmdStartAnimation:
            execute_CmdStartAnimation(strip, event_data);
            break;
        case NeopixelApp::CmdReconfigure:
            ESP_LOGI(TAG, "neopixel_event_handler : CmdReconfigure");
            strip = execute_CmdReconfigure(strip,event_data);
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
    LedSegmentConfig segments[] = { {150, SegmentType::WS2811, DriverType::RMT, &rmt[0]},
                                    {150, SegmentType::WS2811, DriverType::RMT, &rmt[1]} };
    LedStripConfig cfg = {1,2,segments};
#else
    RMTDriverConfig rmt = {GPIO_NUM_16, RMT_CHANNEL_0, 8};
    LedSegmentConfig segment {150, SegmentType::WS2811, DriverType::RMT, &rmt};
    LedStripConfig cfg = {1,1,&segment};
#endif
    strip = LedStrip::create(cfg);
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_handle, NEOPIXEL_EVENTS, ESP_EVENT_ANY_ID, neopixel_event_handler, NULL, NULL));

    start_default_animation(loop_handle);
    vTaskDelete(NULL);
}
}