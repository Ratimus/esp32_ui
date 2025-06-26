#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <type_traits>
#include <vector>
#include <memory>
#include "display.h"
#include "menu_base.h"
#include "element.h"
#include "menu_event.h"
#include "widget.h"

class Canvas : public Element
{
protected:
  int8_t cursor = 0;
  bool fixed_cursor = false;

  std::vector<std::unique_ptr<Widget>> widgets;
  virtual int8_t selected_index() const { return cursor; }

  std::unique_ptr<Header> u_hdr = nullptr;
  std::unique_ptr<Header> u_ftr = nullptr;
  std::unique_ptr<Header> u_pup = nullptr;

  Header *header = nullptr;
  Footer *footer = nullptr;
  Canvas *popup = nullptr;

public:
  Canvas(const char *label)
      : Element(label),
        u_hdr(std::make_unique<Header>(label))
  {
    header = u_hdr.get();
  }

  virtual ~Canvas() = default;

  virtual BaseType base_type() const override;

  Widget *add_element(std::unique_ptr<Element> element);
  Widget *add_submenu(std::unique_ptr<Canvas> canvas);
  Widget *add_widget(std::unique_ptr<Widget> widget);

  Widget *c_current_widget() const;
  Widget *active_widget();
  void move_cursor(const MenuEvent &ev);
  void set_fixed_cursor(bool on_off = true)
  {
    fixed_cursor = on_off;
  }
  /////////////////////////////////////////////////////////////////////////////
  // NAVIGATION FUNCTIONS
  // Propagate navigation commands from root node down to active node
  /////////////////////////////////////////////////////////////////////////////
  virtual bool can_handle(const MenuEvent &ev) const override { return false; }

  /////////////////////////////////////////////////////////////////////////////
  // MODEL/VIEW FUNCTIONS
  // Update model, Draw view
  /////////////////////////////////////////////////////////////////////////////
  virtual void handle_sync() override;
  virtual void handle_draw(Display *d) const override;

  virtual bool handle_event(const MenuEvent &ev) override;
  virtual bool handle_nav_delta(const MenuEvent &ev) override;
  virtual bool handle_nav_select(const MenuEvent &ev) override;
  virtual bool handle_nav_back(const MenuEvent &ev) override;

  virtual void handle_enter() override;
  virtual void handle_exit() override;
};

class Root : public Canvas
{
public:
  Root(const char *label)
      : Canvas(label)
  {
  }

  virtual void handle_draw(Display *d) const override
  {
    if (!schleeping)
    {
      Canvas::handle_draw(d);
    }
  }
};
