#include "widget_pair.h"
#include "event_router.h"

#ifndef LEFT_ENCODER_INDEX
#define LEFT_ENCODER_INDEX 0
#endif

#ifndef RIGHT_ENCODER_INDEX
#define RIGHT_ENCODER_INDEX 0
#endif
// To ensure WidgetPair and Widget work well together in a uniform way:
//     Ensure WidgetPair::handle_nav_delta() and others defer to children only if is_editing == true, like Widget does.
//     If WidgetPair ever becomes more complex (e.g., supporting more than two elements), generalize the left/right to indexed access or a vector.
BaseType WidgetPair::base_type() const
{
  return BaseType::WidgetPair;
}

Widget *WidgetPair::c_left() const
{
  return static_cast<Widget *>(elements[LEFT_INDEX].get());
}

Widget *WidgetPair::c_right() const
{
  return static_cast<Widget *>(elements[RIGHT_INDEX].get());
}

Widget *WidgetPair::left()
{
  return static_cast<Widget *>(elements[LEFT_INDEX].get());
}

Widget *WidgetPair::right()
{
  return static_cast<Widget *>(elements[RIGHT_INDEX].get());
}

void WidgetPair::handle_draw(Display *d) const
{
  const uint8_t y = d->getCursorY();
  auto margin = d->char_width();
  d->setCursor(0, y);
  if (is_active)
  {
    d->print("[");
  }
  else
  {
    d->print(" ");
  }
  d->setCursor(margin, y);
  c_left()->print_label(d);
  d->print(": ");
  c_left()->print_value(d);
  d->setCursor(d->half_width() + margin, y);
  c_right()->print_label(d);
  d->print(": ");
  c_right()->print_value(d);
  if (is_active)
  {
    d->setCursor(d->getWidth() - d->char_width(), y);
    d->print("]");
  }
}

bool WidgetPair::handle_nav_delta(const MenuEvent &ev)
{
  // if (active_index == LEFT_INDEX)
  // {
  //   if (left()->handle_nav_delta(ev))
  //   {
  //     EventRouter::instance()->request_sync();
  //     return true;
  //   }
  // }
  // else if (active_index == RIGHT_INDEX)
  // {
  //   if (right()->handle_nav_delta(ev))
  //   {
  //     EventRouter::instance()->request_sync();
  //     return true;
  //   }
  // }
  if (ev.source == MenuEvent::Source::Encoder)
  {
    if (ev.index == LEFT_ENCODER_INDEX)
    {
      left()->handle_event(ev);
      // EventRouter::instance()->request_sync();
      return true;
    }

    if (ev.index == RIGHT_ENCODER_INDEX)
    {
      right()->handle_event(ev);
      // EventRouter::instance()->request_sync();
      return true;
    }
  }
  return MenuBase::handle_nav_delta(ev);
}

bool WidgetPair::can_handle(const MenuEvent &ev) const
{
  if (is_primary_nav_event(ev))
  {
    if (ev.type != MenuEvent::Type::Select)
    {
      return false;
    }
  }

  if (event_filter(ev))
  {
    return true;
  }

  if (c_left()->can_handle(ev) || c_right()->can_handle(ev))
  {
    return true;
  }

  return Widget::can_handle(ev);
}

bool WidgetPair::handle_event(const MenuEvent &ev)
{
  // menuprintf(" WidgetPair::handle_event %s\n", label);

  // Check if individual element filters out specific event
  auto el = active_element();

  if (!el)
  {
    // menuprintln("WidgetPair::  ...nullptr element!");
  }
  if (!is_editing)
  {
    // menuprintln("WidgetPair::  ...not editing!");
  }

  // if (ev.source == MenuEvent::Source::Encoder)
  // {
  //   if (ev.index == LEFT_ENCODER_INDEX)
  //   {
  //     left()->handle_event(ev);
  //     EventRouter::instance()->request_sync();
  //     return true;
  //   }

  //   if (ev.index == RIGHT_ENCODER_INDEX)
  //   {
  //     right()->handle_event(ev);
  //     EventRouter::instance()->request_sync();
  //     return true;
  //   }
  // }

  return MenuBase::handle_event(ev);
}

void WidgetPair::start_editing()
{
  menuprintf("WidgetPair:: %s start editing\n", label);
  left()->start_editing();
  right()->start_editing();
  is_editing = true;
}

void WidgetPair::stop_editing()
{
  menuprintf("WidgetPair:: %s stop editing\n", label);
  left()->stop_editing();
  right()->stop_editing();
  is_editing = false;
}

void WidgetPair::handle_get_focus()
{
  menuprintf("WidgetPair:: %s handle_get_focus\n", label);
  is_active = true;
  left()->handle_get_focus();
  right()->handle_get_focus();
  if (hover_to_edit)
  {
    start_editing();
  }
}

void WidgetPair::handle_lose_focus()
{
  menuprintf("WidgetPair:: %s handle_lose_focus\n", label);
  left()->handle_lose_focus();
  right()->handle_lose_focus();

  is_active = false;
  Widget::handle_lose_focus();
}

void WidgetPair::commit_edit()
{
  left()->commit_edit();
  right()->commit_edit();
}

void WidgetPair::cancel_edit()
{
  left()->cancel_edit();
  right()->cancel_edit();
}

void WidgetPair::set_live_update(bool enable)
{
  live_update = enable;
  left()->set_live_update(enable);
  right()->set_live_update(enable);
  if (live_update)
  {
    // Can't cancel an edit if you immediately commit every edit
    set_cancel_on_back(false);
  }
}

void WidgetPair::set_hover_to_edit(bool enable)
{
  hover_to_edit = enable;
  left()->hover_to_edit = hover_to_edit;
  right()->hover_to_edit = hover_to_edit;
  if (hover_to_edit)
  {
    // Immediately write changes
    set_live_update(true);
  }
}

void WidgetPair::set_cancel_on_back(bool enable)
{
  cancel_on_back = enable;
  left()->cancel_on_back = cancel_on_back;
  right()->cancel_on_back = cancel_on_back;
  if (cancel_on_back)
  {
    // Don't immediately save changes if you might want to cancel them
    set_live_update(false);
  }
}

void WidgetPair::focus_element()
{
  left()->focus_element();
  right()->focus_element();
}

void WidgetPair::blur_element()
{
  left()->blur_element();
  right()->blur_element();
}

void WidgetPair::handle_sync()
{
  left()->handle_sync();
  right()->handle_sync();
}
