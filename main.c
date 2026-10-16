#include <stm32f4xx.h>
#include <lcd.h>
#include <usart.h>
#include <lvgl.h>
#include <lv_port_disp.h>
#include <lv_port_indev.h>
#include <FreeRTOS.h>
#include <task.h>
#include <myiic.h>
#include <ui.h>
#include <stdio.h>
#include "delay.h"
#include "tim.h"
#include "string.h"
#include "ws2812.h"
#include "key.h"
#include "adc.h"

// 蓝牙指令定义
#define CMD_RED    'R'    // 红色
#define CMD_ORANGE 'O'    // 橙色
#define CMD_YELLOW 'Y'    // 黄色
#define CMD_GREEN  'G'    // 绿色
#define CMD_SKY    'C'    // 天蓝色
#define CMD_BLUE   'B'    // 蓝色
#define CMD_PURPLE 'P'    // 紫色
#define CMD_BREATH 'H'    // 呼吸效果
#define CMD_RAINBOW 'W'   // 彩虹效果
#define CMD_OFF    'X'    // 关闭所有
#define CMD_VOLUME 'V'    // 切换到音量控制模式

// 音量控制相关定义
#define VOLUME_MIN 100     // 最小音量AD值
#define VOLUME_MAX 3000  // 最大音量AD值
#define SAMPLE_COUNT 5     // 采样次数(滤波)
#define SAMPLE_INTERVAL 50 // 采样间隔(ms)
#define SENSITIVITY_BOOST 1.2f  // 灵敏度增强系数
RGBColor_TypeDef current_color = {255, 0, 0};  // 默认红色
// 任务句柄
TaskHandle_t app_task_init_handle = NULL;
TaskHandle_t app_task_lvgl_handle = NULL;
TaskHandle_t app_task_led_control_handle = NULL;
TaskHandle_t xModeMutexHandle = NULL;  // 模式切换互斥锁

// 全局变量
volatile char bt_cmd = 0;         // 蓝牙指令接收变量
uint8_t current_mode = 0;         // 0:蓝牙控制模式 1:音量控制模式

// 函数声明
static void app_task_init(void *pvParameters);
static void app_task_lvgl(void *pvParameters);
static void app_task_led_control(void *pvParameters);
void tim3_init(void);
void timer_cb(struct _lv_timer_t *id);

// 蓝牙相关函数
void USART3_IRQHandler(void);
void execute_bt_command(char cmd);
RGBColor_TypeDef getVolumeColor(u8 volumeLevel);

int main()
{
    //配置优先级分组 --- 只需要在main函数调用一次即可，不要在其他地方调用，否则会出现错误
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  //只有抢占优先级， 范围0~15
    
    /* 系统定时器中断频率为configTICK_RATE_HZ */
    SysTick_Config(SystemCoreClock / configTICK_RATE_HZ);
    
    /* 创建app_task_init任务 */
    xTaskCreate((TaskFunction_t)app_task_init,          /* 任务入口函数 */
                (const char *)"app_task_init",          /* 任务名字 */
                (uint16_t)1024,                         /* 任务栈大小 */
                (void *)NULL,                           /* 任务入口函数参数 */
                (UBaseType_t)5,                         /* 任务的优先级 */
                (TaskHandle_t *)&app_task_init_handle); /* 任务控制块指针 */
                
    /* 开启任务调度 */
    vTaskStartScheduler();
}

void timer_cb(struct _lv_timer_t *id)
{
    static int i = 0;
    // lv_label_set_text_fmt( ui_Label1, "i=%d\r\n",i++);
}

static void app_task_init(void *pvParameters)
{
    // 串口1初始化波特率为115200bps
    usart1_init(115200);
    printf("=== 系统启动 ===\r\n");
    
    // 1. 先初始化基础外设（在LVGL之前）
    printf("初始化基础外设...\r\n");
    Delay_Init();
    Usart3_Init(9600);    // 蓝牙串口(USART3)
    
    // 2. 初始化WS2812（在LVGL之前）
    printf("初始化WS2812...\r\n");
    WS2812b_Configuration();
    printf("WS2812初始化完成\r\n");
    
    // 简单的LED测试
    RGB_WHITE(1);
    delay_ms(100);
    RGB_BLACK(Pixel_S1_NUM);
    
    // 3. 初始化其他外设
    key_init();
    Adc_PA3_Init();
    
    // 4. 初始化LVGL
    printf("初始化LVGL...\r\n");
    lv_init();
    printf("LVGL初始化完成\r\n");
    
    // 5. 初始化LVGL显示
    lv_port_disp_init();
    LCD_Scan_Dir(D2U_L2R); //180度旋转
    printf("显示初始化完成\r\n");
    
    // 6. 初始化LVGL输入设备
    printf("初始化输入设备...\r\n");
    lv_port_indev_init();
    printf("输入设备初始化完成\r\n");

    // 7. 定时器3初始化
    tim3_init();
    printf("定时器初始化完成\r\n");
    
    // 8. 界面初始化
    ui_init();
    printf("UI界面初始化完成\r\n");
    
    // 9. 创建LVGL定时器
    lv_timer_create(timer_cb, 1000, NULL);

    // 10. 创建任务
    xTaskCreate((TaskFunction_t)app_task_lvgl,
                (const char *)"app_task_lvgl",
                (uint16_t)2048,  // 增加堆栈大小
                (void *)NULL,
                (UBaseType_t)7,
                (TaskHandle_t *)&app_task_lvgl_handle);
                
    xTaskCreate((TaskFunction_t)app_task_led_control,
                (const char *)"app_task_led_control",
                (uint16_t)2048,
                (void *)NULL,
                (UBaseType_t)4,
                (TaskHandle_t *)&app_task_led_control_handle);

    printf("所有任务创建完成，系统正常运行\r\n");
    
    // LED启动指示
    RGB_GREEN(1);
    delay_ms(500);
    RGB_BLACK(Pixel_S1_NUM);
    
    vTaskDelete(NULL);
}

static void app_task_lvgl(void *pvParameters)
{
    for (;;)
    {
        // 定期检查与执行LVGL所有已注册的周期性任务
        lv_task_handler();
        vTaskDelay(5);
    }
}

static void app_task_led_control(void *pvParameters)
{
    u16 adc_value = 0;
    u16 adc_avg = 0;
    u8 volume_level = 0;
    u8 i, j;
    static uint32_t effect_counter = 0;

    printf("LED控制任务启动\r\n");

    for (;;)
    {
        // 1. 优先处理蓝牙指令
        if(bt_cmd != 0) {
            printf("收到蓝牙指令: %c\r\n", bt_cmd);
            execute_bt_command(bt_cmd);
            bt_cmd = 0;
        }
        
        // 2. 根据当前模式执行效果
        switch(current_led_mode) {
            case 0: // 单色模式
                // UI切换时已设置，此处无需操作
                break;
                
            case 1: // 呼吸模式
                if(effect_counter % 40 == 0) { // 每200ms更新
                    static uint8_t breath_color_idx = 0;
                    RGBColor_TypeDef breath_colors[] = {RED, ORANGE, YELLOW, GREEN, SKY, BLUE, PURPLE};
                    breathingLight(0, Pixel_S1_NUM-1, breath_colors[breath_color_idx]);
                    breath_color_idx = (breath_color_idx + 1) % 7;
                }
                break;
                
            case 2: // 液体模式
                smooth_color_gradient(Pixel_S1_NUM, 100);
                break;
                
            case 3: // 向下流水模式
            {
                static int rainbow_offset = 0;
                for(i = 0; i < Pixel_S1_NUM; i++) {
                    rgb_SetColor(i, Colourful_Wheel(((i * 256 / Pixel_S1_NUM) + rainbow_offset) & 255));
                }
                rgb_SendArray();
                rainbow_offset = (rainbow_offset + 1) % 256;
                break;
            }
                
          case 4: // 音乐节奏模式（高灵敏度彩虹频谱）
		{
			static int rainbow_offset = 0;
			static uint16_t last_adc_value = 0;
			
			// 快速单次采样（提高响应速度）
			adc_value = Get_PA3_Adc_Value();
			
			// 动态灵敏度调整：对变化更敏感
			uint16_t dynamic_adc = adc_value;
			if (abs(adc_value - last_adc_value) > 50) { // 变化较大时增强响应
				dynamic_adc = (adc_value * 3 + last_adc_value) / 4; // 快速响应
			}
			last_adc_value = adc_value;
			
			// 应用灵敏度增强
			int16_t boosted_adc = (int16_t)(dynamic_adc * SENSITIVITY_BOOST);
			if (boosted_adc > 4095) boosted_adc = 4095;
			if (boosted_adc < 0) boosted_adc = 0;

			// 计算音量等级(0-60)，使用非线性映射增强小信号
			if (boosted_adc < VOLUME_MIN) {
				volume_level = 0;
			} else if (boosted_adc > VOLUME_MAX) {
				volume_level = 60;
			} else {
				// 非线性映射：对小信号更敏感
				float normalized = (float)(boosted_adc - VOLUME_MIN) / (VOLUME_MAX - VOLUME_MIN);
				// 使用指数曲线增强小信号灵敏度
				float enhanced = powf(normalized, 0.7f); // 小于1的指数增强小信号
				volume_level = (u8)(enhanced * 60);
			}

			// 动态范围扩展：确保即使小声也能看到效果
			if (volume_level < 5 && volume_level > 0) {
				volume_level = 5; // 最小可见效果
			}

			// 高灵敏度彩虹频谱效果
			for (j = 0; j < Pixel_S1_NUM; j++) {
				if (j < volume_level) {
					// 更丰富的颜色变化
					u8 hue = (j * 300 / Pixel_S1_NUM + rainbow_offset) % 255;
					RGBColor_TypeDef led_color = Colourful_Wheel(hue);
					
					// 根据音量强度调整亮度
					if (volume_level > 40) {
						// 高音量时增强亮度
						led_color.R = (led_color.R * 3) / 2;
						led_color.G = (led_color.G * 3) / 2;
						led_color.B = (led_color.B * 3) / 2;
					}
					
					rgb_SetColor(j, led_color);
				} else {
					// 更平滑的衰减效果
					if (j < volume_level + 8) {
						u8 fade_distance = j - volume_level;
						u8 fade_brightness = 80 - fade_distance * 10;
						if (fade_brightness > 0) {
							// 衰减区域使用暖色调
							RGBColor_TypeDef fade_color = {
								fade_brightness / 2, 
								fade_brightness / 4, 
								0
							};
							rgb_SetColor(j, fade_color);
						} else {
							RGBColor_TypeDef black_color = {0, 0, 0};
							rgb_SetColor(j, black_color);
						}
					} else {
						RGBColor_TypeDef black_color = {0, 0, 0};
						rgb_SetColor(j, black_color);
					}
				}
			}
			rgb_SendArray();
			
			// 更快的彩虹流动
			rainbow_offset = (rainbow_offset + 3) % 256;
			
			// 调试输出（可选）
			static uint32_t debug_counter = 0;
			if (debug_counter++ % 50 == 0) {
				printf("ADC: %d -> 音量等级: %d/60\r\n", adc_value, volume_level);
			}
			break;
		}
				}

        effect_counter++;
        vTaskDelay(pdMS_TO_TICKS(5)); // 保持5ms基本延时
    }
}
// 音量对应的颜色计算
RGBColor_TypeDef getVolumeColor(u8 volumeLevel) {
    RGBColor_TypeDef color;
    u8 ratio = (volumeLevel * 255) / 60; // 0-60映射到0-255
    color.G = 255 - ratio;  // 绿色随音量增大减弱
    color.R = ratio;        // 红色随音量增大增强
    color.B = 0;            // 蓝色关闭
    return color;
}

// USART3(蓝牙)中断服务函数
void USART3_IRQHandler(void) {
    if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET) {
        bt_cmd = USART_ReceiveData(USART3);  // 读取指令
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);  // 清除标志
    }
}

// 执行蓝牙指令
void execute_bt_command(char cmd) {
    current_mode = 0; // 进入蓝牙控制模式
    
    // 创建颜色变量用于函数调用
    RGBColor_TypeDef color_red = {255, 0, 0};
    RGBColor_TypeDef color_orange = {127, 106, 0};
    RGBColor_TypeDef color_yellow = {255, 255, 0};
    RGBColor_TypeDef color_green = {0, 255, 0};
    RGBColor_TypeDef color_blue = {0, 0, 255};
    RGBColor_TypeDef color_sky = {0, 255, 255};
    RGBColor_TypeDef color_purple = {65, 105, 225};
    
    switch(cmd) {
        case CMD_RED:
            RGB_RED(Pixel_S1_NUM);
            printf("执行红色效果\r\n");
            break;
        case CMD_ORANGE:
            RGB_ORANGE(Pixel_S1_NUM);
            printf("执行橙色效果\r\n");
            break;
        case CMD_YELLOW:
            RGB_YELLOW(Pixel_S1_NUM);
            printf("执行黄色效果\r\n");
            break;
        case CMD_GREEN:
            RGB_GREEN(Pixel_S1_NUM);
            printf("执行绿色效果\r\n");
            break;
        case CMD_SKY:
            RGB_SKY(Pixel_S1_NUM);
            printf("执行天蓝色效果\r\n");
            break;
        case CMD_BLUE:
            RGB_BLUE(Pixel_S1_NUM);
            printf("执行蓝色效果\r\n");
            break;
        case CMD_PURPLE:
            RGB_PURPLE(Pixel_S1_NUM);
            printf("执行紫色效果\r\n");
            break;
        case CMD_BREATH:
            printf("执行呼吸灯效果\r\n");
            for(int i = 0; i < 2; i++) {  // 执行2组呼吸（减少次数避免阻塞）
                breathingLight(0, Pixel_S1_NUM-1, color_red);
                breathingLight(0, Pixel_S1_NUM-1, color_orange);
                breathingLight(0, Pixel_S1_NUM-1, color_yellow);
                breathingLight(0, Pixel_S1_NUM-1, color_green);
                breathingLight(0, Pixel_S1_NUM-1, color_sky);
                breathingLight(0, Pixel_S1_NUM-1, color_blue);
                breathingLight(0, Pixel_S1_NUM-1, color_purple);
                if(bt_cmd != 0) break;  // 有新指令则退出
            }
            break;
        case CMD_RAINBOW:
            printf("执行彩虹效果\r\n");
            // 在FreeRTOS中，彩虹效果需要修改为不阻塞的方式
            for(int j = 0; j < 100; j++) { // 减少循环次数
                for(int i = 0; i < Pixel_S1_NUM; i++) {
                    rgb_SetColor(i, Colourful_Wheel(((i * 256 / Pixel_S1_NUM) + j) & 255));
                }
                rgb_SendArray();
                vTaskDelay(pdMS_TO_TICKS(20));
                if(bt_cmd != 0) break;
            }
            break;
        case CMD_OFF:
            RGB_BLACK(Pixel_S1_NUM);
            printf("关闭所有灯\r\n");
            break;
        case CMD_VOLUME:
            current_mode = 1; // 切换到音量控制模式
            printf("切换到音量控制模式\r\n");
            break;
        default:
            printf("未知指令: %c\r\n", cmd);
            break;
    }
}

// FreeRTOS钩子函数
void vApplicationMallocFailedHook(void)
{
    printf("内存分配失败！\r\n");
    taskDISABLE_INTERRUPTS();
    for(;;);
}

void vApplicationIdleHook(void)
{
    // 空闲任务钩子函数
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    printf("栈溢出！任务名：%s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for(;;);
}

void vApplicationTickHook(void)
{
    // 系统时钟节拍钩子函数
}

// TIM3初始化函数
void tim3_init()
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct = {0};
    NVIC_InitTypeDef        NVIC_InitStruct = {0};
    
    //TIM3的时钟使能
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);    

    //初始化基本定时器的功能
    TIM_TimeBaseInitStruct.TIM_Prescaler        = 8400-1;    //预分配值, 定时器的时钟频率=APB1总线*2= 42MHz*2=84MHz 预分频值=8400， 定时器的计数频率=定时器时钟频率/预分频值=84MHz/8400=10000hz
    TIM_TimeBaseInitStruct.TIM_CounterMode        = TIM_CounterMode_Up;    //向上计数模式    
    TIM_TimeBaseInitStruct.TIM_Period            = 10-1;    //定时器的计数值
                                                            //时间=计数值个数*(1/频率) = 10*(1/10000) = 1ms
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);

    //定时器中断优先级配置
    NVIC_InitStruct.NVIC_IRQChannel                        =    TIM3_IRQn;    //TIM3中断通道
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority    =   5;                //抢占优先级（调整为中等优先级）
    NVIC_InitStruct.NVIC_IRQChannelSubPriority            =   0;                //响应优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd                    =   ENABLE;            //使能中断通道
    NVIC_Init(&NVIC_InitStruct);
    
    
    //启动定时器的定时完成中断(更新中断)
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    //使能定时器的工作
    TIM_Cmd(TIM3, ENABLE);    
    
}

//定时器3的更新中断服务函数
void TIM3_IRQHandler(void)
{
    //检查定时器是否定时完成
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
    {
        lv_tick_inc(1);        //提供给lvgl-1ms的时间
        
        //清空定时器的定时完成标志位
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}