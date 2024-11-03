#include "time.h"

#ifndef DISPLAY_H
#define DISPLAY_H

#include <u8g2.h>

#define PIN_SDA 21  // GPIO21
#define PIN_SCL 22  // GPIO22
#define I2C_ADDRESS 0x78

void init_display(u8g2_t *u8g2);
void draw_display(u8g2_t *u8g2, char *destinationStop, char *departureTime, char *delayTimeMinutes, time_t currentTime);

#endif  // DISPLAY_H
