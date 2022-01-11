#include <driver/i2s.h>
#include <driver/adc.h>
#include <neopixel_audio.h>

int configure_i2s_sampler(int sample_rate/*=44100*/)
{
    static const i2s_port_t I2S_PORT_NUMBER = I2S_NUM_0;
    static const adc_unit_t adcUnit = ADC_UNIT_1;
    static adc1_channel_t adcChannel = ADC1_CHANNEL_0;

#if 0
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .tx_desc_auto_clear = false,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = 1,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .fixed_mclk = 0
    };
#else
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};
#endif
    ESP_ERROR_CHECK(i2s_driver_install(I2S_PORT_NUMBER, &i2s_config, 0, NULL));

    static const i2s_pin_config_t pin_config = {
        .bck_io_num   = GPIO_NUM_32,
        .ws_io_num    = GPIO_NUM_25,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = GPIO_NUM_33
    };

    ESP_ERROR_CHECK(i2s_set_pin(I2S_PORT_NUMBER, &pin_config));
    ESP_ERROR_CHECK(i2s_set_adc_mode(adcUnit,adcChannel));
    ESP_ERROR_CHECK(i2s_adc_enable(I2S_PORT_NUMBER));

    return 0;
}
