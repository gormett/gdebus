#include <stdio.h>
#include "sntp_sync.h"
#include "esp_sntp.h"
#include "esp_log.h"

static const char* TAG = "sntp_sync";

void initialize_sntp(const char* timezone) {
    ESP_LOGI(TAG, "Initializing SNTP");

    // esp_err_t wifi_wait_count = 0;
    // while(esp_netif_get_nr_of_ifs() == 0 && wifi_wait_count < 10) {
    //     ESP_LOGW(TAG, "Waiting for WiFi connection...");
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     wifi_wait_count++;
    // }
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);

    ESP_LOGI(TAG, "Setting SNTP server 0");
    
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "0.sk.pool.ntp.org");
    esp_sntp_setservername(2, "1.sk.pool.ntp.org");
    esp_sntp_setservername(3, "time1.google.com");

    esp_sntp_set_sync_interval(15000);

    esp_sntp_init();

    setenv("TZ", timezone, 1);
    ESP_LOGI(TAG, "Timezone: %s", timezone);
    tzset();

    //     int retry = 0;
    // const int retry_count = 15;
    // while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
    //     ESP_LOGI(TAG, "Waiting for SNTP sync... (%d/%d)", retry, retry_count);
    //     vTaskDelay(2000 / portTICK_PERIOD_MS);
    // }

    // if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
    //     ESP_LOGW(TAG, "SNTP sync failed after %d retries", retry_count);
    // } else {
    //     ESP_LOGI(TAG, "SNTP sync completed successfully");
    // }
}

void func(void)
{

}
