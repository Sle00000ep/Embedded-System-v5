/**
 * relay.h — Relay driver header (Exp10)
 *
 * RELAY1 = PG2 (P17), RELAY2 = PG3 (P16)
 * High level = relay engaged (8050 NPN driver)
 */

#ifndef __RELAY_H
#define __RELAY_H

#include "stm32f10x.h"

void Relay_Init(void);
void Relay_Set(uint8_t ch, uint8_t on);   /* ch=1/2, on=1=engaged, 0=released */
void Relay_Toggle(uint8_t ch);

#endif /* __RELAY_H */
