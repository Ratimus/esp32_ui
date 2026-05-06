#include <esp32_ui/field.h>
#include <esp32_ui/ui_manager.h>

namespace esp32_ui
{

// ======================================================================================
//  --- FieldBase
// ======================================================================================
void FieldBase::handle_draw(Display *d) const
{
  print_label(d);
  print_delimiter(d);
  print_value(d);
}

bool FieldBase::handle_nav_delta(const MenuEvent &ev)
{
  menuprintf("%s: FieldBase handle_nav_delta\n", label);
  if ((ev.type == MenuEvent::Type::NavLeft) || (ev.type == MenuEvent::Type::NavUp))
  {
    apply_delta(-1);
    return true;
  }

  if ((ev.type == MenuEvent::Type::NavRight) || (ev.type == MenuEvent::Type::NavDown))
  {
    apply_delta(+1);
    return true;
  }

  return false;
}

void FieldBase::apply_delta(int8_t delta)
{
  UIManager::request_sync();
}

// ======================================================================================
//
// ======================================================================================

} // namespace esp32_ui
