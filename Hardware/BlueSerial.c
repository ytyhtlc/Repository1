#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <stdarg.h>
#include "string.h"
#include "stdlib.h"
#include "PID.h"

uint8_t Blueserial_Txpacket[4];//存储载荷数据
char Blueserial_Rxpacket[100];
uint8_t Blueserial_Rxflag;

void Blueserial_Init(void)
{
	//开启时钟
	
//蓝牙串口配置
	GPIO_InitTypeDef GPIO_InitStructure;//定义GPIO结构体
	USART_InitTypeDef USART2_InitStructure;//定义串口结构体
	NVIC_InitTypeDef NVIC_InitStructure;//定义优先级结构体
	//使能时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能GPIOA
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 ,ENABLE);//使能串口3
	//发送TX
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_2;//PA2
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//根据GY-33响应频率10hz
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;//复用推挽输出
	GPIO_Init(GPIOA,&GPIO_InitStructure);//初始化GPIOB
	//接收RX
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_3;//PA3
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//
	GPIO_Init(GPIOA,&GPIO_InitStructure);//初始化GPIOA
	//串口
	USART2_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;//不使用硬件
	USART2_InitStructure.USART_BaudRate=9600;//波特率
	USART2_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;//模式
	USART2_InitStructure.USART_StopBits=USART_StopBits_1;//一个停止位
	USART2_InitStructure.USART_Parity=USART_Parity_No;//奇偶校验关闭
	USART2_InitStructure.USART_WordLength=USART_WordLength_8b;//字长
	USART_Init(USART2,&USART2_InitStructure);//初始化串口3
	//优先级配置
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitStructure.NVIC_IRQChannel=USART2_IRQn;//串口3中断
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//使能串口3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=2;//响应优先级
	NVIC_Init(&NVIC_InitStructure);//初始化优先级
	//使能
	USART_Cmd(USART2,ENABLE);//使能串口2
	USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);//使能接收缓存非空寄存器
}




void Blueserial_sendbyte(uint8_t byte)//f发送单个字节函数
{
	USART_SendData(USART2,byte);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET);//获取标志位
}

void Blueserial_sendarray(uint8_t *array,uint16_t length)//发送数组
{
	uint16_t i;
	for(i=0;i<length;i++)
	{
		Blueserial_sendbyte(array[i]);
	}
}

void Blueserial_sendstring(char *string)//发送字符串，自带结束位，不用给长度
{
	uint8_t i;
	for(i=0;string[i] != '\0';i++)
	{
		Blueserial_sendbyte(string[i]);
	}
}

uint32_t Blueserial_pow(uint32_t x,uint32_t y)
{
	uint32_t result = 1;
	while(y --)
	{
		result *=x;
	}
	return result;
}

void Blueserial_sendnum(uint32_t number,uint8_t length)//发送数字串
{
	uint8_t i;
	for(i=0;i<length ;i++)
	{
		Blueserial_sendbyte(number/Blueserial_pow(10,length -i -1) %10+'0');
	}
}


void Blueserial_Printf(char *format, ...)
{
	char String[100];				//定义字符数组
	va_list arg;					//定义可变参数列表数据类型的变量arg
	va_start(arg, format);			//从format开始，接收参数列表到arg变量
	vsprintf(String, format, arg);	//使用vsprintf打印格式化字符串和参数列表到字符数组中
	va_end(arg);					//结束变量arg
	Blueserial_sendstring(String);		//串口发送字符数组（字符串）
}

void Blueserial_sendpacket(void)//发送数据包
{
	Blueserial_sendbyte(0xFF);
	Blueserial_sendarray(Blueserial_Txpacket,4);
	Blueserial_sendbyte(0xFE);
}



uint8_t Blueserial_getrxflag2(void)//封装接收标准位
{
	if(Blueserial_Rxflag == 1)
	{
		Blueserial_Rxflag =0;
		return 1;
	}
	return 0;
}



void USART2_IRQHandler(void)
{
	static uint8_t RxState = 0;
	static uint8_t pRxPacket = 0;
	if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
	{
		uint8_t RxData = USART_ReceiveData(USART2);
		
		if (RxState == 0)
		{
			if (RxData == '[' && Blueserial_Rxflag == 0)
			{
				RxState = 1;
				pRxPacket = 0;
			}
		}
		else if (RxState == 1)
		{
			if (RxData == ']')
			{
				RxState = 0;
				Blueserial_Rxpacket[pRxPacket] = '\0';
				Blueserial_Rxflag = 1;
			}
			else
			{
				Blueserial_Rxpacket[pRxPacket] = RxData;
				pRxPacket ++;
			}
		}
		
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}
}

