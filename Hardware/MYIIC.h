#ifndef __MYIIC_H
#define __MYIIC_H

void MyI2C_Init(void);
void MyI2C_W_SCL(uint8_t bitvalue);
void MyI2C_W_SDA(uint8_t bitvalue);
uint8_t MyI2C_R_SDA(void);
void MyI2C_start(void);
void MyI2C_stop(void);
void MyI2C_SendByte(uint8_t byte);
uint8_t  MyI2C_RecieveByte(void);
void MyI2C_SendAck(uint8_t AckBit);
uint8_t  MyI2C_RecieveAck(void);
#endif