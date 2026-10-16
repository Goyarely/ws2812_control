#include "ws2812.h"
#include "ui.h"
// 呼吸灯相关参数
#define BREATHING_STEPS 200   // 呼吸步数，越大越平滑 200*10=2000ms
#define BREATHING_DELAY_MS 2 // 每步延迟时间（毫秒）
#define PIXEL_MAX_NUM         128    // 最大LED数量（防止越界）



//  每个LED需要24个字节表示颜色数据
static u8 pixelBuffer[Pixel_S1_NUM][24];

/***********************************************************************************************
**     name: WS2812b_Configuration
** function:  WS2812B SPI DMA总线初始化
**parameter: void
************************************************************************************************/
/*****************************************
 说明：
 SPI2：
 引脚：使用的是PB15引脚，在TFTLCD下的LCD BL
 时钟：根据总线图，SPI2由APB1(42MHz)分频而来
 ****************************************/
void WS2812b_Configuration(void)
{

  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef SPI_InitStructure;
  DMA_InitTypeDef DMA_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // 使能GPIOB时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);  // 使能SPI2时钟
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);  // DMA1时钟使能

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;         // PB15复用功能输出
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;       // 复用功能
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;     // 推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; // 100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       // 上拉
  GPIO_Init(GPIOB, &GPIO_InitStructure);             // 初始化

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI3); // PB15复用为 SPI2

  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // 设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                      // 设置SPI工作模式:设置为主SPI
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                  // 设置SPI的数据大小:SPI发送接收8位帧结构
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;                         // 串行同步时钟的空闲状态为高电平
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                       // 串行同步时钟的第二个跳变沿（上升或下降）数据被采样
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                          // NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // 42M/8=5.25M
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                 // 指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
  SPI_InitStructure.SPI_CRCPolynomial = 7;                           // CRC值计算的多项式
  SPI_Init(SPI3, &SPI_InitStructure);                                // 根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器

  SPI_Cmd(SPI3, ENABLE); // 使能SPI外设
	
  DMA_DeInit(DMA1_Stream5);

  while (DMA_GetCmdStatus(DMA1_Stream5) != DISABLE)
  {
  } // 等待DMA可配置

  DMA_InitStructure.DMA_Channel = DMA_Channel_0;                          // 通道选择
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&SPI3->DR;              // DMA外设地址
  DMA_InitStructure.DMA_Memory0BaseAddr = (u32)pixelBuffer;               // DMA 存储器0地址
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;                 // 存储器到外设模式
  DMA_InitStructure.DMA_BufferSize = Pixel_S1_NUM * 24;                   // 数据传输量
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;        // 外设非增量模式
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                 // 存储器增量模式
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 外设数据长度:8位
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;         // 存储器数据长度:8位
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                           // 使用普通模式
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;                   // 中等优先级
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;         // 存储器突发单次传输
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; // 外设突发单次传输
  DMA_Init(DMA1_Stream5, &DMA_InitStructure);                         // 初始化DMA Stream

  SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE); // 使能SPI2的DMA发送
  DMA_Cmd(DMA1_Stream5, DISABLE);                  // 关闭DMA传输
  while (DMA_GetCmdStatus(DMA1_Stream5) != DISABLE)
  {
  } // 确保DMA可以被设置
  DMA_SetCurrDataCounter(DMA1_Stream5, Pixel_S1_NUM * 24); // 数据传输量
  DMA_Cmd(DMA1_Stream5, ENABLE);

  delay_ms(1);
  RGB_BLACK(Pixel_S1_NUM); // RGB RESET
  delay_ms(1);
 
}
	
/***********************************************************************************************
**     name: rgb_SetColor
** function: 设定某个RGB LED的颜色
**parameter: void
**   return: void
************************************************************************************************/
void rgb_SetColor(u16 LedId, RGBColor_TypeDef Color)
{

  u16 i;

  if (LedId > (Pixel_S1_NUM))
  {
    printf("Error:Out of Range!\r\n");
    return; // to avoid overflow
  }
// 应用亮度调节（0-255缩放）
  RGBColor_TypeDef adjustedColor;
  float brightnessRatio = (float)led_brightness / 255.0f;
  adjustedColor.R = (uint8_t)(Color.R * brightnessRatio);
  adjustedColor.G = (uint8_t)(Color.G * brightnessRatio);
  adjustedColor.B = (uint8_t)(Color.B * brightnessRatio);
  // 重要：使用 adjustedColor 而不是原始的 Color
    for (i = 0; i <= 7; i++)
    {
        pixelBuffer[LedId][i] = ((adjustedColor.G & (1 << (7 - i))) ? (CODE1) : CODE0);
    }
    for (i = 8; i <= 15; i++)
    {
        pixelBuffer[LedId][i] = ((adjustedColor.R & (1 << (15 - i))) ? (CODE1) : CODE0);
    }
    for (i = 16; i <= 23; i++)
    {
        pixelBuffer[LedId][i] = ((adjustedColor.B & (1 << (23 - i))) ? (CODE1) : CODE0);
    }
}

/***********************************************************************************************
**     name: rgb_SendArray
** function: Configure colors to RGB pixel series.
             RGBColor_TypeDef: pointer to a RGBColor_TypeDef structure that contains the color configuration information for the RGB pixel.
**parameter: void
**   return: void
************************************************************************************************/
void rgb_SendArray(void)
{

  if (DMA_GetFlagStatus(DMA1_Stream5, DMA_FLAG_TCIF5) != RESET)
  {                                                  // 等待DMA1_Steam4传输完成
    DMA_ClearFlag(DMA1_Stream5, DMA_FLAG_TCIF5);     // 清除DMA1_Steam4传输完成标志，先预想DMA_FLAG_TCIF0的零，代表的是通道0
    SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE); // 使能SPI2的DMA发送
    DMA_Cmd(DMA1_Stream5, DISABLE);                  // 关闭DMA传输
    while (DMA_GetCmdStatus(DMA1_Stream5) != DISABLE)
    {
    } // 确保DMA可以被设置
    DMA_SetCurrDataCounter(DMA1_Stream5, Pixel_S1_NUM * 24); // 数据传输量
    DMA_Cmd(DMA1_Stream5, ENABLE);                           // 开启DMA传输
  }
}

/***********************************************************************************************
**     name: RGB_RED
** function: 设定颜色为RED
**parameter: Pixel_LEN 灯珠数
**   return: void
************************************************************************************************/
void RGB_RED(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, RED);  // 使用宏定义
  }
  rgb_SendArray();
}

void RGB_PURPLE(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, PURPLE);  // 使用宏定义
  }
  rgb_SendArray();
}

void RGB_SKY(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, SKY);  // 使用宏定义
  }
  rgb_SendArray();
}

void RGB_MAGENTA(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, MAGENTA);  // 使用宏定义
  }
  rgb_SendArray();
}

void RGB_ORANGE(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, ORANGE);  // 使用宏定义
  }
  rgb_SendArray();
}

void RGB_GREEN(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, GREEN);  // 使用宏定义
  }
  rgb_SendArray();
}

void RGB_BLUE(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, BLUE);  // 使用宏定义
  }
  rgb_SendArray();
}

void RGB_YELLOW(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, YELLOW);  // 使用宏定义
  }
  rgb_SendArray();
}

void RGB_BLACK(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, BLACK);  // 使用宏定义
  }
  rgb_SendArray();
}

void RGB_WHITE(u16 Pixel_LEN)
{
  u16 i;
  for (i = 0; i < Pixel_LEN; i++)
  {
    rgb_SetColor(i, WHITE);  // 使用宏定义
  }
  rgb_SendArray();
}

/***********************************************************************************************
**     name: RGB_SetAll
** function: 设置所有LED为指定颜色
**parameter:
**   - Pixel_LEN: LED数量
**   - color: 要设置的颜色
**   return: void
************************************************************************************************/
void RGB_SetAll(u16 Pixel_LEN, RGBColor_TypeDef color)
{
    u16 i;

    for (i = 0; i < Pixel_LEN; i++)
    {
        rgb_SetColor(i, color);
    }

    rgb_SendArray();
}
/***********************************************************************************************
**     name: Colourful_Wheel
** function: 彩虹色轮生成（适配WS2812B GRB顺序）
** parameter: WheelPos - 颜色索引（0-255）
** return: RGBColor_TypeDef - GRB颜色
************************************************************************************************/
RGBColor_TypeDef Colourful_Wheel(uint8_t WheelPos)
{
    RGBColor_TypeDef color;
    WheelPos = 255 - WheelPos;  // 颜色顺序反转（可注释取消）

    if (WheelPos < 85)
    {
        color.R = 255 - WheelPos * 3;
        color.G = 0;
        color.B = WheelPos * 3;
    }
    else if (WheelPos < 170)
    {
        WheelPos -= 85;
        color.R = 0;
        color.G = WheelPos * 3;
        color.B = 255 - WheelPos * 3;
    }
    else
    {
        WheelPos -= 170;
        color.R = WheelPos * 3;
        color.G = 255 - WheelPos * 3;
        color.B = 0;
    }

    return color;
}

/***********************************************************************************************
**     name: rainbowCycle
** function: 彩虹循环（所有LED同步渐变彩虹色）
** parameter: Pixel_LEN - LED数量
** return: void
************************************************************************************************/
void rainbowCycle(uint16_t Pixel_LEN)
{
    if (Pixel_LEN == 0 || Pixel_LEN > PIXEL_MAX_NUM) return;

    uint16_t i, j;
    for (j = 0; j < 1023; j++)  // 20s完成一次全循环
    {
        for (i = 0; i < Pixel_LEN; i++)
        {
            rgb_SetColor(i, Colourful_Wheel(((i * 256 / Pixel_LEN) + j) & 255));
        }
        rgb_SendArray();
        delay_ms(20);
    }
}

/***********************************************************************************************
**     name: breathingLight
** function: 指定区间呼吸灯（平滑亮度变化）
** parameter: LedIdStart/End - LED区间，color - 呼吸颜色
** return: void
************************************************************************************************/
void breathingLight(uint16_t LedIdStart, uint16_t LedIdEnd, RGBColor_TypeDef color)
{
    // 边界修正
    LedIdStart = (LedIdStart > PIXEL_MAX_NUM - 1) ? (PIXEL_MAX_NUM - 1) : LedIdStart;
    LedIdEnd = (LedIdEnd > PIXEL_MAX_NUM - 1) ? (PIXEL_MAX_NUM - 1) : LedIdEnd;
    if (LedIdStart > LedIdEnd) return;

    uint16_t i, brightness;
    for (uint16_t step = 0; step < BREATHING_STEPS; step++)
    {
        // 亮度计算（前半段渐亮，后半段渐暗）
        brightness = step < BREATHING_STEPS/2 ? 
                     (uint16_t)(255.0f * step/(BREATHING_STEPS/2)) :
                     (uint16_t)(255.0f * (BREATHING_STEPS - step)/(BREATHING_STEPS/2));
        brightness = (brightness > 255) ? 255 : brightness;

        // 亮度适配颜色
        RGBColor_TypeDef adjusted = {
            .R = (color.R * brightness)/255,
            .G = (color.G * brightness)/255,
            .B = (color.B * brightness)/255
        };

        // 设置区间LED
        for (i = LedIdStart; i <= LedIdEnd; i++) rgb_SetColor(i, adjusted);
        rgb_SendArray();
        delay_ms(BREATHING_DELAY_MS);
    }
}

/***********************************************************************************************
**     name: interpolate_color
** function: 颜色插值（生成两色过渡色）
** parameter: start/end_color - 起始/目标色，ratio - 过渡比例（0.0-1.0）
** return: RGBColor_TypeDef - 过渡色
************************************************************************************************/
RGBColor_TypeDef interpolate_color(RGBColor_TypeDef start_color, RGBColor_TypeDef end_color, float ratio)
{
    ratio = (ratio < 0.0f) ? 0.0f : (ratio > 1.0f) ? 1.0f : ratio;

    RGBColor_TypeDef result;
    result.R = (uint8_t)(start_color.R + (int16_t)(end_color.R - start_color.R) * ratio + 0.5f);
    result.G = (uint8_t)(start_color.G + (int16_t)(end_color.G - start_color.G) * ratio + 0.5f);
    result.B = (uint8_t)(start_color.B + (int16_t)(end_color.B - start_color.B) * ratio + 0.5f);

    // 边界限制
    result.R = (result.R > 255) ? 255 : (result.R < 0) ? 0 : result.R;
    result.G = (result.G > 255) ? 255 : (result.G < 0) ? 0 : result.G;
    result.B = (result.B > 255) ? 255 : (result.B < 0) ? 0 : result.B;

    return result;
}

/***********************************************************************************************
**     name: color_gradient_cycle
** function: 全局同色渐变（所有LED同步切换颜色）
** parameter: Pixel_LEN - LED数量，duration_ms - 两色过渡时长（ms）
** return: void
************************************************************************************************/
void color_gradient_cycle(uint16_t Pixel_LEN, uint16_t duration_ms)
{
    if (Pixel_LEN == 0 || Pixel_LEN > PIXEL_MAX_NUM || duration_ms == 0) return;

    static uint32_t cnt = 0;
    static uint8_t color_idx = 0;
    RGBColor_TypeDef colors[] = {RED, ORANGE, YELLOW, GREEN, SKY, BLUE, PURPLE, WHITE};
    uint8_t color_cnt = sizeof(colors)/sizeof(colors[0]);

    // 取当前起始色和目标色
    RGBColor_TypeDef start = colors[color_idx];
    RGBColor_TypeDef end = colors[(color_idx + 1) % color_cnt];
    float ratio = (float)(cnt % duration_ms) / duration_ms;

    // 生成过渡色并刷新
    RGBColor_TypeDef grad = interpolate_color(start, end, ratio);
    RGB_SetAll(Pixel_LEN, grad);
    rgb_SendArray();

    cnt++;
    if (cnt % duration_ms == 0) color_idx = (color_idx + 1) % color_cnt;
}

/***********************************************************************************************
**     name: smooth_color_gradient
** function: 流水渐变（LED按位置渐变不同颜色，模拟液体流动）
** parameter: Pixel_LEN - LED数量，duration_ms - 渐变周期（ms）
** return: void
************************************************************************************************/
void smooth_color_gradient(uint16_t Pixel_LEN, uint16_t duration_ms)
{
    if (Pixel_LEN == 0 || Pixel_LEN > PIXEL_MAX_NUM || duration_ms == 0) return;

    static uint32_t cnt = 0;
    RGBColor_TypeDef colors[] = {RED, ORANGE, YELLOW, GREEN, SKY, BLUE, PURPLE};
    uint8_t color_cnt = sizeof(colors)/sizeof(colors[0]);
    uint32_t total_steps = (uint32_t)duration_ms * color_cnt;
    uint32_t curr_step = cnt % total_steps;
    uint32_t step_per_color = duration_ms;

    // 计算当前颜色索引和过渡比例
    uint8_t curr_idx = (uint8_t)(curr_step / step_per_color);
    uint32_t step_in_color = curr_step % step_per_color;
    float ratio = (float)step_in_color / step_per_color;

    RGBColor_TypeDef start = colors[curr_idx];
    RGBColor_TypeDef end = colors[(curr_idx + 1) % color_cnt];

    // 每个LED按位置偏移生成渐变色
    for (uint16_t i = 0; i < Pixel_LEN; i++)
    {
        float led_ratio = (float)(i % Pixel_LEN) / Pixel_LEN;  // 每个LED的位置比例
        // 修正：用浮点数加减替代取模，实现循环（超过1.0→0.0，等价于%1.0f）
        float final_ratio = ratio + led_ratio;
        if (final_ratio >= 1.0f) {
            final_ratio -= 1.0f;  // 超过1.0时减去1.0，实现循环
        }
        // 调用颜色插值函数（参数为合法float类型）
        RGBColor_TypeDef led_color = interpolate_color(start, end, final_ratio);
        rgb_SetColor(i, led_color);
    }
    rgb_SendArray();
    cnt++;
}

/***********************************************************************************************
**     name: pulseEffect
** function: 全局脉冲（所有LED同色同步呼吸，区别于多色呼吸）
** parameter: Pixel_LEN - LED数量，baseColor - 基础颜色，speed - 脉冲速度（ms）
** return: void
************************************************************************************************/
void pulseEffect(uint16_t Pixel_LEN, RGBColor_TypeDef baseColor, uint16_t speed)
{
    if (Pixel_LEN == 0 || Pixel_LEN > PIXEL_MAX_NUM) return;

    static uint8_t brightness = 0;
    static int8_t dir = 1;  // 1=渐亮，-1=渐暗

    // 更新亮度
    brightness += dir;
    if (brightness >= 255) { brightness = 255; dir = -1; }
    else if (brightness <= 0) { brightness = 0; dir = 1; }

    // 亮度适配颜色
    RGBColor_TypeDef adjusted = {
        .R = (baseColor.R * brightness)/255,
        .G = (baseColor.G * brightness)/255,
        .B = (baseColor.B * brightness)/255
    };

    // 刷新所有LED
    RGB_SetAll(Pixel_LEN, adjusted);
    rgb_SendArray();
    delay_ms(speed);
}