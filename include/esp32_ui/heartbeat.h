#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp32_ui/menu_base.h>

namespace esp32_ui
{

  // TODO: replace with per-task timeouts?
  const int64_t TIMEOUT_uS = 300 * 1000;

  /*  USAGE:
    TaskHeartbeat * hb = register_task(task name);
    assert(hb && "whoops, max tasks registered");

    while (1)
    {
      hb_start(hb);

      // do your task stuff here

      hb_end(hb);
    }
  */

  struct TaskHeartbeat
  {
    const char *name;
    volatile uint32_t counter;
    volatile int64_t  last_start_us;
    volatile int64_t  last_duration_us;
    volatile int64_t  total_duration_us;
    volatile int64_t  max_duration_us;
    volatile int64_t  min_duration_us;
    volatile int64_t  last_seen_us;

    volatile uint32_t last_counter_snapshot;
    volatile int64_t  last_rate_compute_us;
    volatile uint32_t loops_per_sec;

    volatile int64_t  last_reported_avg_us;
    volatile int64_t  last_reported_wcet_us;
    volatile uint32_t last_reported_rate;
    volatile bool     last_reported_stalled;

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    TaskHeartbeat()
    {}
    TaskHeartbeat(const char *name)
      : name(name),
        counter(0),
        last_start_us(0),
        last_duration_us(0),
        total_duration_us(0),
        max_duration_us(0),
        min_duration_us(INT64_MAX),
        last_seen_us(0),
        last_counter_snapshot(0),
        last_rate_compute_us(0),
        loops_per_sec(0)
      {  }
  };

  struct HbLogEntry
  {
    const char *name;
    int64_t avg_us;
    int64_t wcet_us;
    int64_t last_us;
    uint32_t rate;
    bool stalled;
    bool has_run;
  };

  // Call this immediately before the body of your task loop (but after any blocking calls)
  inline void hb_start(TaskHeartbeat *hb)
  {
    hb->last_start_us = esp_timer_get_time();
  }

  // Call this at the very end of the body of your task loop (but before any blocking delays)
  inline void hb_end(TaskHeartbeat *hb)
  {
    if (hb->last_start_us == 0)
    {
      return;
    }

    int64_t now = esp_timer_get_time();
    hb->last_seen_us = now;

    int64_t dur = now - hb->last_start_us;
    hb->last_duration_us = dur;
    hb->total_duration_us += dur;

    if (dur > hb->max_duration_us)
    {
      hb->max_duration_us = dur;
    }

    if (dur < hb->min_duration_us || hb->min_duration_us == 0)
    {
      hb->min_duration_us = dur;
    }

    hb->counter++;
  }

  const uint8_t MAX_TASKS{10};
  TaskHeartbeat * register_task(const char * name);

  void start_heartbeat();
} // namespace esp32_ui
