#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "timer.h"
#include "ILI93xx.h"
#include "key.h"
#include "beep.h"
#include "servo.h"
#include "touch.h"
#include "contral.h"
#include "serial.h"
#include "string.h"
#include "malloc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "servo.h"
#include "semphr.h"
#include "serial.h"
#include "GUIDemo.h"
#include "limits.h"
#include "contral.h"
#include "ff.h"
#include "exfuns.h"
#include "sram.h"
#include "malloc.h"

//任务优先级
#define START_TASK_PRIO		1
//任务堆栈大小	
#define START_STK_SIZE 		256  
//任务句柄
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);

//任务优先级
#define CONTROLSERVO_TASK_PRIO 2
//任务堆栈大小	
#define CONTROLSERVO_STK_SIZE  512 
//任务句柄
TaskHandle_t ControlServo_Handler;
//任务函数
void controltask(void *pvParameters);

#define DISPLAY_TASK_PRIO 2
//任务堆栈大小	
#define DISPLAY_STK_SIZE  512 
//任务句柄
TaskHandle_t Display_Handler;
//任务函数
void displaytask(void *pvParameters);


//定义全局变量
PID_TypeDef PID_x, PID_y;//两个PID结构体PID_x和PID_y
KFP FilterX_Parameter, FilterY_Parameter; //两个卡尔曼滤波参数结构体FilterX_Parameter与FilterY_Parameter
extern int coords[2];	//存放原始坐标的数组
int AfterFliter[2];		//卡尔曼滤波后物体坐标x与y
//初始化舵机角度
u16 pwmval_x;
u16 pwmval_y;
FRESULT result; 

int main(void)
{ 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置系统中断优先级分组4
	uart_init(115200);     			//初始化串口
	delay_init(168);					//初始化延时函数
	LED_Init();		        			//初始化LED端口
	KEY_Init();							//初始化按键
	BEEP_Init();						//初始化蜂鸣器
	TFTLCD_Init();						//初始化LCD
	TP_Init();							//初始化触摸屏
	FSMC_SRAM_Init(); 					//SRAM初始化	
	TIM4_Int_Init(10000-1,144-1);		//双舵机控制定时器初始化
	mem_init(SRAMIN); 					//内部RAM初始化
	mem_init(SRAMEX); 					//外部RAM初始化
	mem_init(SRAMCCM);					//CCM初始化
	exfuns_init();			//为fatfs文件系统分配内存
	result = f_mount(fs[0],"0:",1);	//挂载SD卡
    //PID参数初始化
	pid_init(0.03, 0, 0.30, &PID_x);
	pid_init(0.03, 0, 0.30, &PID_y);
	//卡尔曼滤波参数初始化
	KFP_init(0.02,0,0,0,0.001,0.543,&FilterX_Parameter);
	KFP_init(0.02,0,0,0,0.001,0.543,&FilterY_Parameter);
	//初始化坐标值
	coords[0] = 320;
	coords[1] = 240;
	//初始化舵机角度
	pwmval_x = 650;
	pwmval_y = 650;
	//创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
}

//开始任务任务函数
void start_task(void *pvParameters)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC,ENABLE);//开启CRC时钟
	WM_SetCreateFlags(WM_CF_MEMDEV);
	GUI_Init();  					//STemWin初始化
	WM_MULTIBUF_Enable(1);  		//开启STemWin多缓冲,RGB屏可能会用到
    taskENTER_CRITICAL();           //进入临界区
	
	//创建DISAPLAY任务
    xTaskCreate((TaskFunction_t )displaytask,     
                (const char*    )"display_task",   
                (uint16_t       )DISPLAY_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )DISPLAY_TASK_PRIO,
                (TaskHandle_t*  )&Display_Handler); 
	//创建CONTRAL任务
    xTaskCreate((TaskFunction_t )controltask,     
                (const char*    )"control_task",   
                (uint16_t       )CONTROLSERVO_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )CONTROLSERVO_TASK_PRIO,
                (TaskHandle_t*  )&ControlServo_Handler); 
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}


void controltask(void *pvParameters)      //云台操作任务
{
	u32 NotifyValue;
	while(1)
	{
		NotifyValue=ulTaskNotifyTake(pdTRUE,portMAX_DELAY);	//获取任务通知,进入阻塞状态，直到数据接收完成、获得CPU使用权
		if(NotifyValue==1)									//清零之前的任务通知值为1，说明任务通知有效
		{
			LED0=!LED0;
			pwmval_x = pwmval_x + better_pid(AfterFliter[0],0,&PID_x);
			pwmval_y = pwmval_y + better_pid(AfterFliter[0],0,&PID_y);
			TIM_SetCompare1(TIM4,pwmval_x);     //X轴舵机控制
			TIM_SetCompare2(TIM4,pwmval_y);     //Y轴舵机控制
			vTaskDelay(500);
		}
		else
		{
			vTaskDelay(10);      //延时10ms，也就是10个时钟节拍	
		}
	}
}

void displaytask(void *pvParameters)     //显示任务
{
	WM_MULTIBUF_Enable(1);
	while(1)
	{
		MainTask();
	}
}

