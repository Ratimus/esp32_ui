#pragma once

#include <set>
#include "element.h"

class Bang : public Element
{
public:
  Bang(const char *label,
       const MenuEvent& trigger = {MenuEvent::Source::NoSource, MenuEvent::Type::NoType, 0},
       std::function<void()> func = nullptr)
    : Element(label)
  {
    if (func)
    {
      register_handler(std::move(func));
    }

    if ( (trigger.source != MenuEvent::Source::NoSource)
      && (trigger.type != MenuEvent::Type::NoType) )
    {
      register_event_listener(trigger);
    }
  }

  virtual ~Bang() = default;

  // void register_event_listener(const MenuEvent& ev)
  // {
  //   menuprintf("registered ");
  //   print_event(ev);
  //   registered_events.insert(ev);
  // }

  // virtual void register_handler(std::function<void()> func)
  // {
  //   this->func = std::move(func);
  // }

  // bool event_filter(const MenuEvent& ev) const
  // {
  //   bool match = (registered_events.find(ev) != registered_events.end());
  //   return match;
  // }

  virtual bool handle_event(const MenuEvent& ev) override
  {
    if (event_filter(ev))
    {
      // menuprint("event_filter match: ");
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
