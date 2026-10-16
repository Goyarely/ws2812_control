#ifndef __DELAY_H
#define __DELAY_H			   
#include "stm32f4xx.h"
#include <stdint.h>

// 微秒级延时（FreeRTOS环境，精确延时）
void delay_us(uint32_t nus);

// 毫秒级延时（FreeRTOS环境，基于任务调度）
void delay_ms(uint32_t nms);

// 秒级延时（基于毫秒延时实现）
void delay_s(uint32_t ns);
void Delay_Init(void);
#endif