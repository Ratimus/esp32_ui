#include "widget.h"
#include "canvas.h"
#include "event_router.h"

Element *Widget::active_element()
{
  if (elements.empty())
  {
    return nullptr;
  }
  return elements[0].get();
}

Element *Widget::c_selected_element() const
{
  if (elements.empty())
  {
    return nullptr;
  }
  return elements[0].get();
}

void Widget::add_submenu(std::unique_ptr<Element> submenu)
{
  assert(submenu->base_type() == BaseType::Canvas);
  linked_canvas = std::move(submenu);
}

void Widget::add_element(std::unique_ptr<Element> element)
{
  elements.emplace_back(std::move(element));
}

bool Widget::handle_event(const MenuEvent &ev)
{
  menuprintf("%s: Widget::handle_event\n", label);

  // Check if individual element filters out specific event
  auto el = active_element();
  if (el)
  {
    // If this isn't a primary nav event, somebody decided we wanted to see it
    // if we got here, so try to do something with it anyway
    if (el->can_handle(ev) || !is_primary_nav_event(ev))
    {
      return el->handle_event(ev);
    }
  }

  return MenuBase::handle_event(ev);
}

bool Widget::handle_nav_delta(const MenuEvent &ev)
{
  menuprintf("\nWidget::%s nav delta", label);
  auto *el = active_element();

  if (!el)
  {
    menuprintln(" ...nullptr element!");
  }
  if (!is_editing)
  {
    menuprintln(" ...not editing!");
  }
  // Only allow editing if we're in hover or explicit edit mode
  if (is_editing && el)
  {
    auto *field = el;
    if ((ev.type == MenuEvent::Type::NavLeft) || (ev.type == MenuEvent::Type::NavUp) || (ev.type == MenuEvent::Type::NavRight) || (ev.type == MenuEvent::Type::NavDown))
    {
      field->handle_event(ev);
    }

    if (live_update)
    {
      field->commit();
    }
    return true;
  }

  return MenuBase::handle_nav_delta(ev);
}

bool Widget::handle_nav_select(const MenuEvent &ev)
{
  // Activate submenu
  menuprintf("%s handle_nav_select\n", label);
  if (linked_canvas)
  {
    return EventRouter::instance()->push_menu(linked_canvas.get());
  }

  if (!hover_to_edit)
  {
    toggle_editing();
    return true;
  }

  if (!live_update)
  {
    // Save edits
    commit_edit();
    return true;
  }
  // if (linked_canvas) return push_linked_canvas();
  // if (!hover_to_edit) return toggle_edit_mode();
  // if (!live_update) return commit_edit();
  return MenuBase::handle_nav_select(ev);
}

bool Widget::handle_nav_back(const MenuEvent &ev)
{
  menuprintf("%s handle_nav_back\n", label);
  if (!is_editing)
  {
    return EventRouter::instance()->pop_menu();
  }

  stop_editing();
  if (hover_to_edit)
  {
    return EventRouter::instance()->pop_menu();
  }

  return MenuBase::handle_nav_back(ev);
}

void Widget::highlight_if_active(Display *d) const
{
  d->setCursor(0, d->getCursorY());
  if (is_active)
  {
    d->print(">");
  }
  else
  {
    d->print(" ");
  }
}

void Widget::handle_draw(Display *d) const
{
  highlight_if_active(d);
  if (linked_canvas)
  {
    d->print(linked_canvas->label);
  }
  else
  {
    auto *el = c_selected_element();
    if (el)
    {
      el->print_label(d);
      uint8_t ofs = d->getCursorX();
      if (ofs < cursor_offset)
      {
        ofs = cursor_offset;
      }
      d->setCursor(ofs, d->getCursorY());
      d->print(":");

      if (is_editing)
      {
        d->print("[");
      }
      else
      {
        d->print(" ");
      }

      el->print_value(d);

      if (is_editing)
      {
        // d->setCursor(d->getWidth() - d->char_width(), d->getCursorY());
        d->print("]");
      }
    }
  }
}

bool Widget::can_handle(const MenuEvent &ev) const
{
  menuprintf("Widget::can_handle?");
  print_event(ev);
  // Check if individual element filters out specific event
  auto el = c_selected_element();
  if (el)
  {
    if (el->can_handle(ev))
    {
      menuprintln("yep");
      return true;
    }
  }

  // Direct (popup) routing
  if (!is_primary_nav_event(ev))
  {
    menuprintln("not primary, so yeah");
    return true;
  }

  // Should Field consume primary encoder?
  switch (ev.type)
  {
  // All Fields should always consume <select>.
  // This doesn't need to be true, but somebody is going to need to consume the event,
  // so it makes sense for MenuNodes to always route <select> to a Field, letting the
  // Field decide whether to ignore it.
  case MenuEvent::Type::Select:
  {
    menuprintln("select - yeah");
    return true;
  }
  case MenuEvent::Type::Back:
  {
    // Menu consumes to navigate back
    if (hover_to_edit)
    {
      menuprintln("hover to edit - no");
      return false;
    }

    // Explicit editing; Field consumes to stop
    if (is_editing)
    {
      menuprintln("editing - yeah");
      return true;
    }

    // Menu should consume to unfocus Field and navigate back
    menuprintln("guess not");
    return false;
  }
  case MenuEvent::Type::NavLeft:
  case MenuEvent::Type::NavRight:
  case MenuEvent::Type::NavUp:
  case MenuEvent::Type::NavDown:
  {
    // Main encoder should be consumed by menu to change focus
    if (hover_to_edit)
    {
      menuprintln("hover to edit - no");
      return false;
    }

    // Menu consumes to change focus
    if (!is_editing)
    {
      menuprintln("not editing - nah");
      return false;
    }

    // Route to Field if editing
    menuprintln("editing - yeah");
    return true;
  }
  default:
    break;
  }
  menuprintln("somehow, no");
  return false;
}

void Widget::focus_element()
{
  if (!active_element())
  {
    return;
  }
  // start_editing calls this, but you need to call it separately if not hover_to_edit
  active_element()->focus();
}

void Widget::blur_element()
{
  if (!active_element())
  {
    return;
  }
  // stop_editing calls this, but you need to call it separately if not editing
  active_element()->blur();
}

void Widget::start_editing()
{
  menuprintf("%s start editing\n", label);
  auto *el = active_element();
  if (el && el->field_data_type() != Element::FieldDataType::None)
  {
    focus_element();
  }
  is_editing = true;
}

void Widget::stop_editing()
{
  menuprintf("%s stop editing\n", label);
  auto *el = active_element();
  if (el && el->field_data_type() != Element::FieldDataType::None)
  {
    // End edit (commit or cancel)
    if (cancel_on_back)
    {
      el->cancel();
    }
    else
    {
      el->commit();
    }
    blur_element();
  }
  is_editing = false;
}

void Widget::toggle_editing()
{
  if (is_editing)
  {
    stop_editing();
  }
  else
  {
    start_editing();
  }
}

void Widget::handle_get_focus()
{
  menuprintf("%s Widget::handle_get_focus\n", label);
  is_active = true;
  if (hover_to_edit)
  {
    // Begin edit (Field)
    start_editing();
  }
  else
  {
    // enable_edit() will call this automatically, but if we're not editing, we still want
    // to make sure we're handling focus/blur states
    // menuprintf("%s focus element\n", label);
    focus_element();
  }

  // This calls handle_sync()
  MenuBase::handle_get_focus();
}

void Widget::handle_lose_focus()
{
  is_active = false;
  stop_editing();

  // MenuBase::handle_lose_focus();
}

void Widget::commit_edit()
{
  auto *el = active_element();
  if (el) //  && el->field_data_type() != Element::FieldDataType::None
  {
    el->commit();
  }
}

void Widget::cancel_edit()
{
  auto *el = active_element();
  if (el) //  && el->field_data_type() != Element::FieldDataType::None)
  {
    el->cancel();
  }
}

void Widget::set_live_update(bool enable)
{
  if (enable)
  {
    // Doesn't make sense to allow cancel_on_back if all edits are immediately committed
    live_update = true;
    cancel_on_back = false;
  }
  else
  {
    live_update = false;
  }
}

void Widget::handle_sync()
{
  for (auto &e : elements)
  {
    e->handle_sync();
  }
}
