#include "key.h"
#include "sys.h"
#include "delay.h"

void KEY_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
}
u8 key_count(void)
{
	static u8 key_up = 0;
	if(PA0 ==0)
	{
		delay_ms(15);
		key_up++;
		if(key_up == 1)
		{
			return 1;
		}
		if(key_up == 2)
		{
		 key_up=0;
		 return 2;
		}
	}
	return 0;
}
