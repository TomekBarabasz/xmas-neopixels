#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include <neopixels.h>

extern "C" void tcp_server_task(void *pvParameters);

extern "C" void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //          function,           name,        stack, params,priority, [out]handler
    xTaskCreate(tcp_server_task,    "tcp_server", 4096, NULL,  5, NULL);
    xTaskCreate(Neopixel::app_main,  "neopixels",  4096, NULL,  5, NULL);
}
