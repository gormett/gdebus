#include "time.h"

#ifndef DISPLAY_H
#define DISPLAY_H

#include <u8g2.h>
#include "departure_info.h"

#define PIN_SDA 21  // GPIO21
#define PIN_SCL 22  // GPIO22
#define RST_PIN 16  // GPIO16
#define I2C_ADDRESS 0x78

void init_display(u8g2_t *u8g2);
void turn_on_display(u8g2_t *u8g2);
void turn_off_display(u8g2_t *u8g2);
void draw_display(u8g2_t *u8g2, departure_info_t *departure_info, time_t currentTime);
void draw_logo(u8g2_t *u8g2);

#endif  // DISPLAY_H
