#pragma once

#include <memory>
#include <vector>
#include <utility>
#include <string>
#include <algorithm>
#include <set>

#include <SequencerTools/Latchable.h>

#include <esp32_ui/menu_base.h>
#include <esp32_ui/menu_event.h>
#include <esp32_ui/display.h>
#include <esp32_ui/element.h>
#include <esp32_ui/widget.h>
#include <esp32_ui/event_router.h>

namespace esp32_ui
{
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

  // For SockPuppet
  //  Absolute: commit(val) calls setter_cb(val), which directly consumes the value
  //  Delta: commit(val) calls setter_cb(val - last_val) for functions that increment or decrement an index in an array of values
  enum class EditMode
  {
    Absolute,
    Delta
  };

  template <typename T>
  class SockPuppet : public FieldBase
  {
  protected:
    std::function<T()> getter_cb;
    std::function<void(T)> setter_cb;
    latchable<T> state;

  public:
    EditMode mode;

    const char *delimiter;

    SockPuppet(const char *label,
               EditMode mode = EditMode::Absolute,
               const char *delimiter = ": ")
        : FieldBase(label),
          mode(mode)
    {
      this->wrappable = false;
    }

    virtual ~SockPuppet() = default;

    virtual void apply_delta(int8_t delta) override
    {
      menuprintf("%s: SockPuppet apply_delta %d\n", this->label, delta);
      state.in += (T)delta;
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
      if (this->getter_cb)
      {
        T val = this->getter_cb();
        if (val != state.out) // Only update if changed, avoiding unnecessary redraws/focus loss
        {
          menuprintf("sync %s: %d --> %d\n", this->label, state.out, val);
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
      T delta = state.in - state.out;
      state.clock();
      if (this->setter_cb)
      {
        if (mode == EditMode::Delta)
        {
          // setter_cb moves the index in array of possible values
          menuprintf("%s: SockPuppet setter_cb (delta=%d)\n", this->label, delta);
          this->setter_cb(delta);
        }
        else
        {
          // setter_cb sets the value directly
          menuprintf("%s: SockPuppet setter_cb\n", this->label);
          this->setter_cb(state.out);
        }
      }
    }

    virtual void cancel() override
    {
      // I haven't fully decided whether to permit cancels. It probably
      // doesn't make sense if SockPuppet is really just a control surface
      // for external values, but I'm leaving the original logic here in case
      // I realize there's a use case for it and I don't remember how it was done
      //
      assert(false && "SockPuppet does not support cancel()");

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
      menuprintf("%s value sync\n", this->label);
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

} // namespace esp32_ui
