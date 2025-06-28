#pragma once

#include <set>

#include <esp32_ui/element.h>

class Bang : public Element
{
public:
  Bang(const char *label,
       const MenuEvent &trigger = {MenuEvent::Source::NoSource, MenuEvent::Type::NoType, 0},
       std::function<void()> func = nullptr)
      : Element(label)
  {
    if (func)
    {
      register_handler(std::move(func));
    }

    if ((trigger.source != MenuEvent::Source::NoSource) && (trigger.type != MenuEvent::Type::NoType))
    {
      register_event_listener(trigger);
    }
  }

  virtual ~Bang() = default;

  virtual bool handle_event(const MenuEvent &ev) override
  {
    if (event_filter(ev))
    {
      print_event(ev);
      if (func)
      {
        func();
      }
      return true;
    }

    return MenuBase::handle_event(ev);
  }
};
