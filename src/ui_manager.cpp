#include <U8g2lib.h>
#include <freertos/task.h>

#include <esp32_ui/field.h>
#include <esp32_ui/heartbeat.h>
#include <esp32_ui/widget.h>
#include <esp32_ui/ui_manager.h>
#include <esp32_ui/event_router.h>
#include <esp32_ui/toggle_element.h>

namespace esp32_ui
{
  TaskHandle_t ui_task_handle = nullptr;
  UIState *MenuBase::ui_state = nullptr;

  TaskHandle_t evt_dispatch_task_handle = nullptr;
  QueueHandle_t evt_queue = nullptr;

  UIManager::UIManager(std::unique_ptr<Canvas> root)
  {
    ui_state = UIState::instance();
    MenuBase::ui_state = ui_state;
    root_node = std::move(root);
    root_node_ptr = root_node.get();
    EventRouter::instance()->push_menu(root_node_ptr);
  }

  void UIManager::dispatch_event(MenuEvent ev)
  {
    if (evt_queue == nullptr)
    {
      return;
    }

    if (xQueueSend(evt_queue, &ev, 0) != pdPASS)
    {
      // increment drop counter
    }
  }

  void evt_dispatch_task(void * param)
  {
    esp32_ui::TaskHeartbeat *hb = esp32_ui::register_task("evt dispatch");
    assert(hb && "whoops, max tasks registered");

    if (evt_queue == nullptr)
    {
      // Hey, look! If you generate a bunch of events and it crashes everything, you'll want to
      // either embiggen this or generate fewer events
      evt_queue = xQueueCreate(16, sizeof(MenuEvent));
    }

    MenuEvent ev;

    //dbprintln("evt_dispatch_task started");
    Serial.println("evt dispatch started");

    while(true)
    {
      if (pdTRUE ==
        xQueueReceive(evt_queue, (void *)&ev, portMAX_DELAY))
      {
        hb_start(hb);
        EventRouter::instance()->dispatch(ev);
        hb_end(hb);
      }
      else
      {
        taskYIELD();
      }
    }
  }

  void UIManager::draw()
  {
    dispatch_event(MenuEvent{MenuEvent::Source::System, MenuEvent::Type::Draw, 0});
  }

  void UIManager::ui_task(void *param)
  {
    TaskHeartbeat * hb = register_task("ui task");
    assert(hb && "whoops, max tasks registered");

    auto *ptr = Display::instance();
    if (ptr)
    {
      ptr->start_display();
    }

    UIManager *ui = static_cast<UIManager *>(param);
    ui->ui_task_begin_hook();
    EventRouter::instance()->push_menu(ui->root_node_ptr);

    const TickType_t xTaskFrequency{100};
    TickType_t xLastWakeTime{xTaskGetTickCount()};
    uint8_t display_refresh_rate{10};
    uint8_t display_refresh_timer{0};

    Serial.println("ui_task started");
    while (1)
    {
      // // Use the handle to obtain further information about the task.
      // vTaskGetInfo(
      //               // The handle of the task being queried.
      //               xHandle,
      //               // The TaskStatus_t structure to complete with information on xTask.
      //               &xTaskDetails,
      //               // Include the stack high water mark value in the TaskStatus_t structure.
      //               pdTRUE,
      //               // Include the task state in the TaskStatus_t structure.
      //               eInvalid );


      // // Serial.println(xTaskDetails.pcTaskName);
      // pcTaskGetName(xHandle);

      hb_start(hb);

      // if (!display_refresh_timer)
      // {
        if (!ui->root_node->is_schleep())
        {
          ui->draw();
        }
        else
        {
          ui->screen_saver();
        }
      // }
      // else
      // {
        ui->update();
      // }

      // ++display_refresh_timer;
      // if (display_refresh_timer == display_refresh_rate)
      // {
      //   display_refresh_timer = 0;
      // }

      hb_end(hb);
      xTaskDelayUntil(&xLastWakeTime, xTaskFrequency);
    }
  }

  void UIManager::start_ui()
  {
    start_heartbeat();
    vTaskDelay(1000);

    xTaskCreate(
        &UIManager::ui_task,
        "ui task",
        1024 * 4,
        this,
        10,
        &ui_task_handle);
    vTaskDelay(1000);

    xTaskCreate(
      &evt_dispatch_task,
      "evt dispatch",
      1024 * 5,
      (void *)nullptr,
      15,
      &evt_dispatch_task_handle);
    vTaskDelay(1000);
  }

} // namespace esp32_ui
