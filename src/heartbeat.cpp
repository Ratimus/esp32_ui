#include "esp_system.h"
#include <esp32_ui/heartbeat.h>
#include <SequencerTools/RatFuncs.h>

namespace esp32_ui
{
  // Probably don't need a forward declaration since only the start_task() function is
  // going to call it, but here it is
  void hb_task(void *arg);

  QueueHandle_t logQueue = nullptr;
  const int64_t REPORT_PERIOD_US = 500000;

  uint8_t task_count = 0;
  TaskHandle_t hb_task_handle = nullptr;
  TaskHandle_t log_task_handle = nullptr;

  TaskHeartbeat beaters[MAX_TASKS];

  TaskHeartbeat * register_task(const char * name)
  {
    if (task_count == MAX_TASKS)
    {
      return nullptr;
    }

    beaters[task_count] = TaskHeartbeat(name);
    TaskHeartbeat * ret = &beaters[task_count];
    ++task_count;
    return ret;
  }

  void hb_dump_all(void)
  {
    menuprintln("\n=== HEARTBEAT DUMP ===");
    for (uint8_t i = 0; i < task_count; ++i)
    {
      TaskHeartbeat snapshot;

      portENTER_CRITICAL(&beaters[i].mux);
      snapshot = beaters[i];
      portEXIT_CRITICAL(&beaters[i].mux);

      dbprintf("%-16s | avg=%lld | wcet=%lld | last=%lld | rate=%uHz | seen=%lld\n",
        snapshot.name,
        snapshot.last_reported_avg_us,
        snapshot.last_reported_wcet_us,
        snapshot.last_duration_us,
        snapshot.last_reported_rate,
        snapshot.last_seen_us
      );
    }
  }

  void hb_task(void *arg)
  {
    if (logQueue == nullptr)
    {
      logQueue = xQueueCreate(10, sizeof(HbLogEntry));
    }
    const TickType_t xDelay = pdMS_TO_TICKS(500);
    dbprintln("heartbeat task started");
    HbLogEntry entry;

    while (1)
    {
      int64_t now = esp_timer_get_time();

      // Blank entry - print log delimiter
      entry.name = "";
      if (xQueueSend(logQueue, &entry, 0) != pdPASS)
      {
        dbprintln("hb_task failed to send logQueue!");
      }

      for (uint8_t idx = 0; idx < task_count; ++idx)
      {
        TaskHeartbeat snapshot;

        portENTER_CRITICAL(&beaters[idx].mux);
        snapshot = beaters[idx];
        portEXIT_CRITICAL(&beaters[idx].mux);

        if (snapshot.counter == 0)
        {
          // Never executed since registration
          entry.name    = snapshot.name;
          entry.avg_us  = 0;
          entry.wcet_us = 0;
          entry.last_us = -1;
          entry.rate    = 0;
          entry.stalled = true;
          entry.has_run = false;
          if (xQueueSend(logQueue, &entry, 0) != pdPASS)
          {
            dbprintln("LOG QUEUE FULL");
          }
          continue;
        }

        int64_t avg_us  = snapshot.total_duration_us / snapshot.counter;
        int64_t wcet_us = snapshot.max_duration_us;

        uint32_t delta_count = snapshot.counter - snapshot.last_counter_snapshot;
        int64_t delta_time   = now - snapshot.last_rate_compute_us;

        uint32_t rate = 0;
        if (delta_time > 0)
        {
          rate = (delta_count * 1000000) / delta_time;
        }

        bool stalled = (now - snapshot.last_seen_us) > TIMEOUT_uS;

        portENTER_CRITICAL(&beaters[idx].mux);
        beaters[idx].last_reported_avg_us  = avg_us;
        beaters[idx].last_reported_wcet_us = wcet_us;
        beaters[idx].last_reported_rate    = rate;
        beaters[idx].last_reported_stalled = stalled;

        beaters[idx].last_counter_snapshot = snapshot.counter;
        beaters[idx].last_rate_compute_us = now;
        portEXIT_CRITICAL(&beaters[idx].mux);

        entry.name    = snapshot.name;
        entry.avg_us  = avg_us;
        entry.wcet_us = wcet_us;
        entry.last_us = snapshot.last_duration_us;
        entry.rate    = rate;
        entry.stalled = stalled;
        entry.has_run = true;

        if (xQueueSend(logQueue, &entry, 0) != pdPASS)
        {
          dbprintln("LOG QUEUE FULL");
        }
      }

      vTaskDelay(xDelay);
    }
  }

  void logit(void * param)
  {
    HbLogEntry entry;

    while(true)
    {
      if (xQueueReceive(logQueue, &entry, portMAX_DELAY))
      {
        if (strcmp(entry.name, "") == 0)
        {
          dbprintln("===== HEARTBEAT =====");
          continue;
        }

        if (!entry.has_run)
        {
          dbprintf("%-16s | NEVER RAN\n", entry.name);
          continue;
        }

        dbprintf("%-16s | avg=%6lldus | wcet=%6lldus | last=%6lldus | rate=%5uHz %s\n",
          entry.name,
          entry.avg_us,
          entry.wcet_us,
          entry.last_us,
          entry.rate,
          entry.stalled ? "STALL" : ""
        );

        if (entry.stalled)
        {
          menuprintf("STALL DETECTED: %s\n", entry.name);
        }
      }
    }
  }

  void start_heartbeat()
  {
    esp_register_shutdown_handler(hb_dump_all);

    xTaskCreate(
        &hb_task,
        "hearbeat monitor task",
        1024 * 3,
        &beaters,
        5,
        &hb_task_handle);

    xTaskCreate(
        &logit,
        "hearbeat logger task",
        1024 * 3,
        (void*)nullptr,
        1,
        &log_task_handle);
  }
} // namespace esp32_ui
