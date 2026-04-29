#include <U8g2lib.h>
#include <esp32_ui/field.h>
#include <esp32_ui/widget.h>
#include <esp32_ui/ui_manager.h>
#include <esp32_ui/event_router.h>
#include <esp32_ui/toggle_element.h>

namespace esp32_ui
{
  TaskHandle_t ui_task_handle = nullptr;
  UIState *MenuBase::ui_state = nullptr;

  UIManager::UIManager(std::unique_ptr<Canvas> root)
  {
    ui_state = UIState::instance();
    MenuBase::ui_state = ui_state;
    root_node = std::move(root);
    root_node_ptr = root_node.get();
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
    auto *ptr = Display::instance();
    if (ptr)
    {
      ptr->start_display();
    }

    UIManager *ui = static_cast<UIManager *>(param);
    ui->ui_task_begin_hook();
    EventRouter::instance()->push_menu(ui->root_node_ptr);

    const TickType_t xTaskFrequency{1};
    TickType_t xLastWakeTime{xTaskGetTickCount()};
    uint8_t display_refresh_rate{33};
    uint8_t display_refresh_timer{0};

    while (1)
    {
      ui->update();
      if (!display_refresh_timer)
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

      ++display_refresh_timer;
      if (display_refresh_timer == display_refresh_rate)
      {
        display_refresh_timer = 0;
      }
      xTaskDelayUntil(&xLastWakeTime, xTaskFrequency);
    }
  }

  void UIManager::start_ui()
  {
    xTaskCreate(
        &UIManager::ui_task,
        "ui task",
        1024 * 4,
        this,
        configMAX_PRIORITIES - 2,
        &ui_task_handle);
  }

} // namespace esp32_ui
