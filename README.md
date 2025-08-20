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

## Fundamental Classes

This UI framework is built around several core classes that manage data representation, user interaction, and display rendering. Below is an explanation of the fundamental building blocks you’ll work with when creating UI elements.

### FieldBase and Field Templates (`field.h`)

- **`FieldBase`**  
  The abstract base class for all editable fields. It provides essential interfaces like label management, value printing, and navigation handling. Every interactive UI element that holds a value derives from this.

- **`ValueField<T>`**  
  A generic, templated field that holds and edits values of type `T`. It supports minimum, maximum, and step constraints, and handles navigation input for modifying the value intuitively.

- **`SockPuppet<T>`**  
  A specialized field that binds its value to external state through user-provided getter and setter callbacks. This allows seamless synchronization between the UI and the underlying application data.

### Bang (`bang.h`)

- **`Bang`**  
  A simple, event-driven element that triggers a callback function when a specific event occurs (e.g., a button press). It serves as an actionable UI component, ideal for commands or triggers without persistent state.

### Canvas and Root (`canvas.h`)

- **`Canvas`**  
  The primary container for UI widgets, managing layout, navigation, and drawing. It holds a collection of `Widget`s and facilitates user interaction by managing cursor position, focus, and event propagation.

- **`Root`**  
  A specialized subclass of `Canvas` that typically acts as the top-level UI container. It manages overall drawing and interaction for the entire UI tree.

### MenuEvent (`menu_event.h`)

- **`MenuEvent`**  
  Defines the input event structure used throughout the UI system. Events have a `Source` (e.g., encoder, button) and a `Type` (e.g., navigation up/down, select, back). This unified event model drives user navigation and interaction.

- **`UIState`**  
  Holds global UI state, such as the index of the main encoder used for navigation, allowing customization and central management of input devices.

### ToggleElement (`toggle_event.h`)

- **`ToggleElement`**  
  A toggleable UI element derived from `Bang` that switches between two states (true/false) when triggered. It shows a dynamic label reflecting its current state and executes an optional callback on changes.

### Widget (`widget.h`)

- **`Widget`**  
  The fundamental container that groups UI elements (`Element`s) and manages focus, editing states, and event routing within that group. It supports behaviors like hover-to-edit, live update, and cancel-on-back, serving as the core building block for interactive UI components.

### WidgetPair (`widget_pair.h`)

- **`WidgetPair`**  
  A compound widget that organizes two child widgets side-by-side (left and right), managing their focus, navigation, and editing states independently but cohesively. This is useful for paired controls or related fields displayed together.

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

In your main application entry, instantiate your UI class and call `start_ui()`:

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

### Defining Your Own Canvas

In your project, you need to define your own menu canvases, widgets, etc. You'd put any headers you define for specific instances in your own project's `/include` folder. Here's an example of defining your own canvas:

```cpp
#pragma once

#include <esp32_ui/toggle_element.h>
#include <esp32_ui/canvas.h>
#include <esp32_ui/field.h>

#include "sampler_voice.h"

static constexpr const char *channel_labels[] = {
    "Channel A", "Channel B", "Channel C", "Channel D"};

namespace esp32_ui
{
  class ChannelSettings : public Canvas
  {
    std::unique_ptr<Header> uhdr;
    uint8_t channel;

  public:
    ChannelSettings(uint8_t channel)
        : Canvas(channel_labels[channel]),
          channel(channel)
    {
      uhdr = std::make_unique<Header>(channel_labels[channel]);
      header = uhdr.get();
      build_canvas();
    }

    virtual ~ChannelSettings() = default;

    void build_canvas()
    {
      auto sample = std::make_unique<ValueField<int16_t>>("Sample", voice[channel].sample.clock(), 0, NUM_SAMPLES - 1, 1);
      sample->register_getter(std::move([this]()
                                        { return voice[channel].sample.in; }));
      sample->register_setter(std::move([this](int16_t idx)
                                        { voice[channel].sample.clock_in(idx); }));
      this->add_element(std::move(sample));

      auto level = std::make_unique<ValueField<int16_t>>("Level", voice[channel].mix.clock(), 0, 200, 1);
      level->register_getter(std::move([this]()
                                       { return voice[channel].mix.in; }));
      level->register_setter(std::move([this](int16_t level)
                                       { voice[channel].mix.clock_in(level); }));
      level->set_big_step(10);
      this->add_element(std::move(level));

      auto decay = std::make_unique<ValueField<int16_t>>("Decay", voice[channel].decay.clock(), 1, 1000, 1);
      decay->register_getter(std::move([this]()
                                       { return voice[channel].decay.in; }));
      decay->register_setter(std::move([this](int16_t decay)
                                       { voice[channel].decay.clock_in(decay); }));
      decay->set_big_step(100);
      this->add_element(std::move(decay));

      auto pitch = std::make_unique<ValueField<int16_t>>("Pitch", voice[channel].pitch.clock(), -96, 96, 1);
      pitch->register_getter(std::move([this]()
                                       { return voice[channel].pitch.in; }));
      pitch->register_setter(std::move([this](int16_t pitch)
                                       { voice[channel].pitch.clock_in(pitch); }));
      pitch->set_big_step(12);
      this->add_element(std::move(pitch));

      if (!(channel % 2))
      {
        auto choke = std::make_unique<ToggleElement>((channel == 0) ? "Choke Ch. B" : "Choke Ch. D", "YES", "NO", voice[channel].choke.in);
        choke->register_event_listener(MenuEvent{MenuEvent::Source::Encoder, MenuEvent::Type::Select, MAIN_ENCODER_INDEX});

        // This is an on_change() callback; the value toggles and THEN the callback fires
        choke->register_handler([this]
                                { voice[channel].choke.clock_in(!voice[channel].choke.in); });
        this->add_element(std::move(choke));
      }
    }
  };
} // namespace esp32_ui
```

## Notes and Gotchas

Thread Safety: Make sure to handle any shared resources carefully. The UI uses FreeRTOS tasks and synchronization primitives like mutexes.

Display Configuration: You must define DISPLAY_BASE in your project before including the display headers.

Hardware Pins: Check your pin mappings (pin_map.h) carefully to match your hardware setup.

Performance: The UI task runs frequently; keep your UI update code efficient to avoid CPU hogging.

### Troubleshooting

If the display doesn’t initialize, verify your wiring and the DISPLAY_BASE definition.

Ensure SPI mutex is created and passed correctly if you get SPI bus errors.

If UI updates don’t appear, confirm the task is running and not blocked.
