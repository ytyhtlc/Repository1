#include "stm32f10x.h"                  // Device header
#include "PWM.h"

void motor_Init(void)
{
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使能时钟
	GPIO_InitTypeDef GPIO_INITSTRUCTURE;//结构体 初始化GPIOA外设
	GPIO_INITSTRUCTURE.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_INITSTRUCTURE.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13 | GPIO_Pin_14|GPIO_Pin_15;
	GPIO_INITSTRUCTURE.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_INITSTRUCTURE);

	pwm_Init();
	
}
void  motor_setspeedL(int8_t speed)//右轮
{
	if(speed>=0){
		GPIO_SetBits(GPIOB,GPIO_Pin_12);
		GPIO_ResetBits(GPIOB,GPIO_Pin_13);
		PWM_SetCompare1(speed);
	}
	else{
		GPIO_SetBits(GPIOB,GPIO_Pin_13);
		GPIO_ResetBits(GPIOB,GPIO_Pin_12);
		PWM_SetCompare1(-speed);
	}
}

void  motor_setspeedR(int8_t speed)//左轮
{
	if(speed>=0){
		GPIO_SetBits(GPIOB,GPIO_Pin_15);
		GPIO_ResetBits(GPIOB,GPIO_Pin_14);
		PWM_SetCompare2(speed);
	}
	else{
		GPIO_SetBits(GPIOB,GPIO_Pin_14);
		GPIO_ResetBits(GPIOB,GPIO_Pin_15);
		PWM_SetCompare2(-speed);
	}
}