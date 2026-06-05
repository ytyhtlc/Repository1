#ifndef __BLUESERIAL_H
#define __BLUESERIAL_H

void Blueserial_Init(void);
void Blueserial_sendbyte(uint8_t byte);
void Blueserial_sendarray(uint8_t *array,uint16_t length);
void Blueserial_sendstring(char *string);
uint32_t Blueserial_pow(uint32_t x,uint32_t y);
void Blueserial_sendnum(uint32_t number,uint8_t length);
void Blueserial_Printf(char *format, ...);
void Blueserial_sendpacket(void);
uint8_t Blueserial_getrxflag2(void);


extern uint8_t Blueserial_Txpacket[4];//存储载荷数据
extern char Blueserial_Rxpacket[100];
extern uint8_t Blueserial_Rxflag;



#endif
