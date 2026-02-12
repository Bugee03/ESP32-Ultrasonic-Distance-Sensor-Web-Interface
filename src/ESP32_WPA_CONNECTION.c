#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "esp_timer.h"


// 📌 MQTT Broker Address (Change this to your server IP)
#define MQTT_BROKER_URI "mqtt://192.168.1.101:1883"  // Replace with your MQTT broker IP

// 📌 HC-SR04 Pin Configuration
#define TRIG_PIN GPIO_NUM_25  // ✅ OUTPUT
#define ECHO_PIN GPIO_NUM_33  // ✅ INPUT

static const char *TAG = "MQTT";
esp_mqtt_client_handle_t client;
const char *ssid = "";
const char *pass = "";
int retry_num = 0;

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

// 📌 Measure Distance using HC-SR04
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

// 📌 MQTT Event Handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected!");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected!");
            break;
        default:
            break;
    }
}

// 📌 Task: Measure Distance and Publish via MQTT
void mqtt_publish_task(void *pvParameters) {
    char payload[32];

    while (1) {
        float distance = measure_distance_cm();
        snprintf(payload, sizeof(payload), "%.2f cm", distance);

        ESP_LOGI(TAG, "Publishing: %s", payload);
        esp_mqtt_client_publish(client, "esp32/distance", payload, 0, 0, 0);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Send every 1s
    }
}

// 📌 Start MQTT Client
void mqtt_app_start() {
    esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = MQTT_BROKER_URI,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

// 📌 Main Function
void app_main() {
    // Initialize NVS for Wi-Fi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Set GPIO Directions
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);

    // Initialize Wi-Fi
    nvs_flash_init();
    wifi_connection();

    // Start MQTT
    mqtt_app_start();

    // Start MQTT Publishing Task
    xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 5, NULL);
}
