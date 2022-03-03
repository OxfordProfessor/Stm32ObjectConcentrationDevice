#include "serial.h"
#include "usart.h"
#include "led.h"
#include "delay.h"
#include "stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "malloc.h"
#include "string.h"

u16 t;  
u16 len;
u16 times=0; //不需要
int coords[2];
extern int AfterFliter[2];
extern KFP FilterX_Parameter, FilterY_Parameter; //两个卡尔曼滤波参数结构体FilterX_Parameter与FilterY_Parameter
extern TaskHandle_t ControlServo_Handler;  //舵机任务句柄

float kalmanFilter(KFP *kfp,float input)   //卡尔曼滤波
{
     //预测协方差方程：k时刻系统估算协方差 = k-1时刻的系统协方差 + 过程噪声协方差
     kfp->Now_P = kfp->LastP + kfp->Q;
     //卡尔曼增益方程：卡尔曼增益 = k时刻系统估算协方差 / （k时刻系统估算协方差 + 观测噪声协方差）
     kfp->Kg = kfp->Now_P / (kfp->Now_P + kfp->R);
     //更新最优值方程：k时刻状态变量的最优值 = 状态变量的预测值 + 卡尔曼增益 * （测量值 - 状态变量的预测值）
     kfp->out = kfp->out + kfp->Kg * (input -kfp->out);//因为这一次的预测值就是上一次的输出值
     //更新协方差方程: 本次的系统协方差付给 kfp->LastP 威下一次运算准备。
     kfp->LastP = (1-kfp->Kg) * kfp->Now_P;
     return kfp->out;
}

void KFP_init(float LastP, float Now_P, float out, float Kg, float Q, float R, KFP* KFP)
{
	KFP->LastP = LastP;
	KFP->Now_P = Now_P;
	KFP->out = out;
	KFP->Kg = Kg;
	KFP->Q = Q;
	KFP->R = R;
}

void receivedata()
{
	u8 flag=0;
	char strX[4], strY[4];
	u8 cnt_x=0;
	u8 cnt_y=0;
	u8 *adress = NULL;
	u8 *date;
	if(USART_RX_STA&0x8000)		//如果接收完数据
	{	
		LED1=!LED1;//1号指示灯变更状态
		len=USART_RX_STA&0x3fff;//得到此次接收到的数据长度
		date = mymalloc(SRAMIN,len+1);  //申请内存
		sprintf((char*)date,"%s",USART_RX_BUF);
		adress = &date[0];	//指针adress储存字符地址，从0-len过一遍	
		//根据协议取出坐标的字符形式到数strX和strY中
		while(t<len)				//用while循环代替for循环，速度快3倍，否则容易造成死机
		{
			t++;
			if(*adress>='0' && *adress<='9')
			{
				if(flag==1)
				{
					strX[cnt_x] = *adress;
					cnt_x++;
				}
				else
				{
					strY[cnt_y] = *adress;
					cnt_y++;
				}	
			}
				else
				{
					if(*adress=='#')
						flag = 1;
					if(*adress=='$')
						flag = 2;
				}
				adress++;	
		}
			//转换字符串为整型，并存储到全局变量coords中
			coords[0] = atoi(strX);
			coords[1] = atoi(strY);
			AfterFliter[0] = kalmanFilter(&FilterX_Parameter,coords[0]);		//X轴滤波坐标
			AfterFliter[1] = kalmanFilter(&FilterX_Parameter,coords[1]);		//Y轴滤波坐标
			//标志位清零
			USART_RX_STA=0;
			memset(USART_RX_BUF,0,USART_REC_LEN);			//串口接收缓冲区清零
			myfree(SRAMIN,date);						//释放内存
			xTaskNotifyGive(ControlServo_Handler);	//向舵机任务发送任务通知，解除其阻塞状态
	}
//		else
//		{
//			vTaskDelay(10);             	//延时10ms，也就是10个时钟节拍	; 
//		} 		 
}