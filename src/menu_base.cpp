#include <esp32_ui/menu_base.h>

bool MenuBase::is_primary_nav_event(const MenuEvent &ev)
{
  if ((ev.source == MenuEvent::Source::Encoder) && (ev.index == UIState::instance()->MAIN_ENCODER))
  {
    return true;
  }

  return false;
}

bool MenuBase::is_primary_select(const MenuEvent &ev)
{
  return (is_primary_nav_event(ev) && (ev.type == MenuEvent::Type::Select));
}

bool MenuBase::handle_event(const MenuEvent &ev)
{
  if (ev.type == MenuEvent::Type::Sync)
  {
    handle_sync();
    return true;
  }

  switch (ev.type)
  {
  case MenuEvent::Type::Back:
    return handle_nav_back(ev);

  case MenuEvent::Type::NavDown:
  case MenuEvent::Type::NavUp:
  case MenuEvent::Type::NavLeft:
  case MenuEvent::Type::NavRight:
    return handle_nav_delta(ev);

  case MenuEvent::Type::Select:
    return handle_nav_select(ev);

  default:
    break;
  }

  return true;
}
