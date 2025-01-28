#ifndef DISPLAY_H
#define DISPLAY_H

#include <u8g2.h>

#include "departure_info.h"
#include "time.h"

void init_display(u8g2_t *u8g2);
void draw_display(u8g2_t *u8g2, departure_info_t *departure_info, time_t currentTime);

#endif  // DISPLAY_H
