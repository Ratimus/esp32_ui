#include <U8g2lib.h>
#include <freertos/task.h>

#include <esp32_ui/field.h>
#include <esp32_utils/heartbeat.h>
#include <esp32_ui/widget.h>
#include <esp32_ui/ui_manager.h>
#include <esp32_ui/event_router.h>
#include <esp32_ui/toggle_element.h>

namespace esp32_ui
{
  static bool dirty_screen;

  TaskHandle_t ui_task_handle = nullptr;
  UIState *MenuBase::ui_state = nullptr;

  TaskHandle_t evt_dispatch_task_handle = nullptr;
  QueueHandle_t evt_queue = nullptr;

  bool sync_pending = false;
  bool redraw_pending = false;

  UIManager::UIManager(std::unique_ptr<Canvas> root)
  {
    ui_state = UIState::instance();
    MenuBase::ui_state = ui_state;
    root_node = std::move(root);
    root_node_ptr = root_node.get();
    EventRouter::instance()->push_menu(root_node_ptr);
    sync_pending = true;
  }

  void UIManager::request_sync()
  {
    // Tell all the active elements that they need to sync their data
    sync_pending |= true;
  }

  void UIManager::request_redraw()
  {
    redraw_pending |= true;
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
    auto router = EventRouter::instance();

    //dbprintln("evt_dispatch_task started");
    Serial.println("evt dispatch started");

    while(true)
    {
      if (pdTRUE ==
        xQueueReceive(evt_queue, (void *)&ev, 1))
      {
        hb_start(hb);
        hb_label(hb, "queued evt");
        router->dispatch(ev);
        hb_end(hb);
      }
      else if (sync_pending)
      {
        hb_start(hb);
        hb_label(hb, "sync");
        router->dispatch({MenuEvent::Source::System, MenuEvent::Type::Sync, 0});
        hb_end(hb);
        sync_pending = false;
      }
      else
      {
        taskYIELD();
      }

    }
  }

  void UIManager::schedule_redraw()
  {
    dirty_screen = true;
    // dispatch_event(MenuEvent{MenuEvent::Source::System, MenuEvent::Type::Draw, 0});
  }

  void UIManager::display_task(void * param)
  {
    UIManager *ui = static_cast<UIManager *>(param);

    TaskHeartbeat * hb = register_task("display task");
    assert(hb && "whoops, max tasks registered");

    const TickType_t xTaskFrequency = pdMS_TO_TICKS(33);
    TickType_t xLastWakeTime{xTaskGetTickCount()};

    schedule_redraw();

    auto *d = Display::instance();
    if (d)
    {
      d->start_display();
    }

    vTaskDelay(100);

    while(1)
    {
      hb_start(hb);
      if (!ui->root_node->is_schleep())
      {
        schedule_redraw();
      }
      else
      {
        ui->screen_saver();
        schedule_redraw();
      }

      if (dirty_screen)
      {
        d->clearBuffer();
        EventRouter::instance()->top_menu()->handle_draw(d);
        d->sendBuffer();

        dirty_screen = false;
      }
      hb_end(hb);

      xTaskDelayUntil(&xLastWakeTime, xTaskFrequency);
    }
  }

  void UIManager::ui_task(void *param)
  {
    TaskHeartbeat * hb = register_task("ui task");
    assert(hb && "whoops, max tasks registered");

    UIManager *ui = static_cast<UIManager *>(param);
    ui->ui_task_begin_hook();
    EventRouter::instance()->push_menu(ui->root_node_ptr);

    const TickType_t xTaskFrequency = pdMS_TO_TICKS(10);
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
      dirty_screen |= ui->update();

      // if (!display_refresh_timer)
      // {

      hb_end(hb);
      xTaskDelayUntil(&xLastWakeTime, xTaskFrequency);
    }
  }

  void UIManager::start_ui()
  {
    dirty_screen = true;
    start_heartbeat();
    vTaskDelay(1000);

    xTaskCreate(
        &UIManager::ui_task,
        "ui task",
        1024 * 4,
        this,
        18,
        &ui_task_handle);
    vTaskDelay(1000);

    xTaskCreate(
      &evt_dispatch_task,
      "evt dispatch",
      1024 * 5,
      (void *)nullptr,
      10,
      &evt_dispatch_task_handle);
    vTaskDelay(1000);

    xTaskCreate(
      &display_task,
      "display_task",
      1024 * 5,
      this,
      1,
      &evt_dispatch_task_handle);
  }

} // namespace esp32_ui
