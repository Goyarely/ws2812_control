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

/* 颜色常量：统一使用 WS_ 前缀。
 *
 * 为什么不叫 RED / GREEN / BLUE：
 * Hardware/LCD/lcd.h 里已经有一组同名宏，但它们是 RGB565 的 16 位数值
 * （RED = 0xF800，用于 LCD 绘图），和这里的 RGB 结构体语义完全不同。
 * C 预处理器对同名宏是"后定义覆盖先定义"，不加前缀时谁生效取决于
 * include 顺序，同一份代码在不同文件里含义可能不同。
 * 加前缀后两组宏彻底隔离，可以放心在同一个文件里同时使用。
 */
#define WS_RED      ((RGBColor_TypeDef){255, 0, 0})
#define WS_ORANGE   ((RGBColor_TypeDef){127, 106, 0})
#define WS_YELLOW   ((RGBColor_TypeDef){255, 255, 0})
#define WS_GREEN    ((RGBColor_TypeDef){0, 255, 0})
#define WS_BLUE     ((RGBColor_TypeDef){0, 0, 255})
#define WS_SKY      ((RGBColor_TypeDef){0, 255, 255})
#define WS_PURPLE   ((RGBColor_TypeDef){65, 105, 225})
#define WS_MAGENTA  ((RGBColor_TypeDef){255, 0, 255})
#define WS_BLACK    ((RGBColor_TypeDef){0, 0, 0})
#define WS_WHITE    ((RGBColor_TypeDef){255, 255, 255})

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
void breathingLight(u16 LedIdStart, u16 LedIdEnd, RGBColor_TypeDef color);
RGBColor_TypeDef Colourful_Wheel(u8 WheelPos);
void smooth_color_gradient(u16 Pixel_LEN, uint16_t duration_ms);
RGBColor_TypeDef interpolate_color(RGBColor_TypeDef start_color, RGBColor_TypeDef end_color, float ratio);

#endif /* __WS2812_H */