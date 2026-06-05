#ifndef __SERIAL_H
#define __SERIAL_H

extern uint8_t serial_Txpacket[];
extern uint8_t serial_Rxpacket[];
void serial_sendbyte(uint8_t byte);
void serial_Init(void);
void serial_sendarray(uint8_t *array,uint16_t length);
void serial_sendstring(char *string);
void serial_sendnum(uint32_t number,uint8_t length);
uint8_t serial_getrxflag(void);//封装接收标zhi位
void serial_sendpacket(void);



#endif