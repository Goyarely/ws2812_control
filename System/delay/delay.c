#include "delay.h"
#include "sys.h"
// FreeRTOS 必要头文件
#include <FreeRTOS.h>
#include <task.h>

/**
 * @brief  微秒级精确延时（FreeRTOS环境，暂停任务调度避免被打断）
 * @param  nus: 延时微秒数（最大不超过 SysTick 重载值对应的时间）
 */
void delay_us(uint32_t nus)
{
    uint32_t ticks;          // 所需的 SysTick 计数值
    uint32_t told, tnow;     // 记录当前 SysTick 计数器值
    uint32_t tcnt = 0;       // 累计计数
    uint32_t reload;         // SysTick 重载寄存器值

    // 获取 SysTick 重载值（系统定时器的最大计数范围）
    reload = SysTick->LOAD;
    // 计算微秒对应的计数值（SystemCoreClock 为系统主频，单位 Hz）
    ticks = nus * (SystemCoreClock / 1000000);
    // 记录当前 SysTick 计数器值（递减计数）
    told = SysTick->VAL;

    // 暂停任务调度，确保延时过程不被其他任务抢占
    vTaskSuspendAll();

    // 循环等待计数达到目标值
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            // 处理 SysTick 计数器递减逻辑（溢出时从 reload 重新开始）
            if (tnow < told)
                tcnt += told - tnow;  // 未溢出：直接累加差值
            else
                tcnt += reload - tnow + told;  // 溢出：累加重载值到当前值的差值

            told = tnow;

            // 累计计数达到目标值，退出循环
            if (tcnt >= ticks)
                break;
        }
    }

    // 恢复任务调度
    xTaskResumeAll();
}

/**
 * @brief  毫秒级延时（FreeRTOS环境，基于系统任务调度）
 * @param  nms: 延时毫秒数（实际延时可能因任务调度略有偏差，适合非精确场景）
 */
void delay_ms(uint32_t nms)
{
    // 直接使用 FreeRTOS 系统延时函数（会触发任务切换）
    vTaskDelay(nms);
}

/**
 * @brief  秒级延时（基于毫秒延时实现）
 * @param  ns: 延时秒数
 */
void delay_s(uint32_t ns)
{
    for (uint32_t i = 0; i < ns; i++)
    {
        delay_ms(1000);  // 每次延时1秒
    }
}
/**
 * @brief  延时模块初始化
 *
 * @warning 这里**不能**再配置 SysTick。
 *
 * 裸机版本曾把 SysTick 时钟源切到 HCLK/8（21MHz），以便用
 * `SysTick->LOAD = nus * 21` 这种简单算式产生微秒延时。
 *
 * 但在 FreeRTOS 环境下，SysTick 已经被用作系统节拍源：
 * main() 里的 SysTick_Config(SystemCoreClock / configTICK_RATE_HZ)
 * 按 HCLK 配好了 LOAD = 167999，对应 1ms 节拍。
 *
 * 如果在此处改写时钟源而 LOAD 不同步重算，节拍周期会从
 *      167999 / 168MHz = 1.00 ms
 * 变成
 *      167999 / 21MHz  = 8.00 ms
 * 结果是所有 vTaskDelay / pdMS_TO_TICKS / delay_ms 的时间整体放大 8 倍。
 * 因为「慢得一致」，功能表面正常，这类问题极难定位。
 *
 * delay_us() 里的计数值是按 SystemCoreClock（HCLK）算的，
 * 与 FreeRTOS 配置的时钟源一致，因此无需任何额外配置。
 */
void Delay_Init(void)
{
	/* 空实现：SysTick 已由 FreeRTOS 接管，此处不做任何配置 */
}