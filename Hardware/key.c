#include "key.h"


void key_init()
{
	GPIO_InitTypeDef GPIO_InitStruct= {0};
	
	//打开GPIOF的设备时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	//初始化GPIO设备参数 -- PF9 
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0;		//9,10号引脚
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN;		//输出模式
	GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;	//浮空
	GPIO_Init( GPIOA, &GPIO_InitStruct);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4;		//9,10号引脚
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN;		//输出模式
	GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;	//浮空
	GPIO_Init( GPIOE, &GPIO_InitStruct);

}
