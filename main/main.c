#include "bus_api.h"
#include "departure_info.h"
#include "display.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sntp_sync.h"
#include "string.h"
#include "time.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include "wifi.h"

#define MAIN_TASK_STACK_SIZE (8192)
#define UPDATE_INTERVAL_MS 30000  // Update every 30 seconds

#define WAKEUP_GPIO GPIO_NUM_33

void deep_sleep(u8g2_t* u8g2) {
    // Turn off display
    // turn_off_display(u8g2);

    // Configure wake-up source
    rtc_gpio_pulldown_en((gpio_num_t)WAKEUP_GPIO);
    rtc_gpio_pullup_dis((gpio_num_t)WAKEUP_GPIO);
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 1);

    printf("Going to deep sleep now\n");
    esp_deep_sleep_start();
}

void store_departure_to_rtc(departure_info_t* departure) {
    if (departure != NULL) {
        memcpy(&rtc_departure_info, departure, sizeof(departure_info_t));
        rtc_departure_info.is_valid = true;
    }
}

departure_info_t* get_departure_from_rtc(void) {
    if (rtc_departure_info.is_valid) {
        return &rtc_departure_info;
    }
    return NULL;
}

time_t get_time_from_rtc(void) {
    time_t now;
    struct tm timeinfo;

    time(&now);   // Get current time from RTC
    now += 3600;  // Add 1 hour for GMT+1
    localtime_r(&now, &timeinfo);

    // Check if time needs initialization
    if (timeinfo.tm_year < (2023 - 1900)) {
        return 0;  // Invalid time, needs SNTP sync
    }

    return now;
}

void main_task(void* ignore) {
    static const char* TAG = "Main";
    departure_info_t* departure = get_departure_from_rtc();

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    u8g2_t u8g2;
    init_display(&u8g2);

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        ESP_LOGI(TAG, "Wakeup caused by button press");
        // Add your wake-up actions here
        time_t current_time = get_time_from_rtc();
        draw_display(&u8g2, departure, current_time);
    } else {
        ESP_LOGI(TAG, "First boot");
        draw_logo(&u8g2);
    }

    // vTaskDelay(pdMS_TO_TICKS(5000));
    // deep_sleep();

    // Initialize display

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

    // delay for 2 seconds
    // vTaskDelay(2000 / portTICK_PERIOD_MS);
    // Wait for time to be set
    time_t now;
    char strftime_buf[64];
    struct tm timeinfo;
    // int retry = 0;
    // const int retry_count = 10;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);

    // while (timeinfo.tm_year < (2020 - 1900) && ++retry < retry_count) {
    //     ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    //     vTaskDelay(2000 / portTICK_PERIOD_MS);
    //     time(&now);
    //     localtime_r(&now, &timeinfo);
    //     strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    //     ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
    // }

    // Get departure info ONCE at startup
    ESP_LOGI(TAG, "Fetching initial bus departure information...");
    departure = (departure_info_t*)get_bus_departures();
    if (departure == NULL) {
        ESP_LOGE(TAG, "Failed to get departure information");
        // Could add error handling here (e.g., display error message)
    } else {
        store_departure_to_rtc(departure);
        ESP_LOGI(TAG, "Successfully retrieved departure info:");
        ESP_LOGI(TAG, "Time: %s", departure->planned_departure_time);
        ESP_LOGI(TAG, "Destination: %s", departure->destination);
        ESP_LOGI(TAG, "Delay: %d minutes", departure->delay_minutes);
        ESP_LOGI(TAG, "Platform: %d", departure->platform_number);
    }
    // Main loop - updates display for 8 seconds then sleeps
    uint32_t start_time = xTaskGetTickCount();
    while (xTaskGetTickCount() - start_time < pdMS_TO_TICKS(8000)) {
        time_t currentTime;
        time(&currentTime);
        if (departure != NULL) {
            draw_display(&u8g2, departure, currentTime);
        }
        // Small delay between display updates
        vTaskDelay(pdMS_TO_TICKS(100));  // Update display every 100ms
    }

    deep_sleep(&u8g2);
}

void app_main(void* ignore) {
    xTaskCreate(
        main_task,
        "main_task",
        MAIN_TASK_STACK_SIZE,  // Increased stack size
        NULL,
        5,
        NULL);
}
