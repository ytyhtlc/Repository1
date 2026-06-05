#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "MyIIC.h"
#include "MPU6050.h"
#include "timer.h"
#include "motor.h"
#include "PWM.h"
#include "encoder.h"
#include "serial.h"
#include "BlueSerial.h"
#include "math.h"
#include "PID.h"
#include "string.h"
#include "stdlib.h"
#include "NRF24L01.h"
#include "TCRT5000.h"

void BlueTest(PID_t *AnglePID,PID_t *SpeedPID,PID_t *TurnPID);
//void XUN_Reaction(void);

int16_t AX, AY, AZ, GX, GY, GZ;
float AngleACC,AngleGyro,Angle,Alpha;
uint8_t Runflag = 1;
int16_t RightPWM,LeftPWM;
int16_t AvePWM,DifPWM;

float Rightspeed,Leftspeed;
float Avespeed,Difspeed;

PID_t AnglePID = {
	.Kp = 2.95,//2.95,
	.Ki = 0.17,//0.17,
	.Kd = 5.38,//5.38,
	.OutMax = 100,
	.OutMin = -100,
	.plus = 2.14,
	.Target = 0
};

PID_t SpeedPID = {
	.Kp = 0.83,//0.83
	.Ki = 0.11,//0.11
	.Kd = 0.139,
	.OutMax = 15,
	.OutMin = -15,
	.Target = 0
};

PID_t TurnPID = {
	.Kp = 3.94,
	.Ki = 1.16,
	.Kd = 0,
	.OutMax = 40,
	.OutMin = -40
};

int main(void)
{
//	TCRT5000_Init();
	Blueserial_Init();
	MPU6050_Init();	
	OLED_Init();
	timer_Init();
	motor_Init();
	encoder_Init();
	serial_Init();
//	NRF24L01_Init();
	
	
	Alpha = 0.001;
	while(1)
	{
		BlueTest(&AnglePID,&SpeedPID,&TurnPID);
		
//		signal_Get();
//		XUN_Reaction();
//		
//		if(NRF24L01_Receive() == 1)
//		{
//			OLED_ShowNum(1,1,NRF24L01_RxPacket[0],4);
//		}
		
		OLED_ShowSignedNum(1,1,AnglePID.Actual,5);
		OLED_ShowSignedNum(2,1,AnglePID.Target,5);
		OLED_ShowSignedNum(3,1,SpeedPID.Actual,5);
		OLED_ShowSignedNum(4,1,SpeedPID.Target,5);
//		Blueserial_Printf("[plot,%f,%f]",AnglePID.Target,AnglePID.Actual);
	}
}








void BlueTest(PID_t *AnglePID,PID_t *SpeedPID,PID_t *TurnPID)
{
	if (Blueserial_Rxflag == 1)
		{
			
			char *Tag = strtok(Blueserial_Rxpacket, ",");
			if (strcmp(Tag, "key") == 0)
			{
				char *Name = strtok(NULL, ",");
				char *Action = strtok(NULL, ",");
				
				
			}
			else if (strcmp(Tag, "slider") == 0)
			{
				char *Name = strtok(NULL, ",");
				char *Value = strtok(NULL, ",");
				
				if(strcmp(Name,"AngleKp") == 0)
				{
					AnglePID->Kp = atof(Value);
				}
				else if(strcmp(Name,"AngleKi") == 0)
				{
					AnglePID->Ki = atof(Value);
				}
				else if(strcmp(Name,"AngleKd") == 0)
				{
					AnglePID->Kd = atof(Value);
				}
				else if(strcmp(Name,"Angleplus") == 0)
				{
					AnglePID->plus = atof(Value);
				}
				else if(strcmp(Name,"SpeedKp") == 0)
				{
					SpeedPID->Kp = atof(Value);
				}
				else if(strcmp(Name,"SpeedKi") == 0)
				{
					SpeedPID->Ki = atof(Value);
				}
				else if(strcmp(Name,"SpeedKd") == 0)
				{
					SpeedPID->Kd = atof(Value);
				}
				else if(strcmp(Name,"TurnKp") == 0)
				{
					TurnPID->Kp = atof(Value);
				}
				else if(strcmp(Name,"TurnKi") == 0)
				{
					TurnPID->Ki = atof(Value);
				}
				else if(strcmp(Name,"TurnKd") == 0)
				{
					TurnPID->Kd = atof(Value);
				}
				
			}
			else if (strcmp(Tag, "joystick") == 0)
			{
				int8_t LH = atoi(strtok(NULL, ","));
				int8_t LV = atoi(strtok(NULL, ","));
				int8_t RH = atoi(strtok(NULL, ","));
				int8_t RV = atoi(strtok(NULL, ","));
				
				SpeedPID->Target = LV/25.0;
				TurnPID->Target = RH/25.0;
			}
			
			Blueserial_Rxflag = 0;
		}
}


//void XUN_Reaction(void)
//{
//	if((signal_X1()==0)&&(signal_X2()==0))
//	{
//		motor_setspeedR(RightPWM);
//		motor_setspeedL(LeftPWM);
//		SpeedPID.Target = 0.8;
//	}
//	else if((signal_X1()==1)&&(signal_X2()==0))
//	{
//		
//		motor_setspeedR(RightPWM+14);
//	}
//	else if((signal_X1()==0)&&(signal_X2()==1))
//	{
//		motor_setspeedL(LeftPWM+14);
//	}
//}


void TIM1_UP_IRQHandler(void)
{
	MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);
	static uint16_t count1,count0;
	if(TIM_GetITStatus(TIM1,TIM_IT_Update) == SET)
	{
		AngleACC = atan2(AX,AZ)/3.14159*180;
		
		AngleGyro = Angle+ (-GY/32768.0*2000*0.001);
		
		Angle = Alpha*AngleACC + (1-Alpha)*AngleGyro;
		
		if(Angle > 45 || Angle <-45){Runflag = 0;}	
		
		count1++;
		count0++;
		if(count0 >=10)
		{
			count0 = 0;
			
			if(Runflag == 1)
			{
				
				AnglePID.Actual = Angle-1.0;
				PID_Update(&AnglePID);
				AvePWM = AnglePID.Out;
				
				RightPWM = AvePWM -DifPWM/2;
				LeftPWM =  AvePWM +DifPWM/2;
				
				if(RightPWM >=90){RightPWM =0;Runflag = 0;}
				else if(RightPWM <= -90){RightPWM = 0;Runflag = 0;}
				
				if(LeftPWM >= 90){LeftPWM =0;Runflag = 0;}
				else if(LeftPWM <= -90){LeftPWM = 0;Runflag = 0;}
				
				motor_setspeedL(LeftPWM);
				motor_setspeedR(RightPWM);
			}
			else
			{
				motor_setspeedL(0);
				motor_setspeedR(0);
			}
		}
		if(count1 >= 50)
		{
			count1 = 0;
			Leftspeed = Encoder_Get(1)/44.0/0.05/20;
			Rightspeed = Encoder_Get(2)/44.0/0.05/20;
			
			Avespeed = (Leftspeed+Rightspeed)/2.0;
			Difspeed = Leftspeed - Rightspeed;
			
			if(Runflag == 1)
			{
				SpeedPID.Actual = Avespeed;//转每秒
				PID_Update(&SpeedPID);
				AnglePID.Target = -SpeedPID.Out;
				
				TurnPID.Actual = Difspeed;
				PID_Update(&TurnPID);
				DifPWM = TurnPID.Out;
			}
		}
		
	}
	TIM_ClearITPendingBit(TIM1,TIM_IT_Update);//清除标志位
	
}