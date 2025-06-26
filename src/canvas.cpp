#include "canvas.h"
#include "event_router.h"
#include "field.h"

BaseType Canvas::base_type() const
{
  return BaseType::Canvas;
}

Widget *Canvas::add_element(std::unique_ptr<Element> element)
{
  auto widget = std::make_unique<Widget>(element->label);
  auto *raw_ptr = widget.get();

  widget.get()->add_element(std::move(element));
  widgets.push_back(std::move(widget));

  return raw_ptr;
}

Widget *Canvas::add_submenu(std::unique_ptr<Canvas> canvas)
{
  auto widget = std::make_unique<Widget>(canvas->label);
  auto *raw_ptr = widget.get();

  widget.get()->add_submenu(std::move(canvas));
  widgets.push_back(std::move(widget));

  return raw_ptr;
}

Widget *Canvas::add_widget(std::unique_ptr<Widget> widget)
{
  auto *raw_ptr = widget.get();

  widgets.push_back(std::move(widget));

  return raw_ptr;
}

// If selectable: menu item responds to <enter> commands (toggle or rotate values, send event, etc.)
// If not: menu item can be entered or edit-enabled
bool Canvas::handle_nav_select(const MenuEvent &ev)
{
  menuprintln("canvas - handle_nav_select");
  auto *widget = active_widget();
  if (widget && widget->can_handle(ev))
  {
    menuprintln("let - widget do it");
    return widget->handle_event(ev);
    // return widget->handle_nav_select(ev);
  }
  menuprintln("canvas - GREEEEED");

  return MenuBase::handle_nav_select(ev);
}

bool Canvas::handle_nav_back(const MenuEvent &ev)
{
  auto *widget = active_widget();
  if (widget && widget->can_handle(ev))
  {
    return widget->handle_event(ev);
    // return widget->handle_nav_back(ev);
  }

  if (popup != nullptr)
  {
    EventRouter::instance()->overwrite_top(popup);
    return true;
  }
  return EventRouter::instance()->pop_menu();
}

void Canvas::handle_enter()
{
  menuprintf("%s Canvas::handle_enter\n", label);
  for (auto &widget : widgets)
  {
    if (widget.get() == active_widget())
    {
      widget->handle_get_focus();
    }
    else
    {
      widget->handle_sync();
    }
  }

  // MenuBase::handle_enter();
}

void Canvas::handle_exit()
{
  menuprintf("%s Canvas::handle_exit\n", label);
  for (auto &w : widgets)
  {
    w->handle_lose_focus();
  }

  MenuBase::handle_exit();
}

bool Canvas::handle_nav_delta(const MenuEvent &ev)
{
  // TODO: filter nav sources. For now, just limit to primary
  menuprintln("canvas - handle_nav_delta");
  auto *widget = active_widget();
  if (widget && widget->can_handle(ev))
  {
    print_event(ev);
    menuprintf(" --> forward to %s\n", widget->label);
    return widget->handle_event(ev);
  }

  if (ev.index == MAIN_ENCODER_INDEX)
  {
    menuprintf("%s move cursor from %s to ", label, active_widget()->label);
    active_widget()->handle_lose_focus();
    move_cursor(ev);
    menuprint(active_widget()->label);
    menuprintf(" ...hover_to_edit=%s\n", active_widget()->hover_to_edit ? "TRUE" : "FALSE");
    active_widget()->handle_get_focus();
    return true;
  }

  return MenuBase::handle_nav_delta(ev);
}

void Canvas::move_cursor(const MenuEvent &ev)
{
  int8_t prev_index = selected_index();
  int8_t next_index = prev_index;
  int8_t inc = 1;
  if ((ev.type == MenuEvent::NavLeft) || (ev.type == MenuEvent::NavUp))
  {
    next_index -= inc;
  }
  else if ((ev.type == MenuEvent::NavRight) || (ev.type == MenuEvent::NavDown))
  {
    next_index += inc;
  }

  if (next_index < 0)
  {
    if (is_wrappable())
    {
      next_index += widgets.size();
    }
    else
    {
      next_index = 0;
    }
  }
  else if (next_index >= widgets.size())
  {
    if (is_wrappable())
    {
      next_index -= widgets.size();
    }
    else
    {
      next_index = widgets.size() - 1;
    }
  }

  cursor = next_index;
}

bool Canvas::handle_event(const MenuEvent &ev)
{
  // Route bound accessory controls directly to active widget
  Widget *widget = active_widget();
  // menuprintf("Widget type: %s\n", widget->widget_type());
  if (widget && this->event_filter(ev))
  {
    return widget->handle_event(ev);
  }

  return MenuBase::handle_event(ev);
}

Widget *Canvas::active_widget()
{
  if (selected_index() < widgets.size())
  {
    return widgets[selected_index()].get();
  }
  return nullptr;
}

Widget *Canvas::c_current_widget() const
{
  if (selected_index() < widgets.size())
  {
    return widgets[selected_index()].get();
  }
  return nullptr;
}

void Canvas::handle_sync()
{
  menuprintf("%s Canvas::handle_sync\n", label);
  auto *w = active_widget();

  // Only blur and re-focus if we're not in edit mode
  const bool should_refocus = w && !w->is_editing_mode();

  if (should_refocus)
  {
    w->handle_lose_focus();
  }

  for (auto &widget : widgets)
  {
    widget->handle_sync();
  }

  if (should_refocus && w == active_widget())
  {
    w->handle_get_focus();
  }
}

void Canvas::handle_draw(Display *d) const
{
  if (header)
  {
    header->handle_draw(d);
  }

  size_t num_widgets = widgets.size();
  if (fixed_cursor)
  {
    for (size_t n = 0; n < num_widgets; ++n)
    {
      d->setCursor(0, n * 8 + 8);
      uint8_t active_index = (selected_index() + n) % num_widgets;
      auto &child = widgets[active_index];
      if (child)
      {
        child.get()->handle_draw(d);
      }
    }
  }
  else
  {
    // TODO: bring these changes back into library, and handle this more goodly
    for (size_t n = 0; n < widgets.size(); ++n)
    {
      d->setCursor(0, n * 12 + 12);
      auto &child = widgets[n];
      if (child)
      {
        child.get()->handle_draw(d);
      }
    }
  }
}
