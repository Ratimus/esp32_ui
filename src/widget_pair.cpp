#include <esp32_ui/widget_pair.h>
#include <esp32_ui/event_router.h>

#ifndef LEFT_ENCODER_INDEX
#define LEFT_ENCODER_INDEX 0
#endif

#ifndef RIGHT_ENCODER_INDEX
#define RIGHT_ENCODER_INDEX 1
#endif

namespace esp32_ui
{
  BaseType WidgetPair::base_type() const
  {
    return BaseType::WidgetPair;
  }

  Widget *WidgetPair::left()
  {
    return static_cast<Widget *>(elements[LEFT_INDEX].get());
  }

  Widget *WidgetPair::right()
  {
    return static_cast<Widget *>(elements[RIGHT_INDEX].get());
  }

  Widget *WidgetPair::c_left() const
  {
    return static_cast<Widget *>(elements[LEFT_INDEX].get());
  }

  Widget *WidgetPair::c_right() const
  {
    return static_cast<Widget *>(elements[RIGHT_INDEX].get());
  }

  bool WidgetPair::can_handle(const MenuEvent &ev) const
  {
    // Let children opt-in first
    if (c_left()->can_handle(ev) || c_right()->can_handle(ev))
    {
      return true;
    }

    return Widget::can_handle(ev);
  }

  bool WidgetPair::handle_event(const MenuEvent &ev)
  {
    if (ev.source == MenuEvent::Source::Encoder)
    {
      if (ev.index == LEFT_ENCODER_INDEX)
      {
        left()->handle_event(ev);
        if (live_update)
        {
          menuprintf("WidgetPair:Left commit\n");
          left()->commit_edit();
        }
        else
        {
          menuprintf("WidgetPair:Left NO commit\n");
        }

        return true;
      }

      if (ev.index == RIGHT_ENCODER_INDEX)
      {
        right()->handle_event(ev);
        if (live_update)
        {
          menuprintf("WidgetPair:Right commit\n");
          right()->commit_edit();
        }
        else
        {
          menuprintf("WidgetPair:Right NO commit\n");
        }

        return true;
      }
    }

    return MenuBase::handle_event(ev);
  }

  bool WidgetPair::handle_nav_delta(const MenuEvent &ev)
  {
    menuprintf("\nWidget::%s nav delta", label);
    if (!is_editing)
    {
      menuprintln(" ...not editing!");
    }

    // Only allow editing if we're in hover or explicit edit mode
    if (is_editing)
    {
      if (ev.index == LEFT_ENCODER_INDEX)
      {
        left()->handle_event(ev);
        if (live_update)
        {
          left()->commit();
        }
      }
      else if (ev.index == RIGHT_ENCODER_INDEX)
      {
        right()->handle_event(ev);
        if (live_update)
        {
          right()->commit();
        }
      }

      return true;
    }

    return MenuBase::handle_nav_delta(ev);
  }

  void WidgetPair::handle_draw(Display *d) const
  {
    const uint8_t y = d->getCursorY();
    auto margin = d->char_width();

    d->setCursor(0, y);
    d->print(is_active ? "[" : " ");

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

    if (hover_to_edit)
    {
      start_editing();
    }
    else
    {
      left()->focus_element();
      right()->focus_element();
    }

    MenuBase::handle_get_focus();
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

    left()->hover_to_edit = enable;
    right()->hover_to_edit = enable;

    if (hover_to_edit)
    {
      set_live_update(true);
    }
  }

  void WidgetPair::set_cancel_on_back(bool enable)
  {
    cancel_on_back = enable;

    left()->cancel_on_back = enable;
    right()->cancel_on_back = enable;

    if (cancel_on_back)
    {
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

} // namespace esp32_ui
