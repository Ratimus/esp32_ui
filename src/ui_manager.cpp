#include <U8g2lib.h>
#include <esp32_ui/field.h>
#include <esp32_ui/widget.h>
#include <esp32_ui/ui_manager.h>
#include <esp32_ui/event_router.h>
#include <esp32_ui/toggle_element.h>

TaskHandle_t ui_task_handle = nullptr;
TaskHandle_t display_task_handle = nullptr;
UIState *MenuBase::ui_state = nullptr;

UIManager::UIManager()
{
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

void UIManager::ui_task(void *param)
{
  UIManager *ui = static_cast<UIManager *>(param);

  const TickType_t xFrequency{1};
  TickType_t xLastWakeTime{xTaskGetTickCount()};
  uint8_t freq{33};
  uint8_t count{0};
  while (1)
  {
    ui->update();
    if (!count)
    {
      if (!ui->root_node->is_schleep())
      {
        ui->draw();
      }
      else
      {
        ui->screen_saver();
      }
    }

    ++count;
    if (count == freq)
    {
      count = 0;
    }
    xTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void UIManager::start_ui()
{
  auto ptr = Display::instance();
  assert(ptr);
  ptr->start_display();
  vTaskDelay(pdMS_TO_TICKS(100));

  xTaskCreate(
      &UIManager::ui_task,
      "ui task",
      1024 * 4,
      this,
      configMAX_PRIORITIES - 2,
      &ui_task_handle);
}
