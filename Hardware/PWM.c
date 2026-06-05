#include "stm32f10x.h"                  // Device header

void pwm_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能时钟
	
	
	GPIO_InitTypeDef GPIO_INITSTRUCTURE;//结构体 初始化GPIOA外设
	GPIO_INITSTRUCTURE.GPIO_Mode = GPIO_Mode_AF_PP;//复用推挽输出
	GPIO_INITSTRUCTURE.GPIO_Pin = GPIO_Pin_0 |GPIO_Pin_1;
	GPIO_INITSTRUCTURE.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_INITSTRUCTURE);
	
	TIM_InternalClockConfig(TIM2);//内部时钟源
	
	TIM_TimeBaseInitTypeDef TIM_timebaseInitstructure;//时基单元结构体
	TIM_timebaseInitstructure.TIM_ClockDivision = TIM_CKD_DIV1;//滤波采样分频
	TIM_timebaseInitstructure.TIM_CounterMode = TIM_CounterMode_Up;//计数模式，向上计数
	TIM_timebaseInitstructure.TIM_Period = 99;//ARR自动重装器的值
	TIM_timebaseInitstructure.TIM_Prescaler = 719;//预分频器的值
	TIM_timebaseInitstructure.TIM_RepetitionCounter = 0;//重复计数器的值
	TIM_TimeBaseInit(TIM2,&TIM_timebaseInitstructure);
	
	TIM_OCInitTypeDef TIM_OCInitstructure;//输出比较结构体
	TIM_OCStructInit(&TIM_OCInitstructure);//结构体赋初始值
	TIM_OCInitstructure.TIM_OCMode = TIM_OCMode_PWM1;//配置输出比较模式
	TIM_OCInitstructure.TIM_OCPolarity = TIM_OCPolarity_High;//配置极性
	TIM_OCInitstructure.TIM_OutputState = TIM_OutputState_Enable;//使能
	TIM_OCInitstructure.TIM_Pulse = 0;//CCR(占空比）
	
	TIM_OC1Init(TIM2,&TIM_OCInitstructure);
	TIM_OC2Init(TIM2,&TIM_OCInitstructure);

	TIM_Cmd(TIM2,ENABLE);
}

void PWM_SetCompare1(uint16_t compare)//函数套函数，外参数作为内参数进入函数运算，法一
{
	TIM_SetCompare1(TIM2,compare);
}

void PWM_SetCompare2(uint16_t compare)//函数套函数，外参数作为内参数进入函数运算，法一
{
	TIM_SetCompare2(TIM2,compare);
}
