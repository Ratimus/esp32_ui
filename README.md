# ESP32 UI Library

A lightweight, modular UI framework for ESP32 projects using the U8g2 OLED display library. Designed for creating menu-driven interfaces with buttons and rotary encoders, optimized for small OLED displays (e.g., 128x32).

---

## Features

- Menu system with submenus, cursors, and event routing  
- Easy input handling via buttons and rotary encoders  
- Screen saver support with graphical effects  
- Abstract display interface supporting any U8g2-compatible display  
- FreeRTOS-based tasks for smooth UI updates and display refresh  

---

## Architecture Overview

- **UIManager**: Base class managing UI update/draw cycle and event dispatch.  
- **EventRouter**: Central event dispatching system for UI events.  
- **Display**: Abstract interface wrapping U8g2 display driver.  
- **SamplerUI** (example): Custom UI inheriting from `UIManager` to implement domain-specific menus and handlers.  

---

## Getting Started

### 1. Include the Library

Add the `esp32_ui` library to your project either as a git submodule or copy the source files.

### 2. Define Your Display Driver
The `esp32_ui` library expects you to provide a concrete `Display` class that inherits from the hardware-specific U8g2 display driver. You must define `DISPLAY_BASE` in platformio.ini for your project (see step 3).
Then, create a `display.cpp` in your project defining the concrete `Display` class that inherits from the appropriate U8g2 driver.

Example (`display.cpp`):

```cpp
#include <esp32_ui/display.h>

#define DISPLAY_BASE U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C

namespace esp32_ui
{
  Display::Display() : DISPLAY_BASE(U8G2_R0, U8X8_PIN_NONE, /* SCL Pin */ 22, /* SDA Pin */ 21)
  {
  }

  void Display::start_display()
  {
    begin();
    setBusClock(getBusClock() * 4);
    setFont(u8g2_font_6x10_tf);
    setFontRefHeightExtendedText();
    setDrawColor(1);
    setFontPosTop();
    setFontDirection(0);
  }

  Display *Display::instance()
  {
    static Display __instance__;
    return &__instance__;
  }

  uint8_t Display::char_width() const
  {
    return 8;
  }

  uint8_t Display::half_width() const
  {
    return getWidth() / 2;
  }
}
```

### 3. Configure platformio.ini

Make sure your platformio.ini defines the DISPLAY_BASE macro and includes dependencies:
```
[env:esp32dev]
platform = espressif32@6.10.0
board = esp32dev
framework = espidf, arduino
build_flags =
  -std=gnu++2a
  -DDISPLAY_BASE=U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C
lib_deps =
  olikraus/U8g2@^2.36.5
monitor_speed = 115200
```

Adjust the DISPLAY_BASE to the U8g2 driver matching your hardware.

### 4. Implement Your UI Class

Create a class inheriting from `esp32_ui::UIManager`. This class will manage the menu, input handling, and screen drawing.

Example header (`sampler_ui.h`):

```cpp
#pragma once

#include <memory>
#include <esp32_ui/ui_manager.h>
#include "hw_inputs.h"
#include "ch_menu.h"

namespace esp32_ui
{
  class SamplerUI : public UIManager
  {
    HWInputs *inputs = nullptr;

  public:
    SamplerUI();
    ~SamplerUI() = default;

    void encoder_handler();
    void button_handler();

    virtual void update() override
    {
      encoder_handler();
      button_handler();
    }

    virtual void screen_saver() override;
  };
}
```

Example implementation snippet (`sampler_ui.cpp`):

```cpp
#include "sampler_ui.h"
#include <esp32_ui/event_router.h>
#include <esp32_ui/display.h>

namespace esp32_ui
{
  SamplerUI::SamplerUI()
      : UIManager(std::make_unique<Root>("Main Menu"))
  {
    inputs = HWInputs::instance();
    root_node_ptr->set_fixed_cursor(true);
    // Setup menus and submenus here
  }

  void SamplerUI::button_handler()
  {
    for (int idx = 0; idx < NUM_PUSHBUTTONS; ++idx)
    {
      MenuEvent ev = inputs->read_button(idx);
      if (ev.type == MenuEvent::Type::NoType)
        continue;

      // handle button events
    }
  }

  void SamplerUI::encoder_handler()
  {
    MenuEvent ev = inputs->read_encoder();
    if (ev.type == MenuEvent::Type::NoType)
      return;

    dispatch_event(ev);
  }

  void SamplerUI::screen_saver()
  {
    // Implement a simple screen saver animation or blank screen here
  }
}
```
### 5. Start the UI Task

In your main application entry, instantiate your UI class and call \`start_ui()\`:

```cpp
#include "sampler_ui.h" // Your UI subclass header

int main() {
    esp32_ui::SamplerUI ui;
    ui.start_ui();

    // Continue with other setup or loop code...
    while (true) {
        // Your main loop logic here
    }
    return 0;
}
```

This starts the UI update task which will handle input, screen updates, and event dispatching.

## Notes and Gotchas

Thread Safety: Make sure to handle any shared resources carefully. The UI uses FreeRTOS tasks and synchronization primitives like mutexes.

Display Configuration: You must define DISPLAY_BASE in your project before including the display headers.

Hardware Pins: Check your pin mappings (pin_map.h) carefully to match your hardware setup.

Performance: The UI task runs frequently; keep your UI update code efficient to avoid CPU hogging.

### Troubleshooting

If the display doesn’t initialize, verify your wiring and the DISPLAY_BASE definition.

Ensure SPI mutex is created and passed correctly if you get SPI bus errors.

If UI updates don’t appear, confirm the task is running and not blocked.
