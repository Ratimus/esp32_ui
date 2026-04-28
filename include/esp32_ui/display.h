#pragma once

#include <U8g2lib.h>
#include <stdint.h>

/**
 * @file display.h
 * @brief Abstracts display handling for menu system.
 *
 * This header provides the interface for interacting with the display used by the menu/UI
 * system. It wraps a hardware-specific U8g2 display driver. The actual implementation of
 * the constructor and startup routine is expected to be provided by the project that
 * links against this library.
 *
 * HOW TO USE IN A PROJECT:
 * - Create a `display_config.h` in your project that defines DISPLAY_BASE to the appropriate
 *   U8g2 class (e.g., `U8G2_SH1106_128X64_WINSTAR_F_4W_HW_SPI`).
 * - Implement `Display::Display()` and `Display::start_display()` in a `display.cpp` file
 *   within your project.
 *
 * The shared library provides only the interface and core accessors.
 */

// Must be defined in the consuming project before including this file:
// #define DISPLAY_BASE U8G2_SH1106_128X64_WINSTAR_F_4W_HW_SPI
#ifndef DISPLAY_BASE
#error "DISPLAY_BASE must be defined before including display.h. Define it in your project."
#endif

namespace esp32_ui
{
  class Display : public DISPLAY_BASE
  {
    Display(); // Implementation must be defined by the consuming project

  public:
    /**
     * @brief Initializes the display hardware.
     * Call once during startup before drawing.
     */
    void start_display();

    /**
     * @brief Singleton accessor for the global display instance.
     */
    static Display *instance()
    {
      static Display __instance__;
      return &__instance__;
    }

    /**
     * @brief Returns the fixed-width character width in pixels.
     */
    uint8_t char_width() const { return 8; }

    /**
     * @brief Returns half of the display width in pixels (useful for centering).
     */
    uint8_t half_width() const { return instance()->getWidth() / 2; }
  };

} // namespace esp32_ui
