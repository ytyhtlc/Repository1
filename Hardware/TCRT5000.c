#include "stm32f10x.h"                  // Device header
#include "motor.h"



#define TCRT_PORT_PINS			GPIO_Pin_4 |GPIO_Pin_5
#define TCRT_PORT				GPIOA
#define TCRT_PORT_PIN1			GPIO_Pin_4
#define TCRT_PORT_PIN2			GPIO_Pin_5



void TCRT5000_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能时钟
	
	GPIO_InitTypeDef GPIO_INITSTRUCTURE;//结构体 初始化GPIOA外设
	GPIO_INITSTRUCTURE.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_INITSTRUCTURE.GPIO_Pin = TCRT_PORT_PINS;
	GPIO_INITSTRUCTURE.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(TCRT_PORT,&GPIO_INITSTRUCTURE);
}

uint8_t signal_X1(void)
{
	return GPIO_ReadInputDataBit(TCRT_PORT,TCRT_PORT_PIN1);
}

uint8_t signal_X2(void)
{
	return GPIO_ReadInputDataBit(TCRT_PORT,TCRT_PORT_PIN2);
}


void signal_Get(void)
{
	signal_X1();
	signal_X2();

}
