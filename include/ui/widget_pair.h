#pragma once

#include "widget.h"


class WidgetPair : public Widget
{
  inline static constexpr uint8_t LEFT_INDEX = 0;
  inline static constexpr uint8_t RIGHT_INDEX = 1;

public:
  WidgetPair(const char *label,
             std::unique_ptr<Element>&& left,
             std::unique_ptr<Element>&& right)
    : Widget(label)
  {
    auto w_left = std::make_unique<Widget>(left->label);
    w_left->add_element(std::move(left));

    auto w_right = std::make_unique<Widget>(right->label);
    w_right->add_element(std::move(right));

    elements.emplace_back(std::move(w_left));
    elements.emplace_back(std::move(w_right));

    set_hover_to_edit(true);
    wrappable = false;
  }
  const char* widget_type() const override { return "WidgetPair"; }
  virtual ~WidgetPair() = default;

  BaseType base_type() const override;
  virtual void  set_live_update (bool enable) override;
  void          set_hover_to_edit(bool enable);
  void          set_cancel_on_back(bool enable);

  Widget *c_left() const;
  Widget *c_right() const;
  Widget *left();
  Widget *right();

  void handle_draw(Display *d) const override;
  virtual bool can_handle(const MenuEvent& ev) const override;
  virtual bool handle_event(const MenuEvent& ev) override;

  virtual bool handle_nav_delta(const MenuEvent& ev) override;
  // virtual bool handle_nav_select(const MenuEvent& ev) override;
  // virtual bool handle_nav_back(const MenuEvent& ev) override;

  ///////////////////////////////////////////////////////////////////
  // State Transitions
  ///////////////////////////////////////////////////////////////////
  virtual void handle_lose_focus() override;
  virtual void handle_get_focus() override;
  virtual void focus_element() override;
  virtual void blur_element() override;
  virtual void handle_sync() override;

protected:

  ///////////////////////////////////////////////////////////////////
  // Editing
  ///////////////////////////////////////////////////////////////////
  virtual void start_editing() override;
  virtual void stop_editing() override;
  virtual void commit_edit() override;
  virtual void cancel_edit() override;
};
