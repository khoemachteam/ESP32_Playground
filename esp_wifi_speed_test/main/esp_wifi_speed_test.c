#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_timer.h"

// #define WIFI_SSID "Sanlab"
// #define WIFI_PASS "sanlab@"
// #define SERVER_IP "192.168.1.158"
#define WIFI_SSID "NQH"
#define WIFI_PASS "12345679"
#define SERVER_IP "192.168.75.34"
#define SERVER_PORT 8888
#define CHUNK_SIZE 1024
#define TOTAL_SIZE (1024 * 1024)  // 1MB mỗi lần
#define TEST_COUNT 10

static const char *TAG = "upload_test";
char dummy_audio[CHUNK_SIZE];

static void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

static void upload_task(void *pvParameters) {
    double speeds[TEST_COUNT];
    double total_speed = 0;

    for (int test_num = 0; test_num < TEST_COUNT; test_num++) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(SERVER_PORT);

        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (sock < 0 || connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
            ESP_LOGE(TAG, "Connection failed on test %d", test_num + 1);
            if (sock >= 0) close(sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        ESP_LOGI(TAG, "Test %d: Connected to server", test_num + 1);

        int total_sent = 0;
        int chunks = TOTAL_SIZE / CHUNK_SIZE;

        int64_t start = esp_timer_get_time();

        for (int i = 0; i < chunks; i++) {
            int bytes_sent = send(sock, dummy_audio, CHUNK_SIZE, 0);
            if (bytes_sent < 0) {
                ESP_LOGE(TAG, "Send failed on test %d", test_num + 1);
                break;
            }
            total_sent += bytes_sent;
        }

        int64_t end = esp_timer_get_time();
        double elapsed_sec = (end - start) / 1e6;
        double speed_mbps = (total_sent * 8.0) / (elapsed_sec * 1000000);

        speeds[test_num] = speed_mbps;
        total_speed += speed_mbps;

        ESP_LOGI(TAG, "Test %d: %.2f KB in %.2f sec = %.2f Mbps",
                 test_num + 1, total_sent / 1024.0, elapsed_sec, speed_mbps);

        close(sock);
        vTaskDelay(pdMS_TO_TICKS(1000));  // nghỉ giữa các lần test
    }

    ESP_LOGI(TAG, "===== Tổng kết =====");
    for (int i = 0; i < TEST_COUNT; i++) {
        ESP_LOGI(TAG, "Lần %d: %.2f Mbps", i + 1, speeds[i]);
    }
    ESP_LOGI(TAG, "Trung bình: %.2f Mbps", total_speed / TEST_COUNT);

    vTaskDelete(NULL);
}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(5000));  // chờ kết nối WiFi

    xTaskCreate(upload_task, "upload_task", 8192, NULL, 5, NULL);
}
