#include "display.h"

#include <u8g2.h>

#include "ico_clock.h"
#include "time.h"
#include "u8g2_esp32_hal.h"

// static const unsigned char image_clock_quarters_bits[] = {0xe0, 0x03, 0x98, 0x0c, 0x84, 0x10, 0x02, 0x20, 0x82, 0x20, 0x81, 0x40, 0x81, 0x40, 0x87, 0x70, 0x01, 0x41, 0x01, 0x42, 0x02, 0x20, 0x02, 0x20, 0x84, 0x10, 0x98, 0x0c, 0xe0, 0x03, 0x00, 0x00};
// static const unsigned char image_Layer_9_bits[] = {0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01, 0x9f, 0xc7, 0x21, 0x04, 0x01};
// static const unsigned char image_Layer_10_bits[] = {0x0f, 0x03, 0x01, 0x01};

void init_display(u8g2_t *u8g2) {
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.bus.i2c.sda = PIN_SDA;
    u8g2_esp32_hal.bus.i2c.scl = PIN_SCL;
    u8g2_esp32_hal.reset = RST_PIN;
    u8g2_esp32_hal_init(u8g2_esp32_hal);

    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        u8g2, U8G2_R0,
        // u8x8_byte_sw_i2c,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb);  // init u8g2 structure
    u8x8_SetI2CAddress(&u8g2->u8x8, I2C_ADDRESS);

    u8g2_InitDisplay(u8g2);      // send init sequence to the display, display is in
                                 // sleep mode after this,
    u8g2_SetPowerSave(u8g2, 0);  // wake up display
}

void draw_delayTimeMinutes(u8g2_t *u8g2, char *delayTimeMinutes) {
    // char to int
    int delayTimeMinutesInt = atoi(delayTimeMinutes);
    // Moves the text to the left if the delay time is more than 1 digit
    u8g2_SetFont(u8g2, u8g2_font_profont29_tr);
    if (delayTimeMinutesInt > 9) {
        u8g2_DrawStr(u8g2, 98, 45, delayTimeMinutes);
    } else {
        u8g2_DrawStr(u8g2, 114, 45, delayTimeMinutes);
    }
}

void draw_display(u8g2_t *u8g2, char *destinationStop, char *departureTime, char *delayTimeMinutes, time_t currentTime) {
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFontMode(u8g2, 1);
    u8g2_SetBitmapMode(u8g2, 1);

    // Destination
    u8g2_SetFont(u8g2, u8g2_font_profont17_tr);
    u8g2_DrawStr(u8g2, 5, 13, destinationStop);

    // Current time
    // u8g2_SetFont(u8g2, u8g2_font_profont22_tr);
    // u8g2_DrawStr(u8g2, 21, 64, "12:00:00");
    char timeStr[20];
    struct tm *tm_info = localtime(&currentTime);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);
    u8g2_SetFont(u8g2, u8g2_font_profont22_tr);
    u8g2_DrawStr(u8g2, 21, 64, timeStr);

    // Departure Time
    u8g2_SetFont(u8g2, u8g2_font_profont29_tr);
    u8g2_DrawStr(u8g2, 0, 45, departureTime);
    // mbtles crt bundle
    // Delay Time Minutes
    draw_delayTimeMinutes(u8g2, delayTimeMinutes);

    // Graphics
    u8g2_SetFont(u8g2, u8g2_font_6x12_tr);
    u8g2_DrawStr(u8g2, 0, 24, "Odchod");
    u8g2_DrawStr(u8g2, 99, 24, "Meska");
    u8g2_DrawXBM(u8g2, 0, 49, 15, 16, image_clock_quarters_bits);
    u8g2_SetDrawColor(u8g2, 2);
    u8g2_DrawBox(u8g2, 0, 0, 62, 15);
    u8g2_SetDrawColor(u8g2, 1);
    // u8g2_DrawXBM(u8g2,64, 0, 33, 15, image_Layer_9_bits);
    u8g2_SetDrawColor(u8g2, 2);
    // u8g2_DrawXBM(u8g2,0, 0, 4, 4, image_Layer_10_bits);
    u8g2_SendBuffer(u8g2);
}
