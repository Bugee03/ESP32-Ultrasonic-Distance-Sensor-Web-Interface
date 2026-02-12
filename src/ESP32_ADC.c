//in this expample I read hcsr04 distance sensor I used adc1 because we cannot use adc2 when we
//wifi modul. I connected GPIO25 = Trigger and GPIO33 = ECHO
//trigger sends a pulse GPIO_MODE_OUTPUT
//echo reads the pulse width GPIO_MODE_INPUT




#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"  // ✅ Required for esp_rom_delay_us()

#define TRIG_PIN GPIO_NUM_25  // Use an output-capable GPIO
#define ECHO_PIN GPIO_NUM_33

void hc_sr04_task(void *pvParameters) {
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);

    while (1) {
        // Send 10µs trigger pulse
        gpio_set_level(TRIG_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2));  // Ensure stable low before triggering
        gpio_set_level(TRIG_PIN, 1);
        esp_rom_delay_us(10);  // ✅ Corrected delay function
        gpio_set_level(TRIG_PIN, 0);

        // Wait for Echo to go HIGH
        int64_t start_time = esp_timer_get_time();
        while (gpio_get_level(ECHO_PIN) == 0 && (esp_timer_get_time() - start_time) < 1000000);

        // Measure Echo HIGH duration
        int64_t echo_start = esp_timer_get_time();
        while (gpio_get_level(ECHO_PIN) == 1 && (esp_timer_get_time() - echo_start) < 1000000);
        int64_t echo_time = esp_timer_get_time() - echo_start;

        // Convert time to distance
        float distance_cm = echo_time * 0.034 / 2;

        printf("Distance: %.2f cm\n", distance_cm);
        vTaskDelay(pdMS_TO_TICKS(100)); // Read every 100ms
    }
}

void app_main() {
    xTaskCreate(hc_sr04_task, "hc_sr04_task", 2048, NULL, 5, NULL);
}
