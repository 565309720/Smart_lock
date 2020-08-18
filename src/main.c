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

#define EC20_RES GPIO_Pin_5								//EC20复位引脚,B端口
#define Lock GPIO_Pin_3									//门锁控制叫,C端口
#define Send_message_now								//心跳包线程使用，记录当前已经发送消息数量

char Deviceerr;											//4G设备故障
int Hearbeat_err_time = 0;								//变量——心跳包线程中遇到错误尝试重发的次数
int message_send_num = 0;									//变量——心跳包线程中，保存当前已经发送的消息数量

void Hearbeat_package()
{
	while (1)
	{
		message_send_num = Send_message_num();
	}
}

void main(void)
{
	//Door lock control pin
	pinModeC(Lock, OUTPUT);
	digitalWriteC(Lock, HIGH);

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

	//操作系统初始化部分
//	xTaskCreate(Hearbeat_package, "Heat", 1024, NULL, 2, NULL); 		//心跳包线程
//	vTaskStartScheduler();												//开启多线程
	while (1)
	{
		send_cmd("AT+QISEND=0 \r\n");
		delay_us(100000);
		send_cmd("~ \r\n");
		delay_us(100000);
		message_send_num = Send_message_num();
//		digitalWriteC(Lock, LOW);
	}
}
