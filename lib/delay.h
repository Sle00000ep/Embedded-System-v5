/**
 * delay.h — Delay functions
 *
 * delay_ms(): When SYSTEM_SUPPORT_OS=1 and FreeRTOS scheduler is running,
 *   calls vTaskDelay() to yield CPU. Otherwise uses busy-wait fallback.
 */

#ifndef _DELAY_H
#define _DELAY_H
#include "sys.h"

void delay_ms(u16 time);

#endif
