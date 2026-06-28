/**
 * gpio_out.c — GPIO output driver
 *
 * LED1-8 (PF0-PF7)       — V4 core board built-in LEDs, active-low, no jumper
 * Traffic Light 6ch (PE0-PE5) — MainBoard, active-low, needs jumpers
 * Buzzer (PE6)           — MainBoard 8050 driver, active-high, needs jumper
 */

#include "gpio_out.h"
#include "pin_config.h"

/**
 * Initialize LED1-LED8 (PF0-PF7, core board built-in LEDs)
 * Push-pull output, 50MHz, default high (off, active-low)
 */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(LED1_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = LED_ALL_PINS;
    GPIO_Init(LED_ALL_PORT, &GPIO_InitStructure);

    /* Default all off (high = off, active-low) */
    GPIO_SetBits(LED_ALL_PORT, LED_ALL_PINS);
}

/* LED pin lookup — handles non-zero-based pin assignments (e.g. PC2-PC9) */
static const uint16_t led_pin[8] = {
    LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN,
    LED5_PIN, LED6_PIN, LED7_PIN, LED8_PIN
};

/**
 * Write all 8 LEDs using BSRR (atomic, no glitch on non-LED pins)
 * val: bit0=LED1 ... bit7=LED8
 */
void LED_WriteAll(uint8_t val)
{
    uint16_t set_bits = 0, reset_bits = 0;
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (val & (1 << i))
            reset_bits |= led_pin[i];  /* active-low: reset = on */
        else
            set_bits |= led_pin[i];    /* active-low: set = off */
    }
    GPIO_Write(LED_ALL_PORT,
        (GPIO_ReadOutputData(LED_ALL_PORT) & ~LED_ALL_PINS) | set_bits);
    /* Then reset the 'on' LEDs */
    GPIO_ResetBits(LED_ALL_PORT, reset_bits);
}

/**
 * Control single LED
 * index: 0-7 (LED1-LED8)
 * on: 1=on, 0=off
 */
void LED_Set(uint8_t index, uint8_t on)
{
    if (index > 7) return;
    if (on)
        GPIO_ResetBits(LED_ALL_PORT, led_pin[index]);   /* low = on */
    else
        GPIO_SetBits(LED_ALL_PORT, led_pin[index]);      /* high = off */
}

/**
 * All LEDs off
 */
void LED_AllOff(void)
{
    GPIO_SetBits(LED_ALL_PORT, LED_ALL_PINS);
}

/**
 * All LEDs on
 */
void LED_AllOn(void)
{
    GPIO_ResetBits(LED_ALL_PORT, LED_ALL_PINS);
}

/**
 * Initialize Traffic Light 6ch (PE0-PE5) + Buzzer (PE6)
 * Push-pull output, 50MHz
 * LEDs: default high (off, active-low)
 * Buzzer: default low (off, active-high via 8050)
 */
void TrafficLight_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(NS_R_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = TRAFFIC_PINS | BUZZER_PIN;
    GPIO_Init(TRAFFIC_PORT, &GPIO_InitStructure);

    /* Traffic LEDs off (high = off, active-low) */
    GPIO_SetBits(TRAFFIC_PORT, TRAFFIC_PINS);
    /* Buzzer off (low = off, active-high via 8050) */
    GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
}

/**
 * Set traffic light state
 * ns: 0=Red 1=Yellow 2=Green
 * we: 0=Red 1=Yellow 2=Green
 * LEDs are active-low: ResetBits = on, SetBits = off
 */
void TrafficLight_Set(uint8_t ns, uint8_t we)
{
    /* Turn all off first */
    GPIO_SetBits(TRAFFIC_PORT, TRAFFIC_PINS);

    /* North-South */
    switch (ns) {
        case 0: GPIO_ResetBits(NS_R_PORT, NS_R_PIN); break;
        case 1: GPIO_ResetBits(NS_Y_PORT, NS_Y_PIN); break;
        case 2: GPIO_ResetBits(NS_G_PORT, NS_G_PIN); break;
    }

    /* West-East */
    switch (we) {
        case 0: GPIO_ResetBits(WE_R_PORT, WE_R_PIN); break;
        case 1: GPIO_ResetBits(WE_Y_PORT, WE_Y_PIN); break;
        case 2: GPIO_ResetBits(WE_G_PORT, WE_G_PIN); break;
    }
}

/**
 * Buzzer control (PE6, active-high via 8050 NPN)
 * on: 1=sound, 0=silent
 */
void Buzzer_Set(uint8_t on)
{
    if (on)
        GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);    /* high -> 8050 on -> sound */
    else
        GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);  /* low -> 8050 off -> silent */
}
