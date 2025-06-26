#include "element.h"
#include "event_router.h"


void Element::bind_control_popup(MenuEvent::Source src, uint8_t idx)
{
  if (is_primary_nav_event(MenuEvent{src}))
  {
    assert(false && "you really wanna bind the main controller?");
  }
  // auto* ir = EventRouter::instance();
  // ir->bind_popup(src, idx, this);
  register_event_listener(MenuEvent{src, MenuEvent::Type::AnyAndAll, idx});
}

void Element::unbind_control_popup(MenuEvent::Source src, uint8_t idx)
{
  // auto* ir = EventRouter::instance();
  // ir->unbind_popup(src, idx);
  unregister_event_listener(MenuEvent{src, MenuEvent::Type::AnyAndAll, idx});
}

void Element::register_event_listener(const MenuEvent& ev)
{
  // menuprintf("registered ");
  print_event(ev);
  registered_events.insert(ev);
}

void Element::unregister_event_listener(const MenuEvent& ev)
{
  // menuprintf("UNregistered ");
  print_event(ev);
  registered_events.erase(ev);
}

void Element::register_handler(std::function<void()> func)
{
  this->func = std::move(func);
}

bool Element::event_filter(const MenuEvent& ev) const
{
  bool match = (registered_events.find(ev) != registered_events.end());
  return match;
}
