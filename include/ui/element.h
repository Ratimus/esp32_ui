#pragma once

#include <memory>
#include <set>

#include "menu_base.h"

class Element : public MenuBase
{
protected:
  std::set<MenuEvent> registered_events;
  std::function<void()> func;

public:
  enum class FieldDataType
  {
    None,
    UInt8,
    Int8,
    UInt16,
    Int16,
    UInt32,
    Int32,
    Int64,
    Array_UInt8,
    Array_Int8,
    Array_UInt16,
    Array_Int16
  };

  Element(const char *label)
      : MenuBase(label)
  {
  }

  virtual ~Element() = default;
  virtual BaseType base_type() const override { return BaseType::Element; }
  virtual FieldDataType field_data_type() const { return FieldDataType::None; }

  std::vector<std::unique_ptr<Element>> elements; // Fields or basic Elements
  std::unique_ptr<Element> linked_canvas = nullptr;

  void bind_control_popup(MenuEvent::Source src, uint8_t idx);
  void unbind_control_popup(MenuEvent::Source src, uint8_t idx);
  void register_event_listener(const MenuEvent &ev);
  void unregister_event_listener(const MenuEvent &ev);
  virtual void register_handler(std::function<void()> func);
  bool event_filter(const MenuEvent &ev) const;
};

class Header : public Element
{
protected:
  uint8_t x_offset = 0;
  uint8_t y_offset = 8;
  uint8_t width = 132;

public:
  Header(const char *label)
      : Element(label)
  {
  }

  virtual ~Header() = default;

  virtual void handle_draw(Display *d) const override
  {
    d->setCursor(16, 0);
    print_label(d);
    d->drawHLine(x_offset, 8, width);
  }
};

class Footer : public Element
{
public:
  virtual void handle_draw(Display *d) const override
  {
    d->drawFrame(0, 56, 32, 8);
    d->drawFrame(31, 56, 32, 8);
    d->drawFrame(63, 56, 32, 8);
    d->drawFrame(95, 56, 32, 8);
  }
  virtual ~Footer() = default;
};
