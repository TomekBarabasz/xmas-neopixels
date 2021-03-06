#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <neopixel_app.h>

#define PORT                        1234
#define KEEPALIVE_IDLE              30
#define KEEPALIVE_INTERVAL          30
#define KEEPALIVE_COUNT             10
#define CONFIG_EXAMPLE_IPV4
static const char *TAG = "WIFI";

static int initialize_socket()
{
#if defined(CONFIG_EXAMPLE_IPV4) 
    const int addr_family =  AF_INET;
#elif defined(CONFIG_EXAMPLE_IPV6)
    int addr_family =  AF_INET6
#endif
    int ip_protocol = 0;
    struct sockaddr_storage dest_addr;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
#ifdef CONFIG_EXAMPLE_IPV6
    else if (addr_family == AF_INET6) {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return -1;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    return listen_sock;
    
CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
    return -1;
}
static void handle_incomming_data(int sock, esp_event_loop_handle_t loop_handle)
{
    uint8_t rx_buffer[256];
    int nbytes;
    do
    {
        nbytes = recv(sock, rx_buffer, sizeof(rx_buffer), 0);
        if (nbytes < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (nbytes == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else 
        {
            ESP_ERROR_CHECK(esp_event_post_to(loop_handle, NeopixelApp::NEOPIXEL_EVENTS, 0, rx_buffer, nbytes, portMAX_DELAY));
        }
    } while(nbytes > 0);
}
static int connect_socket(int listen_sock)
{
    const int keepAlive = 1;
    const int keepIdle = KEEPALIVE_IDLE;
    const int keepInterval = KEEPALIVE_INTERVAL;
    const int keepCount = KEEPALIVE_COUNT;

    ESP_LOGI(TAG, "Socket listening");

    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t addr_len = sizeof(source_addr);
    int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
        return -1;
    }

    // Set tcp keepalive option
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
    
    // Convert ip address to string
    char addr_str[128];
    if (source_addr.ss_family == PF_INET) {
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
    }
#ifdef CONFIG_EXAMPLE_IPV6
    else if (source_addr.ss_family == PF_INET6) {
        inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
    }
#endif
    ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

    return sock;
}
static void tcp_server_task_main(esp_event_loop_handle_t event_loop)
{
    int listen_sock = initialize_socket();

    for(;;)
    {
        int sock = connect_socket(listen_sock);
        if (sock < 0) continue;
        handle_incomming_data(sock,event_loop);
        shutdown(sock, 0);
        close(sock);
    }

    close(listen_sock);
    vTaskDelete(NULL);
}
extern "C" void tcp_server_task(void* params)
{
    ESP_ERROR_CHECK(example_connect());
    auto loop_handle = reinterpret_cast<esp_event_loop_handle_t>(params); 
    tcp_server_task_main(loop_handle);
}