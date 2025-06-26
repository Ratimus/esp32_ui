#pragma once

#include <memory>
#include <vector>
#include <utility>
#include <string>
#include <algorithm>
#include <set>

#include <Latchable.h>
#include <RatFuncs.h>

#include "menu_base.h"
#include "menu_event.h"
#include "display.h"
#include "element.h"
#include "widget.h"
#include "event_router.h"

class FieldBase : public Element
{
protected:
  const char *delimiter;

public:
  FieldBase(const char *label,
            const char *delimiter = ": ")
      : Element(label),
        delimiter(delimiter)
  {
  }

  virtual ~FieldBase() = default;

  virtual BaseType base_type() const override final { return BaseType::Field; }
  virtual FieldDataType field_data_type() const override = 0;

  virtual void commit() = 0; // Confirm edit
  virtual void cancel() = 0; // Revert edit

  virtual void handle_draw(Display *d) const override
  {
    print_label(d);
    print_delimiter(d);
    print_value(d);
  }

  virtual void print_delimiter(Display *d) const { d->print(delimiter); }
  virtual void print_value(Display *d) const override = 0;
  virtual bool handle_nav_delta(const MenuEvent &ev)
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
  virtual void apply_delta(int8_t delta) = 0;
};

template <typename T>
class SockPuppet : public FieldBase
{
protected:
  std::function<T()> getter_cb;
  std::function<void(T)> setter_cb;
  latchable<T> state;

public:
  const char *delimiter;

  SockPuppet(const char *label,
             const char *delimiter = ": ")
      : FieldBase(label)
  {
    this->wrappable = false;
  }

  virtual ~SockPuppet() = default;

  virtual void apply_delta(int8_t delta) override
  {
    menuprintf("%s: SockPuppet apply_delta %d\n", this->label, this->delta);
    state.in += delta;
  }

  virtual T value() const
  {
    return state.in;
  }

  virtual void print_value(Display *d) const override
  {
    d->print(value());
  }

  virtual void handle_sync() override
  {
    menuprintf("ch %d ", ui_state->SELECTED_CHANNEL);
    if (this->getter_cb)
    {
      T val = this->getter_cb();
      if (val != state.out) // Only update if changed, avoiding unnecessary redraws/focus loss
      {
        menuprintf("sync %s: %d --> %d\n", this->label, state.out, this->val);
        state.clock_in(val);
      }
    }
    menuprintln("==============");
  }

  virtual void register_getter(std::function<T()> cb)
  {
    assert(cb);
    this->getter_cb = std::move(cb);
    handle_sync();
  }

  virtual void register_setter(std::function<void(T)> cb)
  {
    assert(cb);
    setter_cb = std::move(cb);
    setter_cb(state.out);
  }

  // Apply the edit to the model
  virtual void commit() override
  {
    menuprintf("%s: commit\n", this->label);
    state.clock();
    if (this->setter_cb)
    {
      menuprintf("%s: SockPuppet setter_cb\n", this->label);
      this->setter_cb(state.out);
    }
  }

  // Revert local state to original
  virtual void cancel() override
  {
    state.loopback();
    menuprintf("%s: cancel\n", this->label);
  }

  virtual FieldDataType field_data_type() const override { return FieldDataType::None; }
};

template <typename T>
class ValueField : public FieldBase
{
protected:
  std::function<void(const T &from, const T &to)> on_change_cb;
  T big_step = 0;

public:
  T perma_val;
  T temp_val;
  T min;
  T max;
  T step;
  const char *delimiter;

  // Just for derived classes
  ValueField(const char *label,
             T initial,
             T min, T max, T step,
             const char *delimiter = ": ")
      : FieldBase(label),
        perma_val(initial),
        temp_val(initial),
        min(min),
        max(max),
        step(step),
        delimiter(delimiter)
  {
    wrappable = false;
  }

  virtual ~ValueField() = default;
  virtual bool handle_nav_delta(const MenuEvent &ev) override
  {
    menuprintf("%s: ValueField handle_nav_delta\n", label);
    if (ev.type == MenuEvent::Type::NavLeft)
    {
      apply_delta(-step);
      return true;
    }

    if (ev.type == MenuEvent::Type::NavRight)
    {
      apply_delta(step);
      return true;
    }

    if (!big_step)
    {
      return false;
    }

    if (ev.type == MenuEvent::Type::NavUp)
    {
      apply_delta(-big_step);
      return true;
    }

    if (ev.type == MenuEvent::Type::NavDown)
    {
      apply_delta(big_step);
      return true;
    }

    return false;
  }

  virtual void apply_delta(int8_t delta) override
  {
    menuprintf("%s: ValueField apply_delta(%d)\n", this->label, delta);
    if (delta > 0)
    {
      if (temp_val + delta <= max)
      {
        temp_val += delta;
      }
      else if (wrappable && (delta == 1))
      {
        temp_val = min;
      }
      else
      {
        temp_val = max;
      }
    }
    else if (delta < 0)
    {
      if (temp_val + delta >= min)
      {
        temp_val += delta;
      }
      else if (wrappable && (delta == -1))
      {
        temp_val = max;
      }
      else
      {
        temp_val = min;
      }
    }
    else
    {
      return;
    }
    EventRouter::instance()->request_sync();
  }

  virtual T value() const
  {
    return temp_val;
  }

  virtual void print_value(Display *d) const override
  {
    d->print(temp_val);
  }

  std::function<T()> getter_cb;
  std::function<void(T)> setter_cb;

  void set_big_step(T val)
  {
    big_step = val;
  }

  virtual void handle_sync() override
  {
    // menuprintf("ch: %d\n", ui_state->SELECTED_CHANNEL);
    menuprintf("%s value sync\n", this->label);
    // menuprint("perma: ");
    // menuprintln(this->perma_val);
    // menuprint("temp: ");
    // menuprintln(this->temp_val);
    if (this->getter_cb)
    {
      T val = this->getter_cb();
      menuprint("gotten: ");
      this->perma_val = val;
      menuprintln(val);
    }
    this->temp_val = this->perma_val;
    menuprintln("==============");
  }

  virtual void register_getter(std::function<T()> cb)
  {
    assert(cb);
    this->getter_cb = std::move(cb);
    handle_sync();
  }

  virtual void register_setter(std::function<void(T)> cb)
  {
    assert(cb);
    setter_cb = std::move(cb);
    this->setter_cb(this->perma_val);
  }

  // Apply the edit to the model
  virtual void commit() override
  {
    perma_val = temp_val;
    if (setter_cb)
    {
      setter_cb(perma_val);
    }
  }

  // Revert local state to original
  virtual void cancel() override
  {
    temp_val = perma_val;
  }

  virtual FieldDataType field_data_type() const override { return FieldDataType::None; }
};

template <>
inline FieldBase::FieldDataType ValueField<int8_t>::field_data_type() const { return FieldDataType::Int8; }
template <>
inline FieldBase::FieldDataType ValueField<uint8_t>::field_data_type() const { return FieldDataType::UInt8; }
template <>
inline FieldBase::FieldDataType ValueField<int16_t>::field_data_type() const { return FieldDataType::Int16; }
template <>
inline FieldBase::FieldDataType ValueField<uint16_t>::field_data_type() const { return FieldDataType::UInt16; }
