#include "stm32f10x.h"  

//volatile uint8_t Num;
uint8_t num;
void timer_Init(void)
{
	//开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);
	//选择内部时钟源
	TIM_InternalClockConfig(TIM1);
	//时基单元配置
	TIM_TimeBaseInitTypeDef TIM_timebaseInitstructure;//时基单元结构体
	TIM_timebaseInitstructure.TIM_ClockDivision = TIM_CKD_DIV1;//滤波采样分频（内部时钟可以不管）
	TIM_timebaseInitstructure.TIM_CounterMode = TIM_CounterMode_Up;//计数模式，向上计数
	TIM_timebaseInitstructure.TIM_Period = 99;//ARR自动重装器的值
	TIM_timebaseInitstructure.TIM_Prescaler = 719;//预分频器的值
	TIM_timebaseInitstructure.TIM_RepetitionCounter = 0;//重复计数器的值（高级计数器才有）
	TIM_TimeBaseInit(TIM1,&TIM_timebaseInitstructure);
	//清除更新中断标志位，避免刚初始化就进中断
	TIM_ClearFlag(TIM1,TIM_FLAG_Update);
	//中断控制，开启更新中断到NVIC的通路
	TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE);
	//NVIC配置
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//NVIC优先级分组
	NVIC_InitTypeDef NVIC_Initstructure;
	NVIC_Initstructure.NVIC_IRQChannel = TIM1_UP_IRQn;
	NVIC_Initstructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Initstructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_Initstructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_Initstructure);
	//启动控制
	TIM_Cmd(TIM1,ENABLE);
	
}	
//中断函数