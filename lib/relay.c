/**
 * relay.c — Relay driver (Exp10)
 *
 * RELAY1 = PG2 (P17), RELAY2 = PG3 (P16)
 * 8050 NPN driver: GPIO high → transistor on → relay coil energized
 */

#include "relay.h"
#include "pin_config.h"

void Relay_Init(void)
{
    GPIO_InitTypeDef s;

    RCC_APB2PeriphClockCmd(RELAY1_CLK, ENABLE);

    s.GPIO_Pin   = RELAY1_PIN | RELAY2_PIN;
    s.GPIO_Speed = GPIO_Speed_50MHz;
    s.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(RELAY1_PORT, &s);

    /* Default: both relays off (low) */
    GPIO_ResetBits(RELAY1_PORT, RELAY1_PIN);
    GPIO_ResetBits(RELAY2_PORT, RELAY2_PIN);
}

void Relay_Set(uint8_t ch, uint8_t on)
{
    GPIO_TypeDef *port;
    uint16_t pin;

    if (ch == 1)      { port = RELAY1_PORT; pin = RELAY1_PIN; }
    else if (ch == 2) { port = RELAY2_PORT; pin = RELAY2_PIN; }
    else return;

    if (on)
        GPIO_SetBits(port, pin);     /* high → engaged */
    else
        GPIO_ResetBits(port, pin);   /* low  → released */
}

void Relay_Toggle(uint8_t ch)
{
    GPIO_TypeDef *port;
    uint16_t pin;

    if (ch == 1)      { port = RELAY1_PORT; pin = RELAY1_PIN; }
    else if (ch == 2) { port = RELAY2_PORT; pin = RELAY2_PIN; }
    else return;

    if (GPIO_ReadOutputDataBit(port, pin))
        GPIO_ResetBits(port, pin);
    else
        GPIO_SetBits(port, pin);
}
