#include <U8g2lib.h>
#include "field.h"
#include "widget.h"
#include "ui_manager.h"
#include "event_router.h"
#include "toggle_element.h"

TaskHandle_t ui_task_handle = nullptr;
TaskHandle_t display_task_handle = nullptr;
UIState *MenuBase::ui_state = nullptr;

UIManager::UIManager()
{
  ui_mutex = xSemaphoreCreateMutex();
  this->ui_state = UIState::instance();
  MenuBase::ui_state = this->ui_state;
}

void UIManager::dispatch_event(MenuEvent ev)
{
  EventRouter::instance()->dispatch(ev);
}

void UIManager::draw()
{
  EventRouter::instance()->dispatch(MenuEvent{MenuEvent::Source::System, MenuEvent::Type::Draw, 0});
}

void UIManager::display_task(void *param)
{
  UIManager *ui = static_cast<UIManager *>(param);
  TickType_t xLastWakeTime{xTaskGetTickCount()};
  const TickType_t xFrequency{33};
  while (1)
  {
    xSemaphoreTake(ui->ui_mutex, portMAX_DELAY);
    if (!ui->root_node->is_schleep())
    {
      ui->draw();
    }
    xSemaphoreGive(ui->ui_mutex);
    xTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void UIManager::ui_task(void *param)
{
  auto ptr = Display::instance();
  assert(ptr);
  ptr->start_display();

  UIManager *ui = static_cast<UIManager *>(param);

  const TickType_t xFrequency{1};
  TickType_t xLastWakeTime{xTaskGetTickCount()};
  while (1)
  {
    ui->update();

    BaseType_t xWasDelayed = xTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void UIManager::start_ui()
{
  xTaskCreate(
      &UIManager::ui_task,
      "ui task",
      1024 * 4,
      this,
      1,
      &ui_task_handle);

  xTaskCreate(
      &UIManager::display_task,
      "display task",
      1024 * 2,
      this,
      1,
      &display_task_handle);
}
