/**
 * delay.c — RTOS-aware delay (V5 adapted)
 *
 * When FreeRTOS scheduler is running:
 *   delay_ms() → vTaskDelay() (yields CPU to other tasks)
 * Before scheduler starts:
 *   delay_ms() → busy-wait loop (fallback)
 *
 * scheduler_started flag is set by ControlTask on first entry.
 */

#include "sys.h"
#include "delay.h"

#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"
#include "task.h"
extern volatile uint8_t scheduler_started;
#endif

void delay_ms(u16 time)
{
#if SYSTEM_SUPPORT_OS
    if (scheduler_started) {
        vTaskDelay(pdMS_TO_TICKS(time));
        return;
    }
#endif
    /* Busy-wait fallback (used before scheduler starts) */
    u16 i = 0;
    while (time--) {
        i = 12000;
        while (i--);
    }
}
