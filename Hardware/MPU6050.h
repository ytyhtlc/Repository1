#ifndef __MPU6050_H
#define __MPU6050_H
void MPU6050_RegWrite(uint8_t RegAddress,uint8_t Data);
uint8_t MPU6050_RegRead(uint8_t RegAddress);
void MPU6050_Init(void);
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);
void MPU6050_RegReads(uint8_t RegAddress,uint8_t *DataArray,uint8_t count);

#endif