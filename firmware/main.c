#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "wifi_connect.h"
#include "image_config.h"

#define HOME

#define PHONE

#ifdef OFFICE
    #define WIFI_SSID ""
    #define WIFI_PASSWORD ""
    #define HOST_IP_ADDR ""
#elif defined PHONE
    #define WIFI_SSID ""
    #define WIFI_PASSWORD ""
    #define HOST_IP_ADDR ""
#elif defined HOME
    #define WIFI_SSID ""
    #define WIFI_PASSWORD ""
    #define HOST_IP_ADDR ""
#endif

#define my_delay(time) vTaskDelay((time) / portTICK_PERIOD_MS)
#define PORT 5000
#define CHUNK_SIZE 512 

#define TAG_UDP  "UDP"
int error = -1;

static void Stream_task(void *pvParameters)
{
    char rx_buffer[128];
    // Initialize camera
    esp_err_t err_cam = esp_camera_init(&camera_config);
    if (err_cam != ESP_OK) {
        ESP_LOGE("CAMERA", "Camera init failed with error 0x%x", err_cam);
        return;
    }
    ESP_LOGE("CAMERA","Camera Init successfully\n");
    while (1) {

        struct sockaddr_in dest_addr_udp;
        dest_addr_udp.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr_udp.sin_family = AF_INET;
        dest_addr_udp.sin_port = htons(PORT);

        int sock_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock_udp < 0) {
            ESP_LOGE(TAG_UDP, "Unable to create socket: errno %d", errno);
            break;
        }

        // Set timeout
        //struct timeval timeout;
        //timeout.tv_sec = 10;
        //timeout.tv_usec = 0;
        //setsockopt (sock_udp, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        ESP_LOGI(TAG_UDP, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);
        // remark UDP doesn't require to connect
        while (1) {
            //Capture image
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                ESP_LOGE("UDP STREAM", "Camera capture failed");
                shutdown(sock_udp, 0);
                close(sock_udp);
                break;
            }

            // Send raw YUV422 data
            int total_sent = 0;
            int to_send = fb->len;
            const char *data = (const char*)fb->buf;

            ESP_LOGI("UDP STREAM", "YUV422 size: %d bytes", to_send);
            while (total_sent < to_send) {
                int remaining = to_send - total_sent;
                int send_size = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;
                int bytes_sent = sendto(sock_udp, data + total_sent, send_size, 0, (struct sockaddr *)&dest_addr_udp, sizeof(dest_addr_udp));
                if (bytes_sent < 0) {
                    ESP_LOGE("UDP STREAM", "Send failed: errno %s", strerror(errno));
                    break;
                }

                total_sent += bytes_sent;
                ESP_LOGI("UDP STREAM", "Sent %d bytes, total: %d/%d", bytes_sent, total_sent, to_send);
                my_delay(10);
            }

            esp_camera_fb_return(fb);
            fb = NULL;
            ESP_LOGI("UDP STREAM", "Successfully sent");

            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock_udp, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG_UDP, "recvfrom failed: errno %d", errno);
                break;
            }else { // Data received
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG_UDP, "Received %d bytes from %s:", len, HOST_IP_ADDR);
                ESP_LOGI(TAG_UDP, "%s", rx_buffer);
            }

            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        if (sock_udp != -1) {
            ESP_LOGE(TAG_UDP, "Shutting down socket and restarting...");
            shutdown(sock_udp, 0);
            close(sock_udp);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    wifi_connection(WIFI_SSID, WIFI_PASSWORD);
    xTaskCreate(Stream_task, "udp_client", 4096, NULL, 5, NULL);
}
