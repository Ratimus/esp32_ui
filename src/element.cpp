#include <esp32_ui/element.h>
#include <esp32_ui/event_router.h>

namespace esp32_ui
{
  void Element::bind_control_popup(MenuEvent::Source src, uint8_t idx)
  {
    if (is_primary_nav_event(MenuEvent{src}))
    {
      assert(false && "you really wanna bind the main controller?");
    }
    register_event_listener(MenuEvent{src, MenuEvent::Type::AnyAndAll, idx});
  }

  void Element::unbind_control_popup(MenuEvent::Source src, uint8_t idx)
  {
    unregister_event_listener(MenuEvent{src, MenuEvent::Type::AnyAndAll, idx});
  }

  void Element::register_event_listener(const MenuEvent &ev)
  {
    print_event(ev);
    registered_events.insert(ev);
  }

  void Element::unregister_event_listener(const MenuEvent &ev)
  {
    print_event(ev);
    registered_events.erase(ev);
  }

  void Element::register_handler(std::function<void()> func)
  {
    this->func = std::move(func);
  }

  bool Element::event_filter(const MenuEvent &ev) const
  {
    bool match = (registered_events.find(ev) != registered_events.end());
    return match;
  }

} // namespace esp32_ui
