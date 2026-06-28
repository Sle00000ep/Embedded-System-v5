#ifndef _KEY_H
#define _KEY_H
#include "sys.h"

#define PA0 GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)
void KEY_init(void);
u8 key_count(void);
#endif

