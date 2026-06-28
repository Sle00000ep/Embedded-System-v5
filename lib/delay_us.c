/**
 * delay_us.c — DWT (Data Watchpoint and Trace) based microsecond delay
 *
 * DWT cycle counter runs at SystemCoreClock (72MHz → 1 tick = ~13.9ns).
 * Accuracy: ±1 CPU cycle (~14ns).
 */

#include "delay_us.h"

void delay_us_init(void)
{
    /* Enable TRC (Trace) in Debug Exception and Monitor Control Register */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Reset and enable DWT cycle counter */
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * Blocking microsecond delay.
 * Max safe delay: ~59 seconds at 72MHz (uint32_t wrap).
 */
void delay_us(uint32_t us)
{
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < ticks) {
        /* spin */
    }
}
