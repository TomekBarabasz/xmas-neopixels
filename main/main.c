#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_event.h"

extern void tcp_server_task(void *pvParameters);
extern void neopixel_task  (void *pvParameters);

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //          function,        name,        stack, params,priority, [out]handler
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL,  5, NULL);
    xTaskCreate(neopixel_task,   "neopixels",  4096, NULL,  5, NULL);
}
