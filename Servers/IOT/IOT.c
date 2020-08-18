/*
 * IOT.c
 * Use TCP protocol connection server
 * server address:
 * Created on: 2020年7月31日
 *      Author: ZRH
 */

#include "IOT.h"
/*操作系统库函数调用*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
//#include "GPS/GPS.h"

//宏定义
#define a 20
#define b 45
#define connect_max_time 3													//记录最大重连次数

//receives:IOT缓冲区。a:返回指令条数，b:每条指令长度
char receives[a][b];
//全局变量
char connect_time = 0;														//记录连网次数
char message_again_time = 0;												//心跳包检测失败后重发次数

extern char GPSerr, CANerr;													//GPS,CAN总线错误标记位
extern char Deviceerr;														//4G设备故障
//extern GPS_GPGGA GPGGA;
//extern GPS_GPRMC GPRMC;

/*	无符号8位整型变量
 *  x_axis:横轴计数,即串口单次接受的数据长度(以\r\n作为一条数据结束标记)
 *  y_axis:纵轴计数,即串口同一条命令下接受的数据条数(以\r\n作为一条数据结束标记)
 */
__IO uint8_t x_axis = 0, y_axis = 0;

/*
 * 单独物联网设备复位
 */
void IOT_Reset(int mode)
{
	digitalWriteB(GPIO_Pin_5, LOW);
	delay_us(500000);					//0.5s
	digitalWriteB(GPIO_Pin_5, HIGH);
//	vTaskDelay(5000);    //单位2ms
	choice_delay(5000, mode);
}

/*
 * 物联网设备和单片机先后进行复位
 */
void FullSystemReset(int mode)
{
	IOT_Reset(mode);
	NVIC_SystemReset();
}

/*
 * 物联网设备断开MQTT服务并重连
 */
void Connect_MQTT_again(int mode)
{
	send_cmd("AT+QMTDISC=0 \r\n");		//关闭MQTT服务
	delay_us(200000);					//0.2s
	if (check_receives("OK"))
	{
		;
	} else
	{
		IOT_Reset(mode);
	}
	IOT_Init(mode);							//重连ALiCloud
}

//清空全部IOT接收缓冲区
void Clear_receives()
{
	int x, y;
	for (y = 0; y < a; y++)
	{
		for (x = 0; x < b; x++)
		{
			receives[y][x] = '\0';
		}
	}
}
//清空IOT接收缓冲区的换行符号
void Clear_receives_newline(int line)
{
//	char x;
//	for (x = 0; x < 4; x++)
//	{
//		receives[line][x] = '\0';
//	}
	receives[line][0] = '\0';
	receives[line][1] = '\0';
	receives[line][2] = '\0';
//	receives[line][3] = '\0';
}

//发送消息指令
void send_cmd(char *str)
{
	Clear_receives();
	printf(str);
	x_axis = 0; //清零
	y_axis = 0;
}

//选择释放线程还是延时.0:释放线程
void choice_delay(int tim, int mode)
{
	if (mode == 1)
	{
		delay_us(tim * 2000);
	} else
	{
		vTaskDelay(tim);
	}
}

//得到当前已发送消息的长度
int Send_message_num()
{
	char send_mes_num[10] = { '\0' };
	int num = 0;
	int i, g, h = 0;
	send_cmd("AT+QISEND=0,0 \r\n");
	delay_us(1000000);
	while (!(receives[h][0] == '+') && (receives[h][1] == 'Q') && (receives[h][3] == 's'))//找到+QISEND消息行
	{
		h++;
	}
	if (check_receives("+QISEND"))
	{
		for (i = 0; i < b; i++)						//找到:并且保存i变量
		{
			if (receives[h][i] == ':')
			{
				break;
			}
		}
		for (g = 0; g < b - i; g++)
		{
			if (receives[h][i + g + 2] == ',')
			{
				break;
			}
			send_mes_num[g] = receives[h][i + g + 2];
		}
	}
	for (i = 0; g > 0; g--, i++)
	{
		num += (send_mes_num[i] - 48) * exp10(g - 1);
	}
	return num;
}

//mode：0为释放线程，1是delay_s
void IOT_Init(int mode)
{
	Ping_NET(10, 10, mode);				//Ping10次，每次间隔10S
	connect_time++;
	if (connect_time > connect_max_time)
	{
		IOT_Reset(mode);
		IOT_Init(mode);
		return;
	} else
	{
		send_cmd("AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\" \r\n");
//		vTaskDelay(100);
		choice_delay(100, mode);

		send_cmd("AT+QIOPEN=1,0,\"TCP\",\"47.98.223.167\",6000,0,0 \r\n");
		choice_delay(400, mode);
		if (check_receives("+QIOPEN: 0,0"))
		{
			;
		}
		/*********** 错误情况待测 ************/
//		else if (check_receives("+QMTOPEN: 0,3"))					//无法断开连接，只能复位
//		{
//			IOT_Reset(mode);
//			IOT_Init(mode);
//			return;
//		} else if (check_receives("+QMTSTAT: 0,1"))					//MQTT状态发生变化，重连即可
//		{
//			IOT_Init(mode);
//			return;
//		} else
//		{
//			Connect_MQTT_again(mode);
//			return;
//		}
		send_cmd("AT+QISEND=0 \r\n");
		delay_us(100000);
		send_cmd("~ \r\n");
//		delay_us(200000);
		choice_delay(200, mode);
		if (check_receives("SEND OK"))
		{
			;
		} else
		{
			Connect_MQTT_again(mode);
			return;
		}
		digitalWriteC(GPIO_Pin_14, LOW);					//联网成功
		Deviceerr = 0;
		connect_time = 0;								//复位连网次数
	}
}
//检查是否Ping网成功
bool check_Ping()
{
	for (int i = 0; i < b; i++)
	{
		if (receives[i][10] == '\"')
		{
			return 1;
		} else
		{
			continue;
		}
	}
	return 0;
}

//Ping网函数，Ping成功后才会退出该函数,N次Ping网失败后将尝试重启每次间隔time秒
bool Ping_NET(char N, char time, int mode)
{
	int i, q;
	for (i = 0; i < N; i++)
	{
		send_cmd("AT+QPING=1,\"www.yuque.com\" \r\n");
//		delay_us(2000000); 		//延时2s
//		vTaskDelay(1000);    //单位2ms
		choice_delay(1000, mode);
		if (check_Ping())
		{
			return 1;
		} else if (check_receives("ERROR"))
		{
			break;
		} else
		{
			continue;
		}
//		delay_us(time*1000000); 		//延时time秒
		for (q = 0; q < time; q++)
		{
//			vTaskDelay(500);    //单位2ms
			choice_delay(500, mode);
		}

	}
	IOT_Reset(mode);
	return 0;
}

/*
 * 串口1:IOT接收中断
 */
void USART1_IRQHandler(void)
{
//异常捕获，防止串口陷入死循环
	if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET)
	{
		USART_ReceiveData(USART1);
		return;
	}
	char temp;
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)	//接收到数据
	{
		USART_ClearITPendingBit(USART1, USART_IT_RXNE); //清除接收中断标志
		temp = USART_ReceiveData(USART1); //接收串口1数据到buff缓冲区
		//防止数据溢出，最后一行缓冲区回滚覆写
		if (y_axis > a - 2)
		{
//					y_axis = a - 1;
//					Clear_receives_line(y_axis);			//清除该行数据
			return;
		}
		receives[y_axis][x_axis] = temp;
		x_axis++;
		//防止数据溢出，转到下一行继续记录
		if (x_axis > b - 2)
		{
//			receives[y_axis][b - 1] = 1;
			x_axis = 0;
			y_axis++;
		}
		if ((temp == '\n') && (x_axis > 2))			//不是空行，换行继续写
		{
			y_axis++;								//换行
			x_axis = 0;
		} else if ((temp == '\n') && (x_axis <= 2))
		{
//			Clear_receives_newline(y_axis);			//清除换行符号
			x_axis = 0;
		}
	}
}

//当心跳包检测不到时将尝试重发，3次后则返回失败
bool Message_again()
{
	send_cmd("AT+QMTPUB=0,0,0,1,\"/a1f2CH9BSx7/ZRH_4G/user/put\" \r\n");
	delay_us(100000); //0.1s
	send_cmd("~ \r\n");
	delay_us(300000); //0.3s

	if (check_receives("+QMTPUB: 0,0,0"))
	{
		message_again_time = 0;
		return 1;
	} else if (check_receives("ERROR"))
	{
		message_again_time = 0;
		return 0;
	} else if (message_again_time < 3)
	{
		message_again_time++;
		return Message_again();
	} else
	{
		return 0;
	}
}

//比对str中，是否包含或者全等于cmd
bool iscontants(char *str, char *cmd)
{
	int i = 0, start = 0, count = 0;
	uint8_t lens = strlen(cmd);
	for (i = 0; i < strlen(str); i++)
	{
		if (*(str + i) == *cmd)
		{
			if (*(str + i + 1) == *(cmd + 1))
			{
				start = i;
				break;
			}
		}
	}

	for (i = 0; i < lens; i++)
	{
		if (*(str + start + i) == *(cmd + i))
			count++;
		else
			break;
	}

	if (count == lens)
		return true;
	else
		return false;
}

//检测返回的消息中是否包含待测数据
bool check_receives(char *cmd)
{
	int i = 0, j = 0;
	char str[b] = { '\0' };
	for (j = 0; j < a; j++)
	{
		for (i = 0; i < b; i++)
		{
			str[i] = receives[j][i];						//从缓冲区中提取数据
		}
		if (str[0] == '\0' || str[0] == '\r' || str[0] == '\n')
		{
			continue;
		} else if (iscontants(str, cmd))
		{
			return true;
		}
	}
	return false;
}
