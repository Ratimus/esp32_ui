#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

#include <esp32_ui/display.h>

#define I2C_MASTER_SCL_IO 22 // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO 21 // GPIO number for I2C master data

Display::Display(void)
    : U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO)
{
}

void Display::start_display(void)
{
  begin();
  setBusClock(getBusClock() * 4);
  setFont(u8g2_font_6x10_tf);
  setFontRefHeightExtendedText();
  setDrawColor(1);
  setFontPosTop();
  setFontDirection(0);
  Serial.printf("display started; speed = %u\n", getBusClock());
}
