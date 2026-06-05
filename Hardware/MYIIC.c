#include "stm32f10x.h"                  // Device header
#include "Delay.h"

void MyI2C_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef GPIO_structureInit;
	GPIO_structureInit.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_structureInit.GPIO_Pin = GPIO_Pin_10 |GPIO_Pin_11;
	GPIO_structureInit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_structureInit);
	
	GPIO_SetBits(GPIOB,GPIO_Pin_10 |GPIO_Pin_11);
}

void MyI2C_W_SCL(uint8_t bitvalue)
{
	GPIO_WriteBit(GPIOB,GPIO_Pin_10,(BitAction)bitvalue);
	//Delay_us(10);
}

void MyI2C_W_SDA(uint8_t bitvalue)
{
	GPIO_WriteBit(GPIOB,GPIO_Pin_11,(BitAction)bitvalue);
	//Delay_us(10);
}

uint8_t MyI2C_R_SDA(void)
{
	uint8_t bitvalue;
	bitvalue = GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11);
	//Delay_us(10);
	return bitvalue;
}
void MyI2C_start(void)
{
	MyI2C_W_SDA(1);
	MyI2C_W_SCL(1);
	MyI2C_W_SDA(0);
	MyI2C_W_SCL(0);
}
void MyI2C_stop(void)
{
	MyI2C_W_SDA(0);
	MyI2C_W_SCL(1);
	MyI2C_W_SDA(1);
}

void MyI2C_SendByte(uint8_t byte)
{
	uint8_t i;
	for(i = 0;i<8;i++)
	{
		MyI2C_W_SDA(byte &(0x80 >>i));
		MyI2C_W_SCL(1);
		MyI2C_W_SCL(0);
	}
}
uint8_t  MyI2C_RecieveByte(void)
{
	MyI2C_W_SDA(1);
	uint8_t i;
	uint8_t byte = 0x00;
	for(i = 0; i<8;i++)
	{
		MyI2C_W_SCL(1);
		if(MyI2C_R_SDA()){byte |= (0x80 >> i);}//高位先行，位运算获取字节
		MyI2C_W_SCL(0);
	}
	
	return byte;
}

void MyI2C_SendAck(uint8_t AckBit)
{
	MyI2C_W_SDA(AckBit);
	MyI2C_W_SCL(1);
	MyI2C_W_SCL(0);			
	
}
uint8_t  MyI2C_RecieveAck(void)
{
	uint8_t AckBit;
	MyI2C_W_SDA(1);
	MyI2C_W_SCL(1);
	AckBit = MyI2C_R_SDA();
	MyI2C_W_SCL(0);
	return AckBit;
}








