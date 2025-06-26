#pragma once

#include "bang.h"

// Register ONE event to this thing. It will toggle its value when it calls
// handle_event() for it. If you want to use a multidirectional controller,
// this is not the class for that.
class ToggleElement : public Bang
{
protected:
  const char *true_label;
  const char *false_label;
  const char *delimiter;
  bool show_label = false;
  bool      value = false;

public:

  ToggleElement(const char *label,
              const char *true_label,
              const char *false_label,
              bool init = false,
              const MenuEvent& trigger = {
                MenuEvent::Source::NoSource,
                MenuEvent::Type::NoType, 0
              },
              std::function<void()> func = nullptr,
              const char *delimiter = ": ")
    : Bang(label, trigger, func),
      true_label(true_label),
      false_label(false_label),
      delimiter(delimiter),
      value(init)
  {}

  virtual ~ToggleElement() = default;

  // This is an on_change() callback; the value toggles and THEN the callback fires
  virtual bool handle_event(const MenuEvent& ev) override
  {
    if (event_filter(ev))
    {
      value = !value;
      // menuprint("event_filter match: ");
      // print_event(ev);
      if (func)
      {
        func();
      }
      return true;
    }

    return false;
  }

  virtual void print_value(Display *d) const override
  {
    d->print(value ? true_label : false_label);
  }

  virtual void handle_draw(Display *d) const override
  {
    print_label(d);
    d->print(delimiter);
    print_value(d);
  }

  void set_value(bool val)
  {
    value = val;
  }

  bool get_value()
  {
    return value;
  }
};
