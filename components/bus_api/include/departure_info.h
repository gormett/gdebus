#ifndef DEPARTURE_INFO_H
#define DEPARTURE_INFO_H

#include "esp_attr.h"
#include "stdbool.h"

typedef struct {
    char planned_departure_time[64];  // ISO 8601 datetime string
    int platform_number;
    int delay_minutes;
    char destination[128];
    bool is_valid;  // New field to check if data is valid
} __attribute__((packed)) departure_info_t;

RTC_DATA_ATTR static departure_info_t rtc_departure_info;

#endif  // DEPARTURE_INFO_H
