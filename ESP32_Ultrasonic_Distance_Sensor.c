#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#define TRIG_PIN GPIO_NUM_25  // ✅ OUTPUT
#define ECHO_PIN GPIO_NUM_33  // ✅ INPUT

static const char *TAG = "ESP32_WEB";
static float latest_distance = 0.0;

const char *ssid = "";
const char *pass = "";
int retry_num = 0;

// 📌 Function to Measure Distance
float measure_distance_cm() {
    gpio_set_level(TRIG_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0 && (esp_timer_get_time() - start_time) < 1000000);

    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1 && (esp_timer_get_time() - echo_start) < 1000000);

    int64_t echo_time = esp_timer_get_time() - echo_start;
    return (echo_time * 0.034 / 2);  // Convert to cm
}

// 📌 HTTP Request Handler (Sends Distance to Web Page)
// Your existing endpoint handler (for example, for `/distance`)
esp_err_t distance_handler(httpd_req_t *req) {
    // Set CORS header
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    // Return the distance data as before
    float distance = measure_distance_cm();
    char payload[32];
    snprintf(payload, sizeof(payload), "%.2f cm", distance);

    httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


// 📌 Start HTTP Server
httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t get_distance_uri = {
                .uri = "/distance",
                .method = HTTP_GET,
                .handler = distance_handler,  // Change to distance_handler
                .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &get_distance_uri);
    }
    return server;
}
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        printf("WiFi Connecting...\n");
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        printf("WiFi Connected\n");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi Lost Connection\n");
        if (retry_num < 5) {
            esp_wifi_connect();
            retry_num++;
            printf("Retrying to Connect...\n");
        } else {
            printf("Connection Failed\n");
        }
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        printf("WiFi Got IP\n\n");
    }
}

void wifi_connection() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
            .sta = {
                    .ssid = "",
                    .password = "",
                    .threshold.authmode = WIFI_AUTH_WPA_PSK // Set WPA authentication mode
            }
    };

    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, pass);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    printf("WiFi Configured: SSID: %s, PASSWORD: %s\n", ssid, pass);
}
// 📌 Task: Measure Distance Periodically
void distance_task(void *pvParameters) {
    while (1) {
        latest_distance = measure_distance_cm();
        ESP_LOGI(TAG, "Distance: %.2f cm", latest_distance);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 📌 Main Function
void app_main() {
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);


    // Initialize Wi-Fi
    nvs_flash_init();
    wifi_connection();


    start_webserver();
    xTaskCreate(distance_task, "distance_task", 4096, NULL, 5, NULL);
}
