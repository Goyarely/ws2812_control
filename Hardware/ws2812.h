#ifndef __WS2812_H
#define __WS2812_H

/* Includes ------------------------------------------------------------------*/
#include "sys.h"
#include "delay.h"
#include "usart.h"

typedef struct
{
  u8 R;
  u8 G;
  u8 B;
} RGBColor_TypeDef;

// 只使用宏定义，移除重复的const定义
#define RED ((RGBColor_TypeDef){255, 0, 0})
#define ORANGE ((RGBColor_TypeDef){127, 106, 0})
#define YELLOW ((RGBColor_TypeDef){255, 255, 0})
#define GREEN ((RGBColor_TypeDef){0, 255, 0})
#define BLUE ((RGBColor_TypeDef){0, 0, 255})
#define SKY ((RGBColor_TypeDef){0, 255, 255})
#define PURPLE ((RGBColor_TypeDef){65, 105, 225})
#define MAGENTA ((RGBColor_TypeDef){255, 0, 255})
#define BLACK ((RGBColor_TypeDef){0, 0, 0})
#define WHITE ((RGBColor_TypeDef){255, 255, 255})

/* Exported constants --------------------------------------------------------*/
#define Pixel_S1_NUM 60 // 灯珠 RGB数量

#define CODE0 0xC0
#define CODE1 0xFC

void WS2812b_Configuration(void);

/* Basic Color Effects -------------------------------------------------------*/
void RGB_RED(u16 Pixel_LEN);
void RGB_GREEN(u16 Pixel_LEN);
void RGB_BLUE(u16 Pixel_LEN);
void RGB_YELLOW(u16 Pixel_LEN);
void RGB_MAGENTA(u16 Pixel_LEN);
void RGB_BLACK(u16 Pixel_LEN);
void RGB_WHITE(u16 Pixel_LEN);
void RGB_SKY(u16 Pixel_LEN);
void RGB_ORANGE(u16 Pixel_LEN);
void RGB_PURPLE(u16 Pixel_LEN);
void rgb_SetColor(u16 LedId, RGBColor_TypeDef Color);
void rgb_SendArray(void);

void RGB_SetAll(u16 Pixel_LEN, RGBColor_TypeDef color);
/* Complicated Color Effects -------------------------------------------------*/
void rainbowCycle(u16 Pixel_LEN);
void breathingLight(u16 LedIdStart, u16 LedIdEnd, RGBColor_TypeDef color);
RGBColor_TypeDef Colourful_Wheel(u8 WheelPos);
void smooth_color_gradient(u16 Pixel_LEN, uint16_t duration_ms);
void color_gradient_cycle(u16 Pixel_LEN, uint16_t duration_ms);
void pulseEffect(u16 Pixel_LEN, RGBColor_TypeDef color, uint16_t speed);
// 添加缺失的 interpolate_color 函数声明
RGBColor_TypeDef interpolate_color(RGBColor_TypeDef start_color, RGBColor_TypeDef end_color, float ratio);

#endif /* __WS2812_H */