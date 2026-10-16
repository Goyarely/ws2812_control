# WS2812 氛围灯控制器

基于 **STM32F407ZE** 的 60 灯 WS2812B 灯带控制器。**触摸屏 + 蓝牙 App + 电位器**三路输入，
五种灯效模式，灯带由 **SPI3 + DMA** 驱动，界面用 **LVGL 8.3.11** 跑在 FSMC 并口 TFT 上，
系统跑 **FreeRTOS**。

---

## 目录

- [1. 功能特性](#1-功能特性)
- [2. 硬件](#2-硬件)
- [3. 软件架构](#3-软件架构)
- [4. WS2812 驱动原理](#4-ws2812-驱动原理)
- [5. 蓝牙指令协议](#5-蓝牙指令协议)
- [6. 编译与烧录](#6-编译与烧录)
- [7. 目录结构](#7-目录结构)
- [8. 代码构成与来源说明](#8-代码构成与来源说明)
- [9. 缺陷修复记录](#9-缺陷修复记录)
- [10. 已知限制与后续计划](#10-已知限制与后续计划)

---

## 1. 功能特性

### 三路控制入口

| 入口 | 硬件 | 说明 |
| --- | --- | --- |
| 触摸屏 | 电阻触摸（SPI 位操作）+ AT24C02 存校准参数 | 7 个颜色按钮、亮度滑块、模式下拉框 |
| 蓝牙 App | USART3，9600bps | 单字符指令，11 条命令 |
| 电位器 | PA3 = ADC1_IN3 | 音量/旋钮采集，驱动「音乐节奏」模式 |

### 五种灯效模式

下拉框选项与 `current_led_mode` 一一对应：

| 索引 | 下拉框 | 效果 | 实现函数 |
| --- | --- | --- | --- |
| 0 | `off` | 单色常亮（由颜色按钮切换） | `RGB_SetAll()` |
| 1 | `Breathing` | 7 色循环呼吸 | `breathingLight()` |
| 2 | `Water Flow` | 平滑颜色渐变 | `smooth_color_gradient()` |
| 3 | `Rainbow` | 彩虹流水（色轮偏移） | `Colourful_Wheel()` |
| 4 | `Music Rhythm` | ADC 音量映射点亮灯珠数 + 彩虹频谱 | `LM_to_WS()` |

### 其他

- **全局亮度**：滑块 0–255，在 `rgb_SetColor()` 里统一做比例缩放，对所有模式和所有颜色生效
- **颜色按钮**：红/橙/黄/绿/青/蓝/紫 7 色，点击后自动切回模式 0
- **状态回显**：LCD 控件实时显示当前颜色名或亮度百分比
- **断帧保护**：DMA 发送带 50ms 超时，异常时放弃本帧而不是覆盖缓冲区

---

## 2. 硬件

### 核心器件

| 器件 | 型号 / 参数 |
| --- | --- |
| 主控 | STM32F407ZE，Cortex-M4F，168MHz，512KB Flash / 192KB SRAM |
| 灯带 | WS2812B，60 颗灯珠，5V 供电 |
| 显示屏 | FSMC 并口 TFT（ILI9341 系列），320×240 横屏 |
| 触摸 | 电阻触摸控制器，SPI 位操作，校准参数存 AT24C02 |
| 晶振 | HSE 8MHz → PLL（M=8, N=336, P=2）→ SYSCLK 168MHz |

### 接线表

| 功能 | 引脚 | 外设 | 备注 |
| --- | --- | --- | --- |
| WS2812 数据 | **PB5** | SPI3_SCK（AF_SPI3） | 5.25 Mbps，DMA1_Stream5 |
| LCD 数据/控制 | — | FSMC Bank1 NORSRAM4 | `LCD_BASE = 0x6C000000 \| 0x7E`，A6 作 RS |
| LCD 背光 | PB15 | GPIO 输出 | |
| 触摸 PEN / DOUT / DIN / CLK / CS | PB1 / PB2 / PF11 / PB0 / PC13 | GPIO 位操作 | |
| 电位器 | **PA3** | ADC1_IN3 | 12 位，56 周期采样 |
| 调试串口 | PA9 / PA10 | USART1 | 115200bps，`printf` 重定向到此 |
| 蓝牙串口 | PB10 / PB11 | USART3 | 9600bps |
| 触摸校准存储 | — | IIC 位操作 → AT24C02 | |

---

## 3. 软件架构

### 分层

```
┌─────────────────────────────────────────────────────────────┐
│  应用层   main.c                                            │
│    app_task_init / app_task_lvgl / app_task_led_control      │
│    蓝牙指令解析 execute_bt_command()                          │
├─────────────────────────────────────────────────────────────┤
│  界面层   UI/ (SquareLine Studio 生成 + 手写事件回调)          │
│    ui_Screen1 / ui_Dropdown1 / ui_Slider1 / 7 个颜色按钮      │
│    ui.c 里的回调负责把 UI 状态同步给灯带                       │
├─────────────────────────────────────────────────────────────┤
│  驱动层   Hardware/  System/                                 │
│    ws2812.c  adc.c  tim.c  key.c                             │
│    LCD/lcd.c  TOUCH/touch.c  24CXX/24cxx.c  IIC/myiic.c       │
│    System/delay  System/sys  System/usart                     │
├─────────────────────────────────────────────────────────────┤
│  中间件   FreeRTOS 10.x (heap_4)   LVGL 8.3.11               │
├─────────────────────────────────────────────────────────────┤
│  硬件抽象  STM32F4xx_StdPeriph_Driver (来自 Keil DFP 包)      │
└─────────────────────────────────────────────────────────────┘
```

### FreeRTOS 任务划分

| 任务 | 栈（字） | 优先级 | 职责 |
| --- | --- | --- | --- |
| `app_task_init` | 1024 | 5 | 外设初始化 + LVGL 初始化 + 创建其余任务，完成后自删 |
| `app_task_lvgl` | 2048 | 7 | 循环 `lv_task_handler()`，周期 5ms |
| `app_task_led_control` | 2048 | 4 | 蓝牙指令处理 + 五种灯效调度，周期 5ms |
| IDLE | — | 0 | |

关键配置（`FreeRTOSConfig.h`）：

```c
configUSE_PREEMPTION                    1
configTICK_RATE_HZ                      1000        // 1ms 节拍
configMAX_PRIORITIES                    32
configTOTAL_HEAP_SIZE                   30 * 1024   // heap_4.c
configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  5
configCHECK_FOR_STACK_OVERFLOW          2
```

### 时基分工（重点）

系统里有**两个**定时源，职责严格分开：

| 定时源 | 用途 | 配置位置 |
| --- | --- | --- |
| **SysTick** | FreeRTOS 系统节拍（1ms） | `vTaskStartScheduler()` 内部配置 |
| **TIM3** | LVGL 的 `lv_tick_inc(1)` | `Hardware/tim.c` → `Tim3_Init()` |

- `Tim3_Init()`：预分频 `8400-1`（84MHz/8400 = 10kHz），周期 `10-1` → **1ms** 更新中断，
  抢占优先级 5
- `lv_conf.h` 里 `LV_TICK_CUSTOM 0`，所以必须由 TIM3 手动喂 tick
- **`Delay_Init()` 是空实现** —— 原因见 [第 9 节](#9-缺陷修复记录) P0-1

### 数据流

```
触摸屏点击 ──┐
蓝牙指令   ──┼──► current_led_mode / current_color / led_brightness
电位器 ADC ──┘         │
                       ▼
        app_task_led_control (5ms 轮询)
                       │
                       ▼
              rgb_SetColor()  ← 此处统一应用亮度缩放
                       │
                       ▼
        pixelBuffer[60][24] (GRB, 每位展开成 1 个 SPI 字节)
                       │
                       ▼
            rgb_SendArray() → SPI3 + DMA1_Stream5 → 灯带
```

---

## 4. WS2812 驱动原理

WS2812B 用**单线归零码**：每一位由一个高电平加一个低电平组成，靠**高电平占空比**区分 0 和 1。
本工程不用 GPIO 翻转 + 纳秒延时，而是**用 SPI 把每一位展开成 8 个 bit**，让硬件外设自己产生精确时序。

### 编码表

SPI3 挂 APB1（42MHz），`SPI_BaudRatePrescaler_8` → **5.25 Mbps**，一个 SPI bit ≈ 190.5ns。

| WS2812B 位 | 高电平占比 | SPI 字节 | 二进制 |
| --- | --- | --- | --- |
| `0` | 短高 | `CODE0 = 0xC0` | `1100_0000` |
| `1` | 长高 | `CODE1 = 0xFC` | `1111_1100` |
| RESET | — | 数据线拉低 > 280µs | 帧间隔 |

所以 1 颗灯珠 = 24 个 WS2812B 位 = 24 个 SPI 字节 = **192 个 SPI bit**；
60 颗灯 = 1440 字节，在 5.25 Mbps 下一帧约 **2.2ms**。

### 缓冲区布局

```c
static u8 pixelBuffer[Pixel_S1_NUM][24];   // [60][24]
```

字节序为 **GRB**（不是 RGB），与 WS2812B 一致：

| 下标 | 通道 |
| --- | --- |
| `[0..7]` | G（高位在前） |
| `[8..15]` | R |
| `[16..23]` | B |

### 帧间复位

`rgb_SendArray()` 在启动 DMA 前先 `delay_us(300)` 拉低数据线。
SPI 空闲时 `CPOL_Low` 输出低电平，所以这 300µs 就构成了 WS2812B 要求的复位间隔。
**这一步不能省** —— 否则连续发帧时帧间隔只有几百纳秒，灯带会显示错乱。

---

## 5. 蓝牙指令协议

串口参数：**USART3，9600bps，8N1**。发送**单个 ASCII 字符**即可。

| 字符 | 宏 | 效果 |
| --- | --- | --- |
| `R` | `CMD_RED` | 全灯红色 |
| `O` | `CMD_ORANGE` | 全灯橙色 |
| `Y` | `CMD_YELLOW` | 全灯黄色 |
| `G` | `CMD_GREEN` | 全灯绿色 |
| `C` | `CMD_SKY` | 全灯天蓝色 |
| `B` | `CMD_BLUE` | 全灯蓝色 |
| `P` | `CMD_PURPLE` | 全灯紫色 |
| `H` | `CMD_BREATH` | 呼吸效果（2 轮 7 色） |
| `W` | `CMD_RAINBOW` | 彩虹流水（100 帧，20ms/帧） |
| `X` | `CMD_OFF` | 关闭全部灯珠 |
| `V` | `CMD_VOLUME` | 切换到音量控制模式 |

### 指令接收链路

```
USART3_IRQHandler()          // 中断里只做一件事：把字节存进 volatile char bt_cmd
        │
        ▼
app_task_led_control()       // 任务里轮询 bt_cmd != 0，然后调用
        │
        ▼
execute_bt_command(cmd)      // 真正执行灯效（可能耗时，所以放在任务里而不是中断里）
```

设计要点：中断服务函数**不执行任何灯效**，只置一个标志位；耗时的 SPI 发送和呼吸动画
全部在任务上下文完成。长动画（呼吸、彩虹）在循环里检查 `bt_cmd != 0` 以便被新指令打断。

---

## 6. 编译与烧录

### 工具链

| 项 | 版本 |
| --- | --- |
| IDE | Keil MDK-ARM **V5** |
| 编译器 | ARM Compiler 5（AC5 / armcc） |
| C 标准 | **C99**（工程已勾选 `--c99`，代码里有语句后声明变量） |
| 优化 | Level 3 |
| 设备包 | **Keil.STM32F4xx_DFP 1.0.8** |

### ⚠️ 关于设备包版本

本工程依赖 ST 的**标准外设库**（`stm32f4xx_adc.h`、`stm32f4xx_dma.h` …），
而这些头文件**不在仓库里** —— 它们来自 Keil 设备包：

```
<Keil Pack Root>/Keil/STM32F4xx_DFP/1.0.8/Device/StdPeriph_Driver/{inc,src}
```

**必须装 1.0.8 或同为「自带 StdPeriph 驱动」的早期版本。**
较新的 STM32F4xx_DFP 已移除 `StdPeriph_Driver` 组件，装新版会直接报
`cannot open source input file "stm32f4xx_adc.h"`。

> 工程里的 `RTE/Device/STM32F407ZE/stm32f4xx_conf.h` 由设备包提供，
> 通过 RTE 机制自动把 `StdPeriph_Driver/inc` 加进 include 路径，
> 所以 `.uvprojx` 的 `<IncludePath>` 里看不到它。

### 源码编码

所有 `.c/.h` 为 **GBK（GB2312）编码 + CRLF 行尾**（Keil 在中文 Windows 下的默认行为）。

- 用 Keil 打开：`Edit → Configuration → Editor → Encoding` 选 **Chinese GB2312**，否则中文注释乱码
- 用 VSCode 打开：右下角编码切到 **GB2312**，或加 `"files.encoding": "gbk"`
- **不要**用 UTF-8 保存这些文件，会破坏原有中文注释

### 步骤

1. 装 Keil MDK V5 + ARM Compiler 5 + `STM32F4xx_DFP 1.0.8`
2. 打开 `Project.uvprojx`
3. `Project → Rebuild all target files`
4. 用 J-Link / ST-Link 下载，或直接烧 `Objects/Project.hex`

### 上电现象

1. 串口 1（115200）打印 `=== 系统启动 ===` 及初始化进度
2. 灯带白闪一下（`RGB_WHITE(1)`）再熄灭 → WS2812 自检
3. 灯带绿闪一下（`RGB_GREEN(1)`）再熄灭 → 系统就绪
4. 屏幕进入 UI，可触摸操作；此时可连蓝牙发指令

### 调试输出

`printf` 已重定向到 **USART1（PA9/PA10，115200）**。开串口助手能看到：

```
=== 系统启动 ===
初始化基础外设...
初始化WS2812...
WS2812初始化完成
...
收到蓝牙指令: R
执行红色效果
ADC: 1234 -> 音量等级: 42/60
```

---

## 7. 目录结构

```
.
├── Project.uvprojx              # Keil 工程（唯一需要提交的工程文件）
├── .gitignore
├── README.md
├── main.c                       # 应用层：任务、蓝牙指令、模式调度
│
├── Hardware/                    # 驱动层
│   ├── ws2812.c/.h              # WS2812B SPI+DMA 驱动 + 灯效函数
│   ├── adc.c/.h                 # PA3 电位器采集 + 音量→灯珠数映射
│   ├── tim.c/.h                 # TIM3，给 LVGL 提供 1ms tick
│   ├── key.c/.h                 # 按键初始化（预留）
│   ├── LCD/                     # FSMC 并口 TFT 驱动 + 字库（第三方移植）
│   ├── TOUCH/                   # 电阻触摸驱动 + 校准（第三方移植）
│   ├── 24CXX/                   # AT24C02 EEPROM（存触摸校准参数，第三方移植）
│   └── IIC/                     # IIC 位操作（第三方移植）
│
├── System/
│   ├── delay/                   # delay_us / delay_ms / delay_s
│   ├── sys/                     # 位带操作、时钟、中断开关
│   └── usart/                   # USART1 调试 + USART3 蓝牙 + printf 重定向
│
├── UI/                          # SquareLine Studio 生成 + 手写事件回调
│   ├── ui.c/.h                  # 手写：事件回调、状态同步、亮度应用
│   ├── screens/ui_Screen1.c     # 生成：控件创建
│   ├── components/              # 生成
│   ├── fonts/ images/           # 生成：字库、图片的 C 数组
│   └── filelist.txt             # SquareLine 的记录文件
│
├── FreeRTOS/                    # FreeRTOS 10.x（heap_4 + Cortex-M4F 移植）
├── lvgl/                        # LVGL 8.3.11 + examples/porting 里的移植文件
├── FONT/                        # 字库资源
└── RTE/                         # Keil RTE 配置（由设备包提供）
```

---

## 8. 代码构成与来源说明

本仓库是在一份现成的「STM32F407 + LVGL 触摸屏」工程模板基础上做的**二次开发**。
为便于后续维护和阅读，这里把「沿用」和「在此之上完成」的部分说清楚。

### 沿用基础工程的部分（非本仓库编写）

| 部分 | 内容 |
| --- | --- |
| 工程框架 | `.uvprojx`、启动文件、`system_stm32f4xx.c`、RTE 配置 |
| 显示/输入驱动 | `Hardware/LCD/`、`Hardware/TOUCH/`、`Hardware/IIC/`、`Hardware/24CXX/`（正点原子风格移植） |
| RTOS / GUI 移植 | `FreeRTOS/`、`lvgl/examples/porting/` 下的 `lv_port_disp.c`、`lv_port_indev.c` |
| 灯带底层骨架 | `Hardware/ws2812.c` 的 SPI+DMA 初始化与颜色缓冲区填充 |
| UI 骨架 | `UI/screens/`、`UI/fonts/`、`UI/images/`（SquareLine Studio 生成） |

### 在此之上完成的部分

| 部分 | 内容 |
| --- | --- |
| 任务架构 | 三个任务的划分、优先级、栈大小，初始化顺序，任务间自删 |
| 模式调度 | `current_led_mode` 状态机，5 种模式在 `app_task_led_control` 里的调度 |
| 灯效实现 | `breathingLight` 呼吸、`smooth_color_gradient` 渐变、`Colourful_Wheel` 色轮、`interpolate_color` 插值 |
| 蓝牙协议 | `CMD_*` 指令表、中断收字节 + 任务里执行的两级结构、长动画可打断 |
| ADC 链路 | 指数移动平均滤波 → 动态步进逼近 → 非线性映射 → 彩虹频谱；含 ADC 超时保护 |
| UI ↔ 灯带联动 | `ui.c` 事件回调、`apply_brightness_to_leds()`、状态回显、全局亮度缩放 |
| 时基梳理 | TIM3 专职喂 LVGL tick，与 FreeRTOS 的 SysTick 解耦 |
| 代码审计与修复 | 见下一节，共 3 个 P0 + 6 个 P1，含一轮死代码清理 |

---

## 9. 缺陷修复记录

在做简历项目整理时对整个工程做了一轮静态审计，下面是查出并修掉的问题。
**这些问题全部是「编译能过、功能表面正常、但根子上不对」的类型**，
也是这个项目最值得讲的部分。

### P0 — 会导致功能错误或内存越界

#### P0-1　`Delay_Init()` 改写了 SysTick 时钟源，让所有延时慢 8 倍

```c
// 修复前
void Delay_Init(void) {
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);   // 168MHz → 21MHz
}
```

这是裸机版本留下来的代码：那时把 SysTick 时钟源切到 HCLK/8，
好让 `SysTick->LOAD = nus * 21` 这种简单算式产生微秒延时。

但在 FreeRTOS 环境下 SysTick 是**系统节拍源**，`main()` 里已经按 HCLK 配好了
`LOAD = 167999`（对应 1ms）。这里只改时钟源、**不同步重算 LOAD**，节拍周期就变成：

```
167999 / 168MHz = 1.00 ms     →     167999 / 21MHz = 8.00 ms
```

于是所有 `vTaskDelay()` / `pdMS_TO_TICKS()` / `delay_ms()` 的时间**整体放大 8 倍**。
因为「慢得一致」，功能看起来正常，这类问题极难定位。

**修复**：改成空实现。`delay_us()` 里的计数值本来就是按 `SystemCoreClock` 算的，
与 FreeRTOS 配置的时钟源一致，不需要任何额外配置。

#### P0-2　ADC 分辨率与实际量程不匹配

```c
// 修复前
ADC_InitStruct.ADC_Resolution = ADC_Resolution_10b;   // 0~1023
ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 1, ADC_SampleTime_3Cycles);
#define MAX_ADC 4095                                   // 但代码按 0~4095 算
```

分辨率配 10 位（满量程 1023），而 `MAX_ADC`、`VOLUME_MAX 3000`、映射公式全都按 12 位（4095）写。
结果是 ADC 读数永远到不了 1023 以上，`VOLUME_MAX 3000` 这个阈值**永远触发不到**，
音量模式在满量程时也点不满灯珠。

另外 `ADC_SampleTime_3Cycles` 采样时间太短 —— 音频信号源（电位器/分压网络）输出阻抗高，
采样保持电容充电不足，读数偏低且跳变。

**修复**：`ADC_Resolution_12b` + `ADC_SampleTime_56Cycles`，并把错误注释一并订正。

#### P0-3　`powf()` 没有包含 `<math.h>`

`app_task_led_control` 里用 `powf(normalized, 0.7f)` 做非线性映射，但没 `#include <math.h>`。
ARMCC 下这是 **#223-D 隐式声明**告警 —— 返回值被当作 `int` 处理，
`powf` 的小数结果被截断，**音量映射曲线直接失效**（小信号灵敏度增强形同虚设）。

**修复**：补 `<math.h>`（以及 `abs()` 需要的 `<stdlib.h>`）。

### P1 — 稳健性 / 可维护性

| 编号 | 问题 | 修复 |
| --- | --- | --- |
| P1-1 | `UI/ui_events.c` 是 SquareLine 生成后又被手写逻辑替代的残留，未参与编译但会让下次重新生成时产生重复定义 | 删除文件及 `filelist.txt` 里的对应行 |
| P1-2 | `Hardware/delay.c/.h` 与 `System/delay/delay.c` 是两份重复实现，谁生效取决于 include 顺序 | 删除 `Hardware/delay.*`，只保留 `System/delay/` |
| P1-3 | 颜色宏 `RED/GREEN/BLUE/...` 与 `Hardware/LCD/lcd.h` 里同名但语义完全不同的 RGB565 宏冲突（后者 `RED = 0xF800`） | 颜色宏统一加 `WS_` 前缀（`WS_RED`…），与 LCD 宏彻底隔离 |
| P1-4 | `Hardware/tim.c` 里有两个从未被调用的 TIM3 初始化函数，而真正在用的那份写在 `main.c` 里（分频值还是错的 `84/1000-1`） | 把 `main.c` 里的正确实现（`8400-1`）移到驱动层 `Hardware/tim.c`，删掉两个死函数 |
| P1-5 | `rgb_SendArray()` 在 DMA 忙时**静默丢帧**，且完全没发 WS2812 帧间复位低电平 | 改为「等待上一帧完成 + 50ms 超时保护」，并在发帧前插入 `delay_us(300)` 复位间隔 |
| P1-6 | `rgb_SetColor()` 边界检查写成 `if (LedId > Pixel_S1_NUM)`，差一位，`LedId == 60` 会越界写 `pixelBuffer[60]` | 改为 `>=` |
| P1-9 | `main.c` 里的 `current_color` 变量遮蔽了 `ui.c` 里的真身；`current_mode` 只赋值从不读取 | 删除遮蔽变量与无效变量 |

### 死代码清理

删除了以下从未被调用的函数和变量（逐个用引用扫描确认过）：

- `Hardware/ws2812.c`：`rainbowCycle()`、`color_gradient_cycle()`、`pulseEffect()`
- `Hardware/adc.c`：`map()`、`high_sensitivity_filter()`、`smoothedADCValue`
- `main.c`：`getVolumeColor()`、`SAMPLE_COUNT` / `SAMPLE_INTERVAL` 宏、局部 `tim3_init()` 与 `TIM3_IRQHandler()`
- `Hardware/adc.h`：`FADE_SPEED` 宏

主文件行数变化：`main.c` 485 → 424 行，`Hardware/ws2812.c` 526 → 445 行。

### 修正的错误注释

代码对、注释错是最容易误导后来人的一类问题，一并订正：

- `Hardware/ws2812.c`：注释写「SPI2 / PB15」，实际是 **SPI3 / PB5**（PB15 是 LCD 背光）；
  `SPI_CPOL_Low` 的注释写成「空闲状态为高电平」，应为低电平
- `UI/ui.c`：`current_led_mode` 的编号注释与下拉框选项、`main.c` 的分支对不上

### 验证方式说明

本机**没有 ARM 工具链**（无 Keil / armcc / arm-none-eabi-gcc），因此上述修改
**只做了静态验证，没有实际编译和上板**。已做的验证：

| 项目 | 方法 | 结果 |
| --- | --- | --- |
| 语法结构 | 全文件大括号配平检查（含注释/字符串剥离） | 12 个文件全部 depth=0、min=0 |
| 编码与行尾 | 原始字节检查 | 全部 GBK、无 BOM、CRLF 一致 |
| 符号拼写 | 提取全部被调用的函数名与宏名，对照 StdPeriph 官方头文件 | 249 个标准库前缀标识符，0 个拼写错误 |
| 死代码删除 | 删除前逐个做全工程引用扫描 | 0 个残留引用 |
| 宏重命名 | 状态机式替换（跳过字符串/字符/行注释/块注释） | 0 个残留旧名、0 个误伤 |

**修改后需在 Keil 里 Rebuild 并在硬件上复测**，尤其是 ADC 量程和灯带帧率。

---

## 10. 已知限制与后续计划

### 已知限制

- **未上板验证**：本次代码修改未编译、未烧录（原因见上）
- **无压测数据**：灯珠数固定 60，没有做长灯带 / 高帧率的压力测试
- **`System/sys/sys.h` 与 `Hardware/sys.h` 仍是重复头文件**：`adc.h` 和 `ws2812.h` 都
  `#include "sys.h"`，而 ARMCC 优先搜索 includer 自身目录，当前实际解析到 `Hardware/sys.h`。
  直接删会改变解析结果，需要连同 include 一起改，留作后续
- **`Hardware/key.c` 只有 `key_init()`**，没有任何按键读取函数，按键功能未实现
- **没有做单元测试 / 静态分析工具接入**（工程本身没有测试框架）

### 后续计划

- [ ] Keil 里 Rebuild，消除剩余告警，硬件复测
- [ ] 补充演示照片与视频（`doc/` 目录）
- [ ] 合并 `Hardware/sys.h` 与 `System/sys/sys.h`
- [ ] 灯珠数、颜色通道顺序改成编译期可配置，支持不同长度的灯带
- [ ] 蓝牙指令加上参数（如 `B128` 设置亮度），从单字符扩展成简单协议
- [ ] 接入 `lv_conf.h` 的性能监视器，量化 UI 帧率与 CPU 占用

---

## 附：主要参数速查

| 参数 | 值 | 位置 |
| --- | --- | --- |
| 灯珠数量 | 60 | `Hardware/ws2812.h` → `Pixel_S1_NUM` |
| 缓冲区 | 1440 字节 | `pixelBuffer[60][24]` |
| SPI 时钟 | 5.25 Mbps | `SPI_BaudRatePrescaler_8`（APB1 42MHz） |
| 单帧耗时 | ≈ 2.2ms | 1440 字节 ÷ 5.25Mbps |
| DMA 超时 | 50ms | `WS2812_DMA_TIMEOUT_MS` |
| 帧间复位 | 300µs | `rgb_SendArray()` |
| 亮度范围 | 0–255 | `ui.c` → `led_brightness` |
| ADC 量程 | 0–4095 | `adc.h` → `MAX_ADC` |
| 音量阈值 | 100–3000 | `main.c` → `VOLUME_MIN/MAX` |
| LVGL 分辨率 | 320×240 | `lv_port_disp.c` |
| LVGL 绘制缓冲 | 320×10 单缓冲 | `buf_1[]` |
| LVGL 内存池 | 48KB 静态 | `lv_conf.h` → `LV_MEM_SIZE` |
| FreeRTOS 堆 | 30KB | `heap_4.c` |
