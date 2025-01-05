#include <stdio.h>
#include "sntp_sync.h"
#include "esp_sntp.h"
#include "esp_log.h"

static const char* TAG = "sntp_sync";

void initialize_sntp(const char* timezone) {
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "0.sk.pool.ntp.org");
    esp_sntp_init();

    setenv("TZ", timezone, 1);
    tzset();
}

void func(void)
{

}
