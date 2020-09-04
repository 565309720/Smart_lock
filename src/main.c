#include "Gen/GenLib.h"
#include "stm32f10x.h"
#include <stdio.h>
#include <stddef.h>
#include "UserConfig.h"//用户库调用
#include "IOT/IOT.h"

/*操作系统库函数调用*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#define EC20_RES   GPIO_Pin_5							//EC20复位引脚,B端口
#define Lock    GPIO_Pin_3								//门锁控制叫,C端口
//#define Send_message_now								//心跳包线程使用,记录当前已经发送消息数量
#define Headbeat_time 15								//常量——心跳包线程使用,设置心跳包发送间隔

//锁的高低位地址
char Lock_addH = 0x00;
char Lock_addL = 0x01;

char Deviceerr;											//4G设备故障
int Hearbeat_err_time = 0;								//变量——心跳包线程中遇到错误尝试重发的次数
int message_send_num = 0;								//变量——心跳包线程中，保存当前已经发送的消息数量
int Headbeat = 0;										//变量——心跳包线程中，用来判断发送时间
//char Random_code =0;									//变量——心跳包线程中，记录当前随机码

void time_break_function()
{
	Headbeat++;
}

void Hearbeat_package()
{
	while (1)
	{
		if (Headbeat > Headbeat_time)
		{
//			Random_code=rand();											//获得随机数
			message_send_num = Send_message_num();						//已经发送的字节数，用于判断是否发送成功
			Send_HearBeat_package(Door_lock, Lock_addH, Lock_addL);
			delay_us(100000);
			if (message_send_num >= Send_message_num())
			{
				IOT_Init(1);
			}
			Headbeat = 0;
			message_send_num = 0;
		}
		vTaskDelay(1500);
	}
}

void Receive_monitoring()
{
	while (1)
	{
		send_cmd("AT+QIRD=0,1500 \r\n");
		delay_us(10000);
		Instruction_received();
		vTaskDelay(500);
	}
}

void main(void)
{
	//Door lock control pin
	pinModeC(Lock, OUTPUT);
	digitalWriteC(Lock, HIGH);

	//开启定时器
	RTC_1s_it_init();
	RTC_Handler(time_break_function);
	RTC_IT_ON;

	//4G初始化部分
	pinModeB(EC20_RES, OUTPUT);
	digitalWriteB(EC20_RES, LOW);
	delay_us(300000);						//0.3s
	digitalWriteB(EC20_RES, HIGH);
	delay_us(12000000);
	usart_1_init(115200);
	delay_us(50000); 		//延时0.05s
	printf("AT+QIACT=1 \r\n");		 //设置一次即可，用于ping网检测使用
	delay_us(50000); 		//延时0.05s
	printf("ATE0 \r\n"); 		//设置一次即可，用于ping网检测使用
	delay_us(50000); 		//延时0.05s
	IOT_Init(1);			//连接服务器 ，延时使用delay_us

	//锁初始化
	pinModeC(GPIO_Pin_14, OUTPUT);
	Control_lock(Door_lock);											//门上锁

	//操作系统初始化部分
	xTaskCreate(Hearbeat_package, "Heat", 1048, NULL, 3, NULL); 		//心跳包线程
	xTaskCreate(Receive_monitoring, "Contor", 1024, NULL, 2, NULL); 		//接收监视线程
	vTaskStartScheduler();												//开启多线程

	while (1)
	{
//		send_cmd("AT+QISEND=0 \r\n");
//		delay_us(100000);
//		send_cmd("~ \r\n");
//		delay_us(100000);
//		message_send_num = Send_message_num();
//		digitalWriteC(Lock, LOW);
		delay_us(50000);
	}
}
