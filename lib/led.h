#ifndef _LED_H
#define _LED_H
#include "sys.h"

#define led0 PEout(0)
#define led1 PEout(1)
#define led2 PEout(2)
#define led3 PEout(3)

void led_init(void);
#endif
