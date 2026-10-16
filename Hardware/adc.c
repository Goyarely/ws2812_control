#include "adc.h"
#include "ws2812.h"
#include <stdlib.h> 

uint16_t smoothedADCValue = 0;
uint16_t displayedLEDCount = 0;


/************************************
引脚说明：
可调电阻连接在PA3
ADC12_IN3(通道3)
PA3电压变化范围:5V

************************************/
void Adc_PA3_Init(void)
{
	//结构体变量
	GPIO_InitTypeDef		GPIO_InitStruct;
	ADC_CommonInitTypeDef	ADC_CommonInitStruct;
	ADC_InitTypeDef			ADC_InitStruct;
	
	
	//开启PA口时钟和ADC1时钟，设置PA3为模拟模式。
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);  
	
	
	GPIO_InitStruct.GPIO_Pin	= GPIO_Pin_3; 	//引脚3
	GPIO_InitStruct.GPIO_Mode	= GPIO_Mode_AN;	//模拟模式
	GPIO_InitStruct.GPIO_PuPd	= GPIO_PuPd_NOPULL; //无上下拉
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	
	ADC_CommonInitStruct.ADC_Prescaler	=  ADC_Prescaler_Div4; //84MHZ/4 = 21MHZ  ADC转换频率不能超过36MHZ
	ADC_CommonInitStruct.ADC_Mode		= ADC_Mode_Independent; //独立模式
	ADC_CommonInitStruct.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //不使用DMA
	ADC_CommonInitStruct.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_13Cycles; //两个输入ADC采样间隔 双重 或者三重才起作用
	//初始化ADC_CCR寄存器。
	ADC_CommonInit(&ADC_CommonInitStruct);
	
	
	ADC_InitStruct.ADC_Resolution	= ADC_Resolution_10b;//ADC分辨率选择 4096等分
	ADC_InitStruct.ADC_DataAlign	= ADC_DataAlign_Right; //数据右对齐
	ADC_InitStruct.ADC_ScanConvMode = DISABLE;//不使用扫描
	ADC_InitStruct.ADC_ContinuousConvMode 	= ENABLE; //单次转换
	//使用软件触发，不配置下面的参数
	ADC_InitStruct.ADC_ExternalTrigConvEdge	=ADC_ExternalTrigConvEdge_None; 
	ADC_InitStruct.ADC_NbrOfConversion = 1; 	//转换通道数目
	//初始化ADC1参数，设置ADC1的工作模式以及规则序列的相关信息。
	ADC_Init(ADC1, &ADC_InitStruct);
	//使能ADC。
	 ADC_Cmd(ADC1, ENABLE);
	//配置规则通道参数：
	//规则序列:1 第一个转换
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 1, ADC_SampleTime_3Cycles);
	

}

u16 Get_PA3_Adc_Value(void)
{
    u16 value = 0;
    // 超时计数器：STM32F4主频168MHz，10万次循环≈几十微秒（足够ADC完成单次转换）
    uint32_t timeout = 100000; 

    // 开启软件转换
    ADC_SoftwareStartConv(ADC1);

    // 改进：等待EOC标志 或 超时退出（二选一）
    while((ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) && (timeout-- > 0))
    {
        __NOP(); // 空操作，避免CPU空转（可选，也可直接留空）
    }

    // 判断是否转换成功（未超时）
    if(timeout > 0)
    {
        value = ADC_GetConversionValue(ADC1); // 读取转换结果
        ADC_ClearFlag(ADC1, ADC_FLAG_EOC);    // 清除EOC标志（避免下次采集误判）
    }
    else
    {
        // 超时：打印调试信息（方便定位问题），返回默认值0
        printf("ADC采集超时！PA3未产生EOC标志\r\n");
        value = 0;
    }

    return value;
}

// 平滑函数，对小变化更敏感
uint16_t smoothADCValue(uint16_t newValue) {
    // 使用指数移动平均，对小变化更敏感
    static uint32_t smoothed = 0;
    const float alpha = SMOOTHING_FACTOR;
    
    smoothed = (uint32_t)(alpha * newValue + (1.0f - alpha) * smoothed);
    return (uint16_t)smoothed;
}

// 动态渐变速度
uint16_t fadeToTargetLEDCount(uint16_t target, uint16_t current) {
    uint16_t diff = abs(target - current);
    uint16_t speed;
    
    // 动态调整速度 - 差异越大速度越快
    if(diff > 20) speed = 4;
    else if(diff > 10) speed = 3;
    else if(diff > 5) speed = 2;
    else speed = 1;
    
    if(target > current) {
        return current + ((target - current) > speed ? speed : (target - current));
    } else if(target < current) {
        return current - ((current - target) > speed ? speed : (current - target));
    }
    return current;
}


// 将ADC值映射到指定范围
uint16_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// 映射函数：将ADC值映射到LED数量
uint16_t mapADCToLEDCount(uint16_t adcValue, uint16_t maxLEDs) {
    // 确保ADC值在有效范围内
    if(adcValue > MAX_ADC) adcValue = MAX_ADC;
    
    // 计算要点亮的LED数量
    uint16_t ledCount = (adcValue * maxLEDs * 1.2) / MAX_ADC;
    
    // 确保至少点亮1个LED（如果ADC值>0）
    if(adcValue > 0 && ledCount == 0) {
        ledCount = 1;
    }
    
    return ledCount;
}

 void LM_to_WS(void)
 {
 	uint16_t rawADCValue = Get_PA3_Adc_Value();
 	uint16_t adcValue = smoothADCValue(rawADCValue);
 	uint16_t targetLEDCount = mapADCToLEDCount(adcValue, LED_NUM);
	
 	// 快速响应大变化
 	if(abs(targetLEDCount - displayedLEDCount) > LED_NUM/4) {
 		displayedLEDCount = targetLEDCount;
 	} else {
 		displayedLEDCount = fadeToTargetLEDCount(targetLEDCount, displayedLEDCount);
 	}
	
 	static uint16_t rainbowOffset = 0;
 	rainbowOffset = (rainbowOffset + 1) % 1024;
	
 	// 设置LED颜色，保留边缘渐变效果但减少影响范围
 	for(uint16_t i = 0; i < LED_NUM; i++) {
 		if(i < displayedLEDCount) {
 			RGBColor_TypeDef color = Colourful_Wheel(((i * 256 / LED_NUM) + rainbowOffset) & 255);
			
 			// 仅最后一个LED做渐变
 			if(i == displayedLEDCount - 1) {
 				float fadeFactor = (float)(adcValue * LED_NUM % MAX_ADC) / MAX_ADC;
 				color.R = (uint8_t)(color.R * fadeFactor);
 				color.G = (uint8_t)(color.G * fadeFactor);
 				color.B = (uint8_t)(color.B * fadeFactor);
 			}
			
 			rgb_SetColor(i, color);
 		} else {
 			//rgb_SetColor(i, BLACK);
 		}
 	}
	
 	rgb_SendArray();
 	delay_ms(10); // 减少延时提高响应速度
 }
 // 高灵敏度移动平均滤波
uint16_t high_sensitivity_filter(uint16_t newValue) {
    static uint16_t buffer[3] = {0};
    static uint8_t index = 0;
    
    buffer[index] = newValue;
    index = (index + 1) % 3;
    
    // 使用加权平均，最新值权重更高
    return (buffer[0] + buffer[1] * 2 + buffer[2] * 3) / 6;
}
