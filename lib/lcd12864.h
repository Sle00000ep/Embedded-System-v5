/**
 * lcd12864.h — ST7920 LCD 12864 serial mode driver header (Exp6)
 *
 * PE7  = CS   (chip select, active high)
 * PE8  = STD  (serial data SID)
 * PE9  = SCLK (serial clock)
 * PE10 = BLA  (backlight, high=on)
 * PE11 = RST  (reset, active low)
 *
 * PSB hardwired to GND on MainBoard → serial mode only.
 */

#ifndef __LCD12864_H
#define __LCD12864_H

#include "stm32f10x.h"

void LCD12864_Init(void);
void LCD12864_Clear(void);
void LCD12864_SetPos(uint8_t row, uint8_t col);
void LCD12864_WriteString(const char *str);
void LCD12864_WriteChar(char ch);
void LCD12864_DisplayNum(int32_t num);
void LCD12864_DisplayFloat(float val, uint8_t decimals);
void LCD12864_Backlight(uint8_t on);

#endif /* __LCD12864_H */
