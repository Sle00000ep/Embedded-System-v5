/**
 * freertos_tasks.h — FreeRTOS 2-task integration (Exp21)
 *
 * ControlTask  (prio 4): polls WKUP, sends mode number on long-press
 * ExperimentTask (prio 3): runs current experiment until stop_requested
 *
 * Queue:  xModeQueue     (1-slot uint8_t)  ControlTask → ExperimentTask
 * Flag:   stop_requested (volatile) ControlTask sets, experiment checks
 * Flag:   scheduler_started (volatile) set by ControlTask, used by delay_ms
 */

#ifndef __FREERTOS_TASKS_H
#define __FREERTOS_TASKS_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ---- Queue: carries mode number (0-20) from ControlTask to ExperimentTask ---- */
extern QueueHandle_t xModeQueue;

/* ---- Stop flag: ControlTask sets this; experiment functions check it ---- */
extern volatile uint8_t stop_requested;

/* ---- Scheduler-started flag: set by first task; delay_ms checks it ---- */
extern volatile uint8_t scheduler_started;

/* ---- Task handles ---- */
extern TaskHandle_t xControlTaskHandle;
extern TaskHandle_t xExperimentTaskHandle;

/* ---- Create tasks and queue; does NOT start scheduler ---- */
void freertos_init(void);

#endif /* __FREERTOS_TASKS_H */
