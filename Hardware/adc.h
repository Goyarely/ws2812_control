#ifndef __ADC_H
#define __ADC_H

/* 灯珠数量统一由 ws2812.h 的 Pixel_S1_NUM 定义，避免两处硬编码不一致 */
#include "stm32f4xx.h"
#include "sys.h"
#include "ws2812.h"

#define LED_NUM Pixel_S1_NUM   // LED总数（= 灯带实际灯珠数）
#define MAX_ADC 4095           // ADC最大值（12 位满量程）

#define SMOOTHING_FACTOR 0.3f  // 指数移动平均系数：越大响应越快、越不平滑

void Adc_PA3_Init(void);
u16 Get_PA3_Adc_Value(void);

extern uint16_t displayedLEDCount;

/* 指数移动平均滤波：对小变化更敏感，且内存开销恒定（无需缓存数组） */
uint16_t smoothADCValue(uint16_t newValue);
/* 逐帧逼近目标值，差异越大步进越快，避免亮度跳变 */
uint16_t fadeToTargetLEDCount(uint16_t target, uint16_t current);
/* 把 ADC 读数映射为要点亮的灯珠数量 */
uint16_t mapADCToLEDCount(uint16_t adcValue, uint16_t maxLEDs);
/* 音量驱动灯效：按当前 ADC 值点亮对应数量的灯珠，带彩虹流动 */
void LM_to_WS(void);

#endif
