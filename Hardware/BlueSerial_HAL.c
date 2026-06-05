/**
 * @file    BlueSerial_HAL.c
 * @brief   STM32F407VET6 HAL 库蓝牙串口驱动（由标准库 BlueSerial.c 移植）
 *
 * 芯片：STM32F407VET6
 * 串口：USART2  PA2(TX)  PA3(RX)  — 与原标准库工程引脚一致，接 HC-05/HC-06 等蓝牙模块
 * 波特率：9600 8N1（蓝牙模块默认波特率，需与模块 AT 配置一致）
 *
 * =========================================================================
 * 【CubeMX 自动生成部分 — 请勿在本文件重复编写，按下列流程在 CubeMX 中配置】
 * =========================================================================
 *
 * 1. 新建 STM32F407VET6 工程，SYS → Debug 选 Serial Wire。
 *
 * 2. Connectivity → USART2：
 *    - Mode: Asynchronous
 *    - Baud Rate: 9600
 *    - Word Length: 8 Bits
 *    - Parity: None
 *    - Stop Bits: 1
 *    - Hardware Flow Control: Disable
 *
 * 3. Pinout（F407 默认复用，CubeMX 会自动分配）：
 *    - PA2  → USART2_TX
 *    - PA3  → USART2_RX
 *
 * 4. NVIC → USART2 global interrupt：勾选 Enabled。
 *
 * 5. Project Manager → 勾选 "Generate peripheral initialization as a pair of
 *    '.c/.h' files per peripheral"（可选，便于管理）。
 *
 * 6. Generate Code 后，CubeMX 会自动生成：
 *    - Core/Src/usart.c        → MX_USART2_UART_Init()
 *    - Core/Inc/usart.h        → extern UART_HandleTypeDef huart2;
 *    - Core/Src/stm32f4xx_hal_msp.c → HAL_UART_MspInit()（GPIO 时钟、复用、NVIC）
 *    - Core/Src/stm32f4xx_it.c → USART2_IRQHandler() 中调用 HAL_UART_IRQHandler(&huart2)
 *
 * 7. Keil/IAR 工程设置（若使用标准 printf 重定向 fputc，见文件末尾注释）：
 *    - 勾选 Use MicroLIB（Keil）或配置 _write  syscall（GCC）
 *
 * =========================================================================
 * 【main.c 中需要添加的代码 — 复制到 HAL 工程 main 函数对应位置】
 * =========================================================================
 *
 *   #include "BlueSerial_HAL.h"
 *
 *   int main(void)
 *   {
 *       HAL_Init();
 *       SystemClock_Config();
 *       MX_GPIO_Init();
 *       MX_USART2_UART_Init();          // CubeMX 生成
 *       Blueserial_HAL_StartReceive();  // 启动单字节中断接收
 *
 *       while (1)
 *       {
 *           Blueserial_HAL_Printf("Angle=%.2f\r\n", angle);   // 调试打印示例
 *
 *           if (Blueserial_HAL_GetRxFlag())
 *           {
 *               // Blueserial_Rxpacket 中为 "[cmd,param]" 格式的字符串
 *               // 用 strtok 等解析，参考原工程 main.c
 *           }
 *       }
 *   }
 *
 * =========================================================================
 */

#include "BlueSerial_HAL.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* CubeMX 在 usart.c 中定义：UART_HandleTypeDef huart2; */
extern UART_HandleTypeDef huart2;

/* ======================== 模块变量 ======================== */
uint8_t  Blueserial_Txpacket[4];
char     Blueserial_Rxpacket[100];
uint8_t  Blueserial_Rxflag;

static uint8_t s_rx_byte;           /* 单字节接收缓冲，供 HAL_UART_Receive_IT 使用 */
static uint8_t s_rx_state  = 0;
static uint8_t s_rx_index  = 0;

/* ======================== 内部工具 ======================== */
static uint32_t Blueserial_Pow(uint32_t x, uint32_t y)
{
    uint32_t result = 1;
    while (y--)
    {
        result *= x;
    }
    return result;
}

static void Blueserial_RearmReceive(void)
{
    HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
}

/* ======================== 初始化 / 启动接收 ======================== */
void Blueserial_HAL_StartReceive(void)
{
    Blueserial_Rxflag = 0;
    s_rx_state = 0;
    s_rx_index = 0;
    Blueserial_RearmReceive();
}

/* ======================== 发送函数 ======================== */
void Blueserial_HAL_SendByte(uint8_t byte)
{
    HAL_UART_Transmit(&huart2, &byte, 1, HAL_MAX_DELAY);
}

void Blueserial_HAL_SendArray(uint8_t *array, uint16_t length)
{
    uint16_t i;
    for (i = 0; i < length; i++)
    {
        Blueserial_HAL_SendByte(array[i]);
    }
}

void Blueserial_HAL_SendString(char *string)
{
    uint8_t i;
    for (i = 0; string[i] != '\0'; i++)
    {
        Blueserial_HAL_SendByte(string[i]);
    }
}

void Blueserial_HAL_SendNum(uint32_t number, uint8_t length)
{
    uint8_t i;
    for (i = 0; i < length; i++)
    {
        Blueserial_HAL_SendByte((uint8_t)(number / Blueserial_Pow(10, length - i - 1) % 10 + '0'));
    }
}

/**
 * @brief  自定义格式化打印（与原 Blueserial_Printf 相同思路）
 *
 * 这不是 C 标准库 printf 的 fputc 重定向，而是：
 *   1. va_list 收集可变参数
 *   2. vsprintf 格式化为本地字符串
 *   3. 逐字节从蓝牙串口发出
 *
 * 优点：不影响 printf 默认输出；可指定从蓝牙口而非调试口打印。
 * 缺点：缓冲区固定 100 字节，超长格式可能溢出；不支持直接 printf("...") 调用。
 */
void Blueserial_HAL_Printf(char *format, ...)
{
    char string[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(string, format, arg);
    va_end(arg);
    Blueserial_HAL_SendString(string);
}

void Blueserial_HAL_SendPacket(void)
{
    Blueserial_HAL_SendByte(0xFF);
    Blueserial_HAL_SendArray(Blueserial_Txpacket, 4);
    Blueserial_HAL_SendByte(0xFE);
}

/* ======================== 接收标志 ======================== */
uint8_t Blueserial_HAL_GetRxFlag(void)
{
    if (Blueserial_Rxflag == 1)
    {
        Blueserial_Rxflag = 0;
        return 1;
    }
    return 0;
}

/* ======================== 接收状态机（与原 USART2_IRQHandler 逻辑一致） ======================== */
/**
 * @brief  处理单字节接收，帧格式：[ ... ]
 * @note   由 Blueserial_HAL_RxCpltCallback 调用；收到 ']' 后置 Blueserial_Rxflag
 */
static void Blueserial_ProcessRxByte(uint8_t rx_data)
{
    if (s_rx_state == 0)
    {
        if (rx_data == '[' && Blueserial_Rxflag == 0)
        {
            s_rx_state = 1;
            s_rx_index = 0;
        }
    }
    else if (s_rx_state == 1)
    {
        if (rx_data == ']')
        {
            s_rx_state = 0;
            Blueserial_Rxpacket[s_rx_index] = '\0';
            Blueserial_Rxflag = 1;
        }
        else
        {
            Blueserial_Rxpacket[s_rx_index] = (char)rx_data;
            s_rx_index++;
        }
    }
}

/**
 * @note  在 HAL 工程中，将下列函数体合并到 Core/Src/main.c 里已有的
 *        HAL_UART_RxCpltCallback() 中（或在本文件实现后于 main.c 用 weak 覆盖）：
 *
 *        void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 *        {
 *            Blueserial_HAL_RxCpltCallback(huart);
 *        }
 */
void Blueserial_HAL_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        Blueserial_ProcessRxByte(s_rx_byte);
        Blueserial_RearmReceive();
    }
}

/*
 * =========================================================================
 * 【可选】标准 printf 重定向 — 与原 serial.c 的 fputc 方法相同
 * =========================================================================
 *
 * 若希望直接写 printf("hello") 就从蓝牙口输出，取消下面注释，并在 Keil 勾选 MicroLIB：
 *
 * int fputc(int ch, FILE *f)
 * {
 *     (void)f;
 *     Blueserial_HAL_SendByte((uint8_t)ch);
 *     return ch;
 * }
 *
 * GCC + newlib 时使用 _write：
 *
 * int _write(int fd, char *ptr, int len)
 * {
 *     (void)fd;
 *     HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
 *     return len;
 * }
 *
 * 两种方法对比见聊天说明。
 * =========================================================================
 */
