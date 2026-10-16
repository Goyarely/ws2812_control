#ifndef __TIM_H
#define __TIM_H

#include "stm32f4xx.h"

/* TIM3：为 LVGL 提供 1ms 系统节拍 */
void Tim3_Init(void);

#endif
