#include <stdio.h>
#include <string.h>
#include <driver/rmt.h>
#include "neopixels.h"

namespace Neopixel
{
static rmt_channel_t get_channel_number(WS2811::Channel chn) 
{
    switch( chn ) 
    {
        default:
        case WS2811::CH0 : return RMT_CHANNEL_0;
        case WS2811::CH1 : return RMT_CHANNEL_1;
        case WS2811::CH2 : return RMT_CHANNEL_2;
        case WS2811::CH3 : return RMT_CHANNEL_3;
        case WS2811::CH4 : return RMT_CHANNEL_4;
        case WS2811::CH5 : return RMT_CHANNEL_5;
        case WS2811::CH6 : return RMT_CHANNEL_6;
        case WS2811::CH7 : return RMT_CHANNEL_7;
    }
}
WS2811::WS2811(gpio_num_t gpio_port, Channel chn, int mem_block_num)
{
    const rmt_channel_t tx_channel = get_channel_number(chn);
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio_port, tx_channel);
    // set counter clock to 40MHz
    config.clk_div = 2;
    config.mem_block_num = mem_block_num;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
}

void WS2811::write(int size, RGB* data)
{

}

LedStrip::LedStrip(int ndrivers, Config* cfg) : _ndrivers(ndrivers), _cfg(cfg), _newData(false)
{
    _totSize = 0;
    for (int i=0;i<ndrivers;++i) {
        _totSize += cfg[i].size;
    }
    const auto alloc_size = _totSize * sizeof(RGB);
    _front = (RGB*) malloc(2 * alloc_size);
    _back = _front + _totSize;
}

void LedStrip::setRGB(int first, int count, RGB* rgb)
{
    memcpy(_back + first, rgb, count * sizeof(RGB));
    _newData = true;
}

inline uint16_t operator "" _us(unsigned long long value)
{
    return static_cast<uint16_t>(value);
}

void LedStrip::setHSV(int first, int count, HSV* hsv)
{
    for (int i=0; i<count; ++i) {
        _back[first + i] = hsv[i].toRGB();
    }
    _newData = true;
}

int LedStrip::refresh()
{
    RGB *data = _back;
    _back = _front;
    for (int i=0;i<_ndrivers;++i) 
    {
        auto & c = _cfg[i];
        c.drv->write(c.size, data);
        data += c.size;
    }
    return 0;
}

}