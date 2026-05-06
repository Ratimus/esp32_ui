#pragma once

#include <esp32_ui/field.h>

namespace esp32_ui
{
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
      auto tmp = state.in() + (T)delta;
      state.set_input(tmp);
    }

    virtual T value() const
    {
      return state.in();
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
        if (val != state.out()) // Only update if changed, avoiding unnecessary redraws/focus loss
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
      setter_cb(state.out());
    }

    // Apply the edit to the model
    virtual void commit() override
    {
      menuprintf("%s: commit\n", this->label);
      T delta = state.in() - state.out();
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
          this->setter_cb(state.out());
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
}
