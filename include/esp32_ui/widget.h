#pragma once

#include <memory>
#include "menu_base.h"
#include "element.h"

// UI logic unit. Handles child selection, focus, editing, routing,
// and state control.
// Responsibilities
//     Manages routing logic for a row or group of UI components.
//     Applies hover_to_edit, live_update, etc.
//     Pushes new Canvas on <select> if it's a link.
//     Updates focus/edit state of children.
//     Holds child Elements, Fields, or other Widgets.
class Widget : public Element
{
public:
  // Config
  bool hover_to_edit = false;
  bool live_update = true;
  bool cancel_on_back = false;

protected:
  std::vector<std::unique_ptr<Element>> elements; // Fields or basic Elements
  std::unique_ptr<Element> linked_canvas = nullptr;
  uint8_t cursor_offset = 50;

public:
  bool is_active = false;
  bool is_editing = false;

  Element *c_selected_element() const;
  Element *active_element();

  void add_submenu(std::unique_ptr<Element> submenu);
  void add_element(std::unique_ptr<Element> element);

  Widget(const char *label = "")
      : Element(label)
  {
  }

  virtual ~Widget() = default;

  virtual const char *widget_type() const { return "Widget"; }
  virtual void set_live_update(bool enable);
  virtual BaseType base_type() const override { return BaseType::Widget; }
  virtual void print_value(Display *d) const override
  {
    if (!c_selected_element())
      return;
    c_selected_element()->print_value(d);
  }

  void set_cursor_offset(uint8_t ofs)
  {
    cursor_offset = ofs;
  }

  ///////////////////////////////////////////////////////////////////
  // Event Handlers
  ///////////////////////////////////////////////////////////////////
  virtual bool handle_event(const MenuEvent &ev) override;
  virtual bool handle_nav_delta(const MenuEvent &ev) override;
  virtual bool handle_nav_select(const MenuEvent &ev) override;
  virtual bool handle_nav_back(const MenuEvent &ev) override;
  virtual void highlight_if_active(Display *d) const;
  virtual void handle_draw(Display *d) const override;

  // Tells the parent Canvas whether the Widget can consume the event (in which
  // case the event will be routed to it).
  // In your Field classes, override this function to pick off specific events
  // for the desired behavior, then call the base version to handle the rest
  virtual bool can_handle(const MenuEvent &ev) const override;

  virtual void handle_sync() override;
  ///////////////////////////////////////////////////////////////////
  // State Transitions
  ///////////////////////////////////////////////////////////////////
  virtual void focus_element();
  virtual void blur_element();
  virtual void start_editing();
  virtual void stop_editing();
  virtual void toggle_editing();
  virtual void handle_get_focus() override;
  virtual void handle_lose_focus() override;

  ///////////////////////////////////////////////////////////////////
  // Editing
  ///////////////////////////////////////////////////////////////////
  virtual void commit_edit();
  virtual void cancel_edit();
  bool is_editing_mode() const { return is_editing; }
};
