#include "stm32f10x.h"                  // Device header
#include "MYIIC.h"
#include "MPU6050_Reg.h"

#define MPU6050_Address		0xD0

void MPU6050_RegWrite(uint8_t RegAddress,uint8_t Data)
{
	MyI2C_start();
	MyI2C_SendByte(MPU6050_Address);
	MyI2C_RecieveAck();
	MyI2C_SendByte(RegAddress);
	MyI2C_RecieveAck();
	MyI2C_SendByte(Data);
	MyI2C_RecieveAck();
	MyI2C_stop();
}

uint8_t MPU6050_RegRead(uint8_t RegAddress)
{
	uint8_t Data;
	MyI2C_start();
	MyI2C_SendByte(MPU6050_Address);
	MyI2C_RecieveAck();
	MyI2C_SendByte(RegAddress);
	MyI2C_RecieveAck();
	
	MyI2C_start();
	MyI2C_SendByte(MPU6050_Address | 0x01);
	MyI2C_RecieveAck();
	Data = MyI2C_RecieveByte();
	MyI2C_SendAck(1);
	MyI2C_stop();
	return Data;
}

void MPU6050_RegReads(uint8_t RegAddress,uint8_t *DataArray,uint8_t count)
{
	uint8_t i;
	MyI2C_start();
	MyI2C_SendByte(MPU6050_Address);
	MyI2C_RecieveAck();
	MyI2C_SendByte(RegAddress);
	MyI2C_RecieveAck();
	
	MyI2C_start();
	MyI2C_SendByte(MPU6050_Address | 0x01);
	MyI2C_RecieveAck();
	for(i = 0;i<count;i++)
	{
		DataArray[i] = MyI2C_RecieveByte();
		if(i<count-1)
		{
			MyI2C_SendAck(0);
		}
		else 
		{
			MyI2C_SendAck(1);
		}
	}
	MyI2C_stop();

}

void MPU6050_Init(void)
{
	MyI2C_Init();									//先初始化底层的I2C
	
	/*MPU6050寄存器初始化，需要对照MPU6050手册的寄存器描述配置，此处仅配置了部分重要的寄存器*/
	MPU6050_RegWrite(MPU6050_PWR_MGMT_1, 0x01);		//电源管理寄存器1，取消睡眠模式，选择时钟源为X轴陀螺仪
	MPU6050_RegWrite(MPU6050_PWR_MGMT_2, 0x00);		//电源管理寄存器2，保持默认值0，所有轴均不待机
	MPU6050_RegWrite(MPU6050_SMPLRT_DIV, 0x07);		//采样率分频寄存器，配置采样率
	MPU6050_RegWrite(MPU6050_CONFIG, 0x00);			//配置寄存器，配置DLPF
	MPU6050_RegWrite(MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪配置寄存器，选择满量程为±2000°/s
	MPU6050_RegWrite(MPU6050_ACCEL_CONFIG, 0x18);	//加速度计配置寄存器，选择满量程为±16g
}

//void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
//						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
//{
//	uint8_t DataH, DataL;								//定义数据高8位和低8位的变量
//	
//	DataH = MPU6050_RegRead(MPU6050_ACCEL_XOUT_H);		//读取加速度计X轴的高8位数据
//	DataL = MPU6050_RegRead(MPU6050_ACCEL_XOUT_L);		//读取加速度计X轴的低8位数据
//	*AccX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_RegRead(MPU6050_ACCEL_YOUT_H);		//读取加速度计Y轴的高8位数据
//	DataL = MPU6050_RegRead(MPU6050_ACCEL_YOUT_L);		//读取加速度计Y轴的低8位数据
//	*AccY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_RegRead(MPU6050_ACCEL_ZOUT_H);		//读取加速度计Z轴的高8位数据
//	DataL = MPU6050_RegRead(MPU6050_ACCEL_ZOUT_L);		//读取加速度计Z轴的低8位数据
//	*AccZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_RegRead(MPU6050_GYRO_XOUT_H);		//读取陀螺仪X轴的高8位数据
//	DataL = MPU6050_RegRead(MPU6050_GYRO_XOUT_L);		//读取陀螺仪X轴的低8位数据
//	*GyroX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_RegRead(MPU6050_GYRO_YOUT_H);		//读取陀螺仪Y轴的高8位数据
//	DataL = MPU6050_RegRead(MPU6050_GYRO_YOUT_L);		//读取陀螺仪Y轴的低8位数据
//	*GyroY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//	
//	DataH = MPU6050_RegRead(MPU6050_GYRO_ZOUT_H);		//读取陀螺仪Z轴的高8位数据
//	DataL = MPU6050_RegRead(MPU6050_GYRO_ZOUT_L);		//读取陀螺仪Z轴的低8位数据
//	*GyroZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
//}

void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t Data[14];
	
	MPU6050_RegReads(MPU6050_ACCEL_XOUT_H,Data,14);
	
	*AccX = Data[0]<<8 | Data[1];
	*AccY = Data[2]<<8 | Data[3];
	*AccZ = Data[4]<<8 | Data[5];
	
	*GyroX = Data[8]<<8 | Data[9];
	*GyroY = Data[10]<<8 | Data[11];
	*GyroZ = Data[12]<<8 | Data[13];
	
}
