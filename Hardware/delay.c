#include "delay.h"

u32 my_nus = 21;  		//在21MHZ，计21个数，用时1us
u32 my_nms = 21000;  	//在21MHZ，计21000个数，用时1ms

void Delay_Init(void)
{
	//HCLK = 168MHZ  HCLK/8 = 21MHZ
	//SysTick定时器的时钟频率:21MHZ
	//使用外部参考的时钟
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

}

//nus:取值范围：1~798,915
void delay_us(int nus)
{
	//不设置中断
	u32 temp = 0x00;
	
	
	//重装载值寄存器初始值
	SysTick->LOAD = nus*my_nus - 1;
	//设置计数器为0
	SysTick->VAL = 0x00;
	//使能定时器，定时器中的SysTick->VAL值为0，硬件会自动将SysTick->LOAD重新装到SysTick->VAL
	SysTick->CTRL |=(1<<0);
	
	//等待SysTick->CTRL第16位为1
	while(1)
	{
		temp = SysTick->CTRL;
		
		if(temp & (1<<16)) //如果为真,说明16为位1
			break;
		
        //检测系统定时器是否意外关闭    
        if((temp & 0x1)==0) //判断第0位是否为0，0表示定时器被关闭了   	
					return;
	}
	
	//关闭定时器
	SysTick->CTRL &= ~(1<<0);
	
}



//nms:取值范围：1~798
void delay_ms(int nms)
{
	//不设置中断
	u32 temp = 0x00;
	
	
	//重装载值寄存器初始值
	SysTick->LOAD = nms*my_nms - 1;
	//设置计数器为0
	SysTick->VAL = 0x00;
	//使能定时器，定时器中的SysTick->VAL值为0，硬件会自动将SysTick->LOAD重新装到SysTick->VAL
	SysTick->CTRL |=(1<<0);
	
	//等待SysTick->CTRL第16位为1
	while(1)
	{
		temp = SysTick->CTRL;
		
		if(temp & (1<<16)) //如果为真,说明16为位1
			break;
		
        //检测系统定时器是否意外关闭    
        if((temp & 0x1)==0) //判断第0位是否为0，0表示定时器被关闭了	
					return;
	}
	
	//关闭定时器
	SysTick->CTRL &= ~(1<<0);
	
}

void delay_s(int ns)
{
	int i;
	for(i=0; i<ns; i++)
	{
		delay_ms(500);
		delay_ms(500);
	}
}

