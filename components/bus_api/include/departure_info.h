#ifndef DEPARTURE_INFO_H
#define DEPARTURE_INFO_H

typedef struct {
    char planned_departure_time[64];  // ISO 8601 datetime string
    int platform_number;
    int delay_minutes;
    char destination[128];
} departure_info_t;

#endif  // DEPARTURE_INFO_H
