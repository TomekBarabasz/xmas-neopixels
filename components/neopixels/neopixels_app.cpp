#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <neopixel.h>
#include <neopixel_app.h>
#include <utils.hpp>
#include <animation.hpp>
#include <collections.hpp>
#include <random.hpp>

using namespace Neopixel;
uint32_t esp_random(void);

namespace Neopixel
{
void Animation::main(void*param)
{
    auto anim = reinterpret_cast<Animation*>(param);
    for(;;)
    {
        anim->step();
        vTaskDelay(pdMS_TO_TICKS(anim->get_delay_ms()));
    }
}
}
namespace NeopixelApp 
{
ESP_EVENT_DEFINE_BASE(NEOPIXEL_EVENTS);
TaskHandle_t animationTask = NULL;
LedStrip *strip = nullptr;

class EspRandomGenerator : public RandomGenerator
{
    uint32_t make_random() override { return esp_random(); }
    void make_random_n(uint32_t *values, int length) override
    {
        while(length-- >0) {
            *values++ = esp_random();
        }
    }
    void release() override {}
};
EspRandomGenerator radomGen;
Animation *currentAnimation = nullptr;
static const char* TAG = "npx-app";

static constexpr Strips rings { 8, {
        {0, 42, 0},
        {42,36, 1},
        {78,37, 0},
        {115,37, 1},
        {152,47, 0},
        {199,15, 1},
        {214,17, 1},
        {231,18, 1}
    }
};

static constexpr Strips strips { 16, {
        {0,28,1},
        {29,27,-1},
        {57,28,1},
        {86,26,-1},
        {113,28,1},
        {142,28,-1},
        {171,28,1},
        {200,28,-1},
        {229,27,1},
        {257,27,-1},
        {285,27,1},
        {313,28,-1},
        {342,28,1},
        {371,28,-1},
        {402,22,1},
        {425,22,-1},
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
    currentAnimation = Animation::create(strip,animation_id, data, &strips,&radomGen);
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
#if 0
static void start_default_animation(esp_event_loop_handle_t loop_handle)
{
    uint8_t default_animation_params[32];
    void *p = default_animation_params;
    encode<uint8_t>(p, NeopixelApp::CmdStartAnimation);
    encode<uint16_t>(p, 1);
    encode<uint16_t>(p, 11);
    encode<uint16_t>(p, 20);
    encode<uint16_t>(p, 100);
    encode<int8_t>(p,  5);
    encode<int8_t>(p,  1);
    encode<uint8_t>(p, 252);
    encode<uint32_t>(p,  0x00EDA356);
    ESP_LOGI(TAG, "neopixel_main : starting default animation, params ptr %p", default_animation_params);
    ESP_ERROR_CHECK(esp_event_post_to(loop_handle, NEOPIXEL_EVENTS, 0, default_animation_params, sizeof(default_animation_params), portMAX_DELAY));
}
#else
static void start_default_animation(esp_event_loop_handle_t loop_handle)
{
    uint8_t default_animation_params[32];
    void *p = default_animation_params;
    encode<uint8_t>(p, NeopixelApp::CmdStartAnimation);
    encode<uint16_t>(p,12);
    encode<uint16_t>(p,11);
    encode<uint16_t>(p,25);
    encode<uint16_t>(p,150);
    encode<uint16_t>(p,0);
    encode<uint16_t>(p,360);
    encode<int8_t>(p,1);
    encode<uint8_t>(p,1);
    encode<uint8_t>(p,230);
    ESP_LOGI(TAG, "neopixel_main : starting default animation, params ptr %p", default_animation_params);
    ESP_ERROR_CHECK(esp_event_post_to(loop_handle, NEOPIXEL_EVENTS, 0, default_animation_params, sizeof(default_animation_params), portMAX_DELAY));
}
#endif
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
    LedSegmentConfig segment {450, SegmentType::WS2811, DriverType::RMT, &rmt};
    LedStripConfig cfg = {1,1,&segment};
#endif
    strip = LedStrip::create(cfg);
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_handle, NEOPIXEL_EVENTS, ESP_EVENT_ANY_ID, neopixel_event_handler, NULL, NULL));

    start_default_animation(loop_handle);
    vTaskDelete(NULL);
}
}