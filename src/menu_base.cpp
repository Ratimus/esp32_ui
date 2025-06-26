#include "menu_base.h"


bool MenuBase::is_primary_nav_event(const MenuEvent& ev)
{
  if ( (ev.source == MenuEvent::Source::Encoder)
    && (ev.index == UIState::instance()->MAIN_ENCODER))
  {
    return true;
  }

  return false;
}


bool MenuBase::is_primary_select(const MenuEvent& ev)
{
  return (is_primary_nav_event(ev) && (ev.type == MenuEvent::Type::Select));
}


bool MenuBase::handle_event(const MenuEvent& ev)
{
  // menuprintf("%s\n", label);

  if (ev.type == MenuEvent::Type::Sync)
  {
    handle_sync();
    return true;
  }

  // if (is_primary_nav_event(ev))
  // {
    switch (ev.type)
    {
      case MenuEvent::Back:
        return handle_nav_back(ev);

      case MenuEvent::NavDown:
      case MenuEvent::NavUp:
      case MenuEvent::NavLeft:
      case MenuEvent::NavRight:
        return handle_nav_delta(ev);

      case MenuEvent::Select:
        return handle_nav_select(ev);

      default:
        break;
    }
  // }

  return true;
}
