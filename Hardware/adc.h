#ifndef __ADC_H
#define __ADC_H

#include "stm32f4xx.h"
#include "sys.h"


#define LED_NUM 60  // LED总数
#define MAX_ADC 4095 // ADC最大值


#define SMOOTHING_FACTOR 0.3f  // 提高平滑系数，保持一定平滑度但增加响应速度
#define FADE_SPEED 5          // 增加渐变速度

void Adc_PA3_Init(void);
u16 Get_PA3_Adc_Value(void);

extern uint16_t smoothedADCValue;
extern uint16_t displayedLEDCount;

//平滑函数，对小变化更敏感
uint16_t smoothADCValue(uint16_t newValue);
// 动态渐变速度
uint16_t fadeToTargetLEDCount(uint16_t target, uint16_t current);
// 将ADC值映射到指定范围
uint16_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
// 映射函数：将ADC值映射到LED数量
uint16_t mapADCToLEDCount(uint16_t adcValue, uint16_t maxLEDs);
//声音控制ws2818b
void LM_to_WS(void);

#endif
