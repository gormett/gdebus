#include "display.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "time.h"
#include "sntp_sync.h"
#include "bus_api.h"
#include "esp_crt_bundle.h"  // Add this line

#define MAIN_TASK_STACK_SIZE (8192)

void main_task(void* ignore) {
    static const char* TAG = "Main";
    u8g2_t u8g2;


    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    // Initialize SNTP
    // CET timezone, M3.5.0 = last sunday in March, M10.5.0 = last sunday in October 
    // More examples: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
    initialize_sntp("CET-1CEST,M3.5.0,M10.5.0/3");

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", 
        retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    // init_display(&u8g2);

    // Make the API request
    const char* response = get_bus_departures();
    ESP_LOGI(TAG, "API Response: %s", response);

    while (true) {
        time_t currentTime;
        time(&currentTime);
        
        draw_display(&u8g2, "Domov", "11:22", "1", currentTime);
        vTaskDelay(pdMS_TO_TICKS(100));  // Update every 1/10 second
    }
}

void app_main(void* ignore) {
    xTaskCreate(
        main_task,
        "main_task",
        MAIN_TASK_STACK_SIZE,  // Increased stack size
        NULL,
        5,
        NULL
    );
}
