/*
 * IOT.h
 *
 *  Created on: 2020年7月31日
 *      Author: ZRH
 */

#ifndef IOT_IOT_H_
#define IOT_IOT_H_
#include "UserConfig.h"

#define Door_unlock 0x00
#define Door_lock  0x01
#define Door_open  0x10						//返回开锁成功消息
#define Online  0x11
#define Unknow_message  0x12					//收到未知消息

void IOT_Reset(int mode);
void FullSystemReset(int mode);
void Connect_MQTT_again(int mode);
void Clear_receives();
void Clear_receives_newline(int line);
void send_cmd(char *str);
void choice_delay(int tim, int mode);
void IOT_Init(int mode);
int Send_message_num();
bool Message_again();
bool check_Ping();
bool Ping_NET(char N, char time, int mode);
bool iscontants(char *str, char *cmd);
bool check_receives(char *cmd);

#endif /* IOT_IOT_H_ */
