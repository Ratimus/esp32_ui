#pragma once

#include <memory.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp32_ui/canvas.h>

extern TaskHandle_t ui_task_handle;

// This class takes nav and sw inputs and navigates through menu stuff

class UIManager
{
protected:
  static void ui_task(void *param);
  virtual void screen_saver() {}
  void dispatch_event(MenuEvent ev);

  UIState *ui_state = nullptr;

public:
  std::unique_ptr<Canvas> root_node;
  virtual void update() = 0;
  virtual void draw();
  virtual void start_ui();

  UIManager();
  ~UIManager() = default;
};
