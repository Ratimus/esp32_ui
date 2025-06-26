#pragma once

#include <stdint.h>
constexpr uint8_t MAIN_ENCODER_INDEX = 0;

#include <string>
#include <Arduino.h>

#ifdef DEBUGGING_MENU

#define menuprint Serial.print
#define menuprintf Serial.printf
#define menuprintln Serial.println
#else
#define menuprint(...)
#define menuprintf(...)
#define menuprintln(...)
#endif

struct MenuEvent
{
  enum Source
  {
    NoSource = 0,
    PushButton = 1 << 0,
    Encoder = 1 << 1,
    Toggle = 1 << 2,
    Gate = 1 << 3,
    System = 1 << 4
  } source = System;

  enum Type
  {
    NoType,
    NavUp,
    NavDown,
    NavLeft,
    NavRight,
    Select,
    Back,
    ButtonHeld,
    ButtonReleased,
    Draw,
    Sync,
    AnyAndAll
  } type = NoType;

  uint8_t index = 0;

  MenuEvent(Source src = System,
            Type t = NoType,
            uint8_t idx = 0)
      : source(src),
        type(t),
        index(idx)
  {
  }

  bool operator==(const MenuEvent &other) const
  {
    return (
        (source == other.source) && ((type == other.type) || (type == AnyAndAll)) && (index == other.index));
  }

  bool operator<(const MenuEvent &other) const
  {
    return std::tie(source, type, index) < std::tie(other.source, other.type, other.index);
  }
};

class UIState
{
  UIState()
  {
  }

public:
  volatile int8_t SELECTED_CHANNEL = -1;
  const uint8_t MAIN_ENCODER = MAIN_ENCODER_INDEX;

  static UIState *instance()
  {
    static UIState __instance;
    return &__instance;
  }
};

inline const char *event_source_to_str(MenuEvent::Source src)
{
  switch (src)
  {
  case MenuEvent::Source::PushButton:
    return "PushButton";
  case MenuEvent::Source::Encoder:
    return "Encoder";
  case MenuEvent::Source::Toggle:
    return "Toggle";
  case MenuEvent::Source::Gate:
    return "Gate";
  case MenuEvent::Source::System:
    return "System";
  default:
    return "UnknownSource";
  }
}

inline const char *event_type_to_str(MenuEvent::Type t)
{
  switch (t)
  {
  case MenuEvent::Type::NavRight:
    return "Right";
  case MenuEvent::Type::NavLeft:
    return "Left";
  case MenuEvent::Type::NavUp:
    return "Up";
  case MenuEvent::Type::NavDown:
    return "Down";
  case MenuEvent::Type::Select:
    return "Select";
  case MenuEvent::Type::Back:
    return "Back";
  case MenuEvent::Type::ButtonHeld:
    return "ButtonHeld";
  case MenuEvent::Type::ButtonReleased:
    return "ButtonReleased";
  case MenuEvent::Type::Draw:
    return "Draw";
  default:
    return "UnknownEvent";
  }
}

inline void print_nav_event(const MenuEvent &ev)
{
  menuprintf("evt{source=%s, type=%s, index=%u}\n",
             event_source_to_str(ev.source),
             event_type_to_str(ev.type),
             ev.index);
}
