#pragma once

#include <functional>
#include "menu_event.h"
#include "display.h"

enum class BaseType
{
  Canvas,
  WidgetPair,
  Widget,
  Field,
  Element
};

class MenuBase
{
public:
  const char *label;
  static UIState *ui_state;
  bool wrappable = true;
  bool schleeping = false;

  MenuBase(const char *label = "UNHANDLED")
      : label(label)
  {
  }

  virtual ~MenuBase() = default;

  virtual BaseType base_type() const = 0;

  bool is_wrappable() const { return wrappable; }
  virtual bool event_filter(const MenuEvent &ev) const { return false; }

  virtual void handle_sleep()
  {
    schleeping = true;
  }

  virtual void wake_up()
  {
    schleeping = false;
  }

  bool is_schleep()
  {
    return schleeping;
  }
  //////////////////////////////////////////////////////////////////////////////
  // MenuEvent Pre-processing/Filtering
  virtual bool can_handle(const MenuEvent &ev) const
  {
    return event_filter(ev);
  }

  static bool is_primary_nav_event(const MenuEvent &ev);
  static bool is_primary_select(const MenuEvent &ev);

  //////////////////////////////////////////////////////////////////////////////
  // MenuEvent Handlers
  virtual bool handle_event(const MenuEvent &ev);
  virtual bool handle_nav_delta(const MenuEvent &ev)
  {
    if (on_nav_delta_cb)
      on_nav_delta_cb(ev);
    return true;
  }
  virtual bool handle_nav_select(const MenuEvent &ev)
  {
    if (on_select_cb)
      on_select_cb(ev);
    return true;
  }
  virtual bool handle_nav_back(const MenuEvent &ev)
  {
    if (on_back_cb)
      on_back_cb(ev);
    return true;
  }

  //////////////////////////////////////////////////////////////////////////////
  // MenuEvent Callbacks
  std::function<void(const MenuEvent &ev)> on_back_cb;   //{[this](const MenuEvent& ev){ print_event(ev); }};
  std::function<void(const MenuEvent &ev)> on_select_cb; //{[this](const MenuEvent& ev){ print_event(ev); }};
  std::function<void(const MenuEvent &ev)> on_nav_delta_cb{[this](const MenuEvent &ev)
                                                           { print_event(ev); }};

  //////////////////////////////////////////////////////////////////////////////
  // Navigation State Handlers
  virtual void handle_get_focus()
  {
    handle_sync();
    if (on_get_focus_cb)
      on_get_focus_cb();
  }
  virtual void handle_lose_focus()
  {
    if (on_lose_focus_cb)
      on_lose_focus_cb();
  }
  virtual void handle_enter()
  {
    handle_sync();
    if (on_enter_cb)
      on_enter_cb();
  }
  virtual void handle_exit()
  {
    if (on_exit_cb)
      on_exit_cb();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Navigation State Callbacks
  std::function<void()> on_get_focus_cb;  //{[this]{menuprintf("%s: focus\n", label);}};
  std::function<void()> on_lose_focus_cb; //{[this]{menuprintf("%s: blur\n", label);}};
  std::function<void()> on_enter_cb;      //{[this]{menuprintf("%s: enter\n", label);}};
  std::function<void()> on_exit_cb;       //{[this]{menuprintf("%s: exit\n", label);}};

  //////////////////////////////////////////////////////////////////////////////
  // Display Functions
  virtual void print_value(Display *d) const {}
  virtual void print_label(Display *d) const { d->print(label); }
  virtual void handle_draw(Display *d) const {}

  virtual void handle_sync() { menuprintf("%s: sync\n", label); }
  virtual void focus() { handle_sync(); }
  virtual void blur() {} // e.g., unhighlight
  virtual void commit() {}
  virtual void cancel() {}

  virtual void apply_delta(int8_t delta) { menuprintf("%s:MenuBase  apply_delta [%d]\n", label, delta); }

  virtual void print_event(const MenuEvent &ev) const
  {
    menuprintf("%s: ", label);
    print_nav_event(ev);
  }
};
