#include <stdio.h>
#include <string.h>
#include <tuple>
#include "freertos/FreeRTOS.h"
#include <esp_log.h>
#include <neopixel.h>
#include <neopixel_drv.h>
#include <math_utils.hpp>

using namespace Neopixel;
using namespace NeopixelDrv;

namespace NeopixelDrv
{
static rmt_item32_t ws2811_bits[2];
static rmt_item32_t ws2812_bits[2];    

static inline void IRAM_ATTR rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num, const rmt_item32_t *bits)
{
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }
    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;
    while (size < src_size && num < wanted_num) 
    {
        for (int i = 0; i < 8; i++) 
        {
            // MSB first
            if (*psrc & (1 << (7 - i))) {
                pdest->val =  bits[1].val;
            } else {
                pdest->val =  bits[0].val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

static void IRAM_ATTR rmt_adapter_ws2811(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    rmt_adapter(src,dest,src_size,wanted_num,translated_size,item_num,ws2811_bits);
}

static void IRAM_ATTR rmt_adapter_ws2812(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    rmt_adapter(src,dest,src_size,wanted_num,translated_size,item_num,ws2812_bits);
}

static const Timing& get_timing(SegmentType type)
{
    static const Timing ws2811 { 500, 2000, 1200, 1300, 500 };
    static const Timing ws2812 { 350, 1000, 1000,  350, 280 };
    switch(type) {
        default:
        case SegmentType::WS2811: return ws2811;
        case SegmentType::WS2812: return ws2812;
    }
}
struct RMT : public Driver
{
    RMT(gpio_num_t gpio_port, rmt_channel_t channel, int mem_block_num, SegmentType segType) :
        tx_channel( channel)
    {
        rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio_port, tx_channel);
        // set counter clock to 40MHz
        config.clk_div = 2;
        config.mem_block_num = mem_block_num;

        ESP_ERROR_CHECK(rmt_config(&config));
        ESP_LOGI("drv","rmt_config done");
        ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
        ESP_LOGI("drv","rmt_driver_install done");
        initTiming(segType);
        ESP_LOGI("drv","initTiming done");
    }
    void initTiming(SegmentType segType)
    {
        uint32_t counter_clk_hz = 0;
        ESP_ERROR_CHECK(rmt_get_counter_clock(tx_channel, &counter_clk_hz));
        const Timing& tm = get_timing(segType);
        // ns -> ticks
        const float ratio = (float)counter_clk_hz / 1e9;
        const auto t0h_ticks = (uint16_t)(ratio * tm.T0H_NS);
        const auto t0l_ticks = (uint16_t)(ratio * tm.T0L_NS);
        const auto t1h_ticks = (uint16_t)(ratio * tm.T1H_NS);
        const auto t1l_ticks = (uint16_t)(ratio * tm.T1L_NS);
        reset_ticks          = (uint16_t)(ratio * tm.RESET_NS);

        rmt_item32_t *bits;
        switch(segType){
            default:
            case SegmentType::WS2811:
                bits = ws2811_bits;
                rmt_translator_init(tx_channel, rmt_adapter_ws2811);
                break;
            case SegmentType::WS2812:
                bits = ws2812_bits;
                rmt_translator_init(tx_channel, rmt_adapter_ws2812);
                break;
        }
        bits[0] = {{{ t0h_ticks, 1, t0l_ticks, 0 }}}; //Logical 0
        bits[1] = {{{ t1h_ticks, 1, t1l_ticks, 0 }}}; //Logical 1
    }
    void write(int size, Neopixel::RGB* data, bool wait) override
    {
        ESP_ERROR_CHECK(rmt_write_sample(tx_channel, (uint8_t*)data, size * 3, wait));
    }
    bool wait(uint32_t timeout_ms) override
    {
        return rmt_wait_tx_done(tx_channel, pdMS_TO_TICKS(timeout_ms)) == ESP_OK;
    }
    void unload() override
    {
        ESP_ERROR_CHECK(rmt_driver_uninstall(tx_channel));
    }
    const rmt_channel_t tx_channel;
    uint16_t reset_ticks;
};
}

namespace Neopixel
{

struct SegmentInfo
{
    int num_leds;
    Driver *driver;
};

struct LedStripImpl : public LedStrip
{
    LedStripImpl(int totSize, int nSegments, SegmentInfo* segments, RGB* front, RGB* back, void* rawMem) : 
        _nSegments(nSegments), _totSize(totSize), _segments(segments), 
        _front(front), _back(back),
        _rawMem(rawMem)
    {}
    int getLength() const override { return _totSize; } 
    RGB* getBuffer() override { return _back; }
    void setPixelsRGB(int first, int count, const RGB* rgb) override
    {
        count = min(count, _totSize - first);
        memcpy(_back + first, rgb, count * sizeof(RGB));
    }
    void fillPixelsRGB(int first, int count, const RGB& rgb) override
    {
        count = min(count, _totSize - first);
        auto * ptr = _back + first;
        while(count--) *ptr++ = rgb;
    }
    void setPixelsHSV(int first, int count, const HSV* hsv) override
    {
        count = min(count, _totSize - first);
        for (int i=0; i<count; ++i) {
            _back[first + i] = hsv[i].toRGB();
        }
    }
    void refresh(bool wait) override
    {
        RGB *data = _back;
        _back = _front;
        _front = data;
        for (int i=0;i<_nSegments;++i)
        {
            auto & s = _segments[i];
            s.driver->write(s.num_leds, data);
            data += s.num_leds;
        }
        if (wait) {
            waitReady(1000);
        }
    }
    bool waitReady(uint32_t timeout_ms) override
    {
        bool done = true;
        for (int i=0;i<_nSegments;++i) {
            done &= _segments[i].driver->wait(timeout_ms);
        }
        return done;
    }
    void copyFrontToBack() override
    {
        memcpy(_back, _front, sizeof(RGB)*_totSize);
    }
    void release() override
    {
        for (int i=0;i<_nSegments;++i) {
            _segments[i].driver->unload();
            //TODO: remove when in-place driver new will be fixed
            delete reinterpret_cast<NeopixelDrv::RMT*>(_segments[i].driver);
        }
        free(_rawMem);
    }

    int _nSegments, _totSize;
    const SegmentInfo* _segments;
    RGB *_front, *_back;
    void* _rawMem;
};

static uint32_t calc_led_driver_size(DriverType type)
{
    switch(type){
        case DriverType::RMT : return sizeof(NeopixelDrv::RMT);
        default: return 0;
    }
}

static std::tuple<uint32_t,uint32_t> calc_alloc_size(const LedStripConfig& cfg)
{
    uint32_t total_alloc_size = 0;
    uint32_t total_led_count = 0;
    for (int i=0;i<cfg.num_segments;++i)
    {
        auto & segment = cfg.segments[i];
        total_led_count += segment.num_leds;
        total_alloc_size += calc_led_driver_size(segment.driver);
        total_alloc_size += sizeof(SegmentInfo);
    }
    total_alloc_size += cfg.num_buffers * sizeof(RGB) * total_led_count;
    total_alloc_size += sizeof(LedStripImpl);
    return {total_alloc_size, total_led_count};
}

static NeopixelDrv::Driver* create_driver(const LedSegmentConfig& cfg, uint8_t*& raw_mem)
{
    switch(cfg.driver) 
    {
        case DriverType::RMT: 
        {
            const auto & rmt_cfg = *reinterpret_cast<RMTDriverConfig*>(cfg.driver_config);
            auto * drv = new (raw_mem) NeopixelDrv::RMT(rmt_cfg.gpio, rmt_cfg.channel, rmt_cfg.mem_block_num, cfg.strip);
            raw_mem += sizeof(NeopixelDrv::RMT);
            return drv;
        }
        default:
            return nullptr;
    }
}

LedStrip* LedStrip::create(const LedStripConfig& cfg)
{
    auto [total_size,total_led_count] = calc_alloc_size(cfg);
    void* raw_mem = malloc(total_size);
    memset(raw_mem, 0xcd, total_size);
    auto *next_ptr = reinterpret_cast<uint8_t*>(raw_mem);
    auto *segments = reinterpret_cast<SegmentInfo*>(next_ptr);
    next_ptr += sizeof(SegmentInfo)*cfg.num_segments;
    for (int s=0;s<cfg.num_segments;++s)
    {
        segments[s].num_leds = cfg.segments[s].num_leds;
        segments[s].driver = create_driver(cfg.segments[s], next_ptr);
    }
    RGB* front = reinterpret_cast<RGB*>(next_ptr);
    next_ptr += sizeof(RGB) * total_led_count;
    RGB *back;
    if (2==cfg.num_buffers){
        back = reinterpret_cast<RGB*>(next_ptr);
        next_ptr += sizeof(RGB) * total_led_count;
    } else{
        back = front;
    }
    return  new (next_ptr) LedStripImpl(total_led_count, cfg.num_segments, segments, front, back, raw_mem);
}
}