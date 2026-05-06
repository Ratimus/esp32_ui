#pragma once

#include <memory.h>
#include <freertos/FreeRTOS.h>
#include <esp32_ui/canvas.h>

// This class takes nav and sw inputs and navigates through menu stuff

namespace esp32_ui
{
  class UIManager
  {
  protected:

    static void ui_task(void *param);
    static void display_task(void * param);

    virtual void screen_saver() {}

    UIState *ui_state = nullptr;
    std::unique_ptr<Canvas> root_node;

  public:
    Canvas *root_node_ptr = nullptr;

    virtual bool update() = 0;
    static void schedule_redraw();



    // Put custom setup stuff here to be called once at the start of the ui_task
    // on an instance of your derived class
    virtual void ui_task_begin_hook() {}
    void start_ui();

    UIManager(std::unique_ptr<Canvas> root);
    static void dispatch_event(MenuEvent ev);
    static void request_sync();
    static void request_redraw();

    virtual ~UIManager() = default;
  };

} // namespace esp32_ui
