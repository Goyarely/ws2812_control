#include "usart.h"
#include "stdio.h"

// 结构体定义
static GPIO_InitTypeDef GPIO_InitStructure;
static NVIC_InitTypeDef NVIC_InitStructure;
static USART_InitTypeDef USART_InitStructure;

#pragma import(__use_no_semihosting)

struct __FILE { int handle; };
FILE __stdout;
FILE __stdin;

// printf重定向到USART1
int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, ch);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

// 系统退出函数
int _sys_exit(int x) 
{ 
    x = x; 
    return 0; 
}

// USART1 初始化 (调试串口 - PA9/PA10)
void Usart1_Init(uint32_t myBaudRate)
{
    // 使能GPIOA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    
    // 使能USART1时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    
    // 配置PA9、PA10为复用功能
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 引脚复用映射
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
    
    // 配置USART1参数
    USART_InitStructure.USART_BaudRate = myBaudRate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);
    
    // 配置USART1中断
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    // 使能USART1
    USART_Cmd(USART1, ENABLE);
}

// USART3 初始化 (蓝牙串口 - PB10/PB11)
void Usart3_Init(uint32_t myBaudRate)
{
    // 使能GPIOB时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    
    // 使能USART3时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    
    // 配置PB10、PB11为复用功能
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 引脚复用映射
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);
    
    // 配置USART3参数
    USART_InitStructure.USART_BaudRate = myBaudRate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART3, &USART_InitStructure);
    
    // 配置USART3中断
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能接收中断
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    
    // 使能USART3
    USART_Cmd(USART3, ENABLE);
}

// 向后兼容的小写函数名
void usart1_init(uint32_t baud)
{
    Usart1_Init(baud);
}

// USART1 中断服务函数 (调试串口)
void USART1_IRQHandler(void)
{
    static uint8_t received_data = 0;
    
    // 检查是否接收到数据
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        received_data = USART_ReceiveData(USART1);
        
        // 回显接收到的数据 (可选功能)
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, received_data);
        
        // 清除中断标志
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

//// USART3 中断服务函数 (蓝牙串口)
//void USART3_IRQHandler(void)
//{
//    // 检查是否接收到数据
//    if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET) 
//    {
//        // 读取接收到的数据
//        char bt_cmd = USART_ReceiveData(USART3);
//        
//        // 这里可以添加蓝牙指令处理逻辑
//        // 例如: execute_bt_command(bt_cmd);
//        
//        // 清除中断标志
//        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
//    }
//}