#include "tim.h"
#include "lvgl.h"   /* lv_tick_inc() */

/*************************************
定时器说明:
TIM3 -- APB1
TIM3频率：84MHZ
TIM3:16位定时器
用途：为 LVGL 提供 1ms 系统节拍
*************************************/

/**
 * @brief  TIM3 初始化：产生 1ms 周期中断，供 LVGL 计时
 *
 * 时钟链：APB1 = 42MHz，APB1 上的定时器时钟 = 42MHz x 2 = 84MHz
 *         预分频 8400 -> 计数频率 84MHz / 8400 = 10kHz
 *         重装载 10   -> 中断周期 10 / 10kHz = 1ms
 *
 * 注意：FreeRTOS 的系统节拍由 SysTick 提供（见 main.c 的 SysTick_Config），
 *       TIM3 只负责驱动 lv_tick_inc()，两者互不干扰，不要混用。
 */
void Tim3_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct = {0};
    NVIC_InitTypeDef        NVIC_InitStruct = {0};

    /* TIM3 时钟使能 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 时基单元配置 */
    TIM_TimeBaseInitStruct.TIM_Prescaler     = 8400 - 1;    /* 84MHz / 8400 = 10kHz */
    TIM_TimeBaseInitStruct.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_Period        = 10 - 1;      /* 10 / 10kHz = 1ms */
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);

    /* 中断优先级配置（抢占优先级 5，中等，低于 LVGL 任务但高于 LED 任务） */
    NVIC_InitStruct.NVIC_IRQChannel                   = TIM3_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    /* 使能更新中断并启动定时器 */
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

/**
 * @brief  TIM3 更新中断服务函数：给 LVGL 递增 1ms 时基
 */
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
    {
        lv_tick_inc(1);     /* 提供给 LVGL 的 1ms 时间片 */

        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}
