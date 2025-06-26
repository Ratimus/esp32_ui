#pragma once

#include <U8g2lib.h>

class Display : public U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C
{
  Display(void);

public:
  void start_display(void);
  static Display *instance(void)
  {
    static Display __instance__;
    return &__instance__;
  }

  uint8_t char_width() const { return 8; }
  uint8_t half_width() const { return instance()->getWidth() / 2; }
};
