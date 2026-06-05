#include "stm32f10x.h"// Device header
#include <stdio.h>
#include <stdarg.h>

uint8_t serial_Txpacket[4];//存储载荷数据
uint8_t serial_Rxpacket[4];
uint8_t serial_Rxflag;


void serial_Init(void)
{
	//开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	//配置复用推挽输出TX
	GPIO_InitTypeDef GPIO_INITSTRUCTURE;//结构体 初始化GPIOA外设
	GPIO_INITSTRUCTURE.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_INITSTRUCTURE.GPIO_Pin = GPIO_Pin_9;
	GPIO_INITSTRUCTURE.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_INITSTRUCTURE);
	//配置上拉输入RX
	GPIO_INITSTRUCTURE.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_INITSTRUCTURE.GPIO_Pin = GPIO_Pin_10;
	GPIO_INITSTRUCTURE.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_INITSTRUCTURE);
	//USART结构体配置
	USART_InitTypeDef USART_Initstructure;
	USART_Initstructure.USART_BaudRate = 9600;//波特率
	USART_Initstructure.USART_WordLength = USART_WordLength_8b;//字长
	USART_Initstructure.USART_StopBits = USART_StopBits_1;//停止位
	USART_Initstructure.USART_HardwareFlowControl =USART_HardwareFlowControl_None ;//硬件流控
	USART_Initstructure.USART_Mode = USART_Mode_Tx|USART_Mode_Rx;//发送或接受模式
	USART_Initstructure.USART_Parity = USART_Parity_No;//校验位
	USART_Init(USART1,&USART_Initstructure);
	
	//USART中断控制
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	
	//配置NVIC
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//NVIC优先级分组
	NVIC_InitTypeDef NVIC_Initstructure;
	NVIC_Initstructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_Initstructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Initstructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_Initstructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_Initstructure);
	
	USART_Cmd(USART1,ENABLE);
	

}



void serial_sendbyte(uint8_t byte)//f发送单个字节函数
{
	USART_SendData(USART1,byte);
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);//获取标志位
}


void serial_sendarray(uint8_t *array,uint16_t length)//发送数组
{
	uint16_t i;
	for(i=0;i<length;i++)
	{
		serial_sendbyte(array[i]);
	}
}

void serial_sendstring(char *string)//发送字符串，自带结束位，不用给长度
{
	uint8_t i;
	for(i=0;string[i] != '\0';i++)
	{
		serial_sendbyte(string[i]);
	}
}


uint32_t serial_pow(uint32_t x,uint32_t y)
{
	uint32_t result = 1;
	while(y --)
	{
		result *=x;
	}
	return result;
}

void serial_sendnum(uint32_t number,uint8_t length)//发送数字串
{
	uint8_t i;
	for(i=0;i<length ;i++)
	{
		serial_sendbyte(number/serial_pow(10,length -i -1) %10+'0');
	}
}


int fputc(int ch,FILE *f)//重定向printf到串口发送
{
	serial_sendbyte(ch);
	return ch;
}

void serial_sendpacket(void)//发送数据包
{
	serial_sendbyte(0xFF);
	serial_sendarray(serial_Txpacket,4);
	serial_sendbyte(0xFE);
}

uint8_t serial_getrxflag(void)//封装接收标准位
{
	if(serial_Rxflag == 1)
	{
		serial_Rxflag =0;
		return 1;
	}
	return 0;
}


void USART1_IRQHandler(void)//中断函数
{
	static uint8_t Rxstate = 0;
	static uint8_t pRxpacket =0;
	if(USART_GetFlagStatus(USART1,USART_IT_RXNE)==SET)
	{
		uint8_t Rxdata = USART_ReceiveData(USART1);
		switch(Rxstate)
		{
			case 0:
				if(Rxdata == 0xFF)
				{
					Rxstate = 1;
					pRxpacket = 0;
				}
				break;
			case 1:
				serial_Rxpacket[pRxpacket] = Rxdata;
				pRxpacket ++;
				if(pRxpacket>=4)
				{
					Rxstate =2;
				}
				break;
			case 2:
				if(Rxdata == 0xFE)
				{
					Rxstate = 0;
					serial_Rxflag = 1;
				}
				break;
		}
	}
	USART_ClearITPendingBit(USART1,USART_IT_RXNE);
	
}



