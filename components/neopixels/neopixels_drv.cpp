#include <stdio.h>
#include <string.h>
#include <tuple>
#include <driver/gpio.h>
#include <driver/rmt.h>
#include <neopixels.h>
#include <neopixels_drv.h>

namespace NeopixelDrv
{
static void IRAM_ATTR rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    
}
struct RMT : public Driver
{
    RMT(gpio_num_t gpio_port, rmt_channel_t channel, int mem_block_num, const Timing& tm) :
        tx_channel( channel)
    {
        rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio_port, tx_channel);
        // set counter clock to 40MHz
        config.clk_div = 2;
        config.mem_block_num = mem_block_num;

        ESP_ERROR_CHECK(rmt_config(&config));
        ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
        initTiming(tm);
        rmt_translator_init(config.channel, rmt_adapter);
    }
    void initTiming(const Timing& tm)
    {
        uint32_t counter_clk_hz = 0;
        ESP_ERROR_CHECK(rmt_get_counter_clock(tx_channel, &counter_clk_hz));
        // ns -> ticks
        const float ratio = (float)counter_clk_hz / 1e9;
        t0h_ticks   = (uint32_t)(ratio * tm.T0H_NS);
        t0l_ticks   = (uint32_t)(ratio * tm.T0L_NS);
        t1h_ticks   = (uint32_t)(ratio * tm.T1H_NS);
        t1l_ticks   = (uint32_t)(ratio * tm.T1L_NS);
        reset_ticks = (uint32_t)(ratio * tm.RESET_NS);
    }
    void write(int size, Neopixel::RGB* data) override
    {

    }
    const rmt_channel_t tx_channel;
    uint16_t t0h_ticks, t0l_ticks, t1h_ticks, t1l_ticks, reset_ticks;
};
}

namespace Neopixel
{  
#define MAKE_RGB(R,G,B) { uint8_t((R)/100), uint8_t((G)/100), uint8_t((B)/100) }
RGB HSV::toRGB() const
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

struct SegmentInfo
{
    int num_leds;
    NeopixelDrv::Driver *driver;
};

struct LedStripImpl : public LedStrip
{
    LedStripImpl(int totSize, int nSegments, SegmentInfo* segments, RGB* front, RGB* back, void* rawMem) : 
        _nSegments(nSegments), _totSize(totSize), _segments(segments), 
        _front(front), _back(back),
        _newData(false),
        _rawMem(rawMem)
    {}
    int getLength() const override { return _totSize; } 
    RGB* getBuffer() override { return _back; }
    void setPixelRGB(int first, int count, const RGB* rgb) override
    {
        memcpy(_back + first, rgb, count * sizeof(RGB));
        _newData = true;
    }
    void setPixelHSV(int first, int count, const HSV* hsv) override
    {
        for (int i=0; i<count; ++i) {
            _back[first + i] = hsv[i].toRGB();
        }
        _newData = true;
    }
    void refresh() override
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
    }
    void release() override
    {
        free(_rawMem);
    }

    RGB& operator[](int i) { return _back[i];}
    const RGB& operator[](int i) const { return _back[i];}

    int _nSegments, _totSize;
    const SegmentInfo* _segments;
    RGB *_front, *_back;
    bool _newData;
    void* _rawMem;
};

inline uint16_t operator "" _us(unsigned long long value)
{
    return static_cast<uint16_t>(value);
}

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
    total_alloc_size += 2 * sizeof(RGB)*total_led_count;
    total_alloc_size += sizeof(LedStripImpl);
    return {total_alloc_size, total_led_count};
}
static const NeopixelDrv::Timing& get_timing(SegmentType type)
{
    static const NeopixelDrv::Timing ws2811 { 500, 2000, 1200, 1300, 500 };
    static const NeopixelDrv::Timing ws2812 { 350, 1000, 1000,  350, 280 };
    switch(type){
        default:
        case SegmentType::WS2811: return ws2811;
        case SegmentType::WS2812: return ws2812;
    }
}
static NeopixelDrv::Driver* create_driver(const LedSegmentConfig& cfg, uint8_t*& raw_mem)
{
    switch(cfg.driver){
        case DriverType::RMT: 
        {
            static rmt_channel_t channel[] = {RMT_CHANNEL_0,RMT_CHANNEL_1,RMT_CHANNEL_2,RMT_CHANNEL_3,RMT_CHANNEL_4,RMT_CHANNEL_5,RMT_CHANNEL_6,RMT_CHANNEL_7};
            const auto & rmt_cfg = *reinterpret_cast<RMTDriverConfig*>(cfg.driver_config);
            auto * drv = new (raw_mem) NeopixelDrv::RMT((gpio_num_t)rmt_cfg.gpio, 
                                        channel[rmt_cfg.channel], rmt_cfg.mem_block_num, get_timing(cfg.strip));
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
    RGB* back = reinterpret_cast<RGB*>(next_ptr);
    next_ptr += sizeof(RGB) * total_led_count;
    return  new (next_ptr) LedStripImpl(total_led_count, cfg.num_segments, segments, front, back, raw_mem);
}
}