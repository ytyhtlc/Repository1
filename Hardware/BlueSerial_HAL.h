/**
 * @file    BlueSerial_HAL.h
 * @brief   STM32F407VET6 HAL 库蓝牙串口驱动（由标准库 BlueSerial.c 移植）
 * @note    本文件为独立参考模块，不修改原工程；CubeMX 生成部分见 BlueSerial_HAL.c 顶部注释
 */

#ifndef __BLUESERIAL_HAL_H
#define __BLUESERIAL_HAL_H

#include "main.h"           /* CubeMX 生成，含 huart2 等句柄声明 */
#include <stdint.h>

/* ======================== 对外变量 ======================== */
extern uint8_t  Blueserial_Txpacket[4];   /* 待发送的二进制载荷（sendpacket 用） */
extern char     Blueserial_Rxpacket[100];   /* 接收到的字符串帧，格式 [xxx] */
extern uint8_t  Blueserial_Rxflag;          /* 收到完整帧后置 1 */

/* ======================== 初始化 ======================== */
/**
 * @brief  启动蓝牙串口接收（CubeMX 已完成 UART/GPIO/NVIC 初始化后调用）
 * @note   【main.c 中调用】放在 MX_USART2_UART_Init() 之后
 */
void Blueserial_HAL_StartReceive(void);

/* ======================== 发送接口 ======================== */
void Blueserial_HAL_SendByte(uint8_t byte);
void Blueserial_HAL_SendArray(uint8_t *array, uint16_t length);
void Blueserial_HAL_SendString(char *string);
void Blueserial_HAL_SendNum(uint32_t number, uint8_t length);
void Blueserial_HAL_Printf(char *format, ...);   /* 自定义格式化打印（vsprintf 封装，非 fputc 重定向） */
void Blueserial_HAL_SendPacket(void);

/* ======================== 接收接口 ======================== */
uint8_t Blueserial_HAL_GetRxFlag(void);          /* 读一次接收标志，读后自动清零 */

/* ======================== HAL 回调（由 HAL 库自动调用） ======================== */
/**
 * @note  无需在 main.c 中手动调用；实现体在 BlueSerial_HAL.c
 *        需在 stm32f4xx_it.c 的 USART2_IRQHandler 中保留 HAL_UART_IRQHandler(&huart2)
 */
void Blueserial_HAL_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* __BLUESERIAL_HAL_H */
