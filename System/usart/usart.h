#ifndef __USART_H
#define __USART_H

#include "stm32f4xx.h"
#include "sys.h"
#include "stdio.h"

// USART1 初始化 (调试串口 - PA9/PA10)
void Usart1_Init(uint32_t myBaudRate);

// USART3 初始化 (蓝牙串口 - PB10/PB11)  
void Usart3_Init(uint32_t myBaudRate);

// 向后兼容的小写函数名
void usart1_init(uint32_t baud);

#endif