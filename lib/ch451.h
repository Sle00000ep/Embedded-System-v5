/**
 * ch451.h — CH451 数码管+键盘驱动 (Exp7)
 *
 * 根据 CH451DS1 数据手册:
 *   12位命令 LSB 优先
 *   LOAD=0 传输, LOAD=1 锁存
 *   BCD译码模式 (显示参数 b7=1)
 *   按键代码 7 位 (bit6=按下标志, bit5-0=键号)
 */

#ifndef __CH451_H
#define __CH451_H

#include "stm32f10x.h"

void CH451_Init(void);
void CH451_WriteCmd(uint16_t word);
void CH451_SetRaw(uint8_t pos, uint8_t segments);
void CH451_SetDigit(uint8_t pos, uint8_t val);  /* BCD: 0-9=数字, 0x10=空格 */
void CH451_DisplayNum(int32_t num);
void CH451_Clear(void);
void CH451_SetBrightness(uint8_t level);
uint8_t CH451_ReadKey(void);   /* 返回键号(0-63) 或 0xFF(无按键) */

#endif /* __CH451_H */
