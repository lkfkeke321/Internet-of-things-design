/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
* 文件名  ：Coordinator
* 作者    ：longjiang
* 版本    ：V0.0.1
* 时间    ：2021/5/18
* 描述    ：协调器头文件
********************************************************************
* 副本
*
*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/


#ifndef _COORDINATOR_H
#define _COORDINATOR_H
/* 头文件 ----------------------------------------------------------------*/
#include "ZComDef.h"
/* 宏定义 ----------------------------------------------------------------*/
#define GENERICAPP_ENDPOINT       10
#define GENERICAPP_PROFID         0x0F04
#define GENERICAPP_DEVICEID       0x0001
#define GENERICAPP_DEVICE_VERSION 0
#define GENERICAPP_FLAGS          0
#define GENERICAPP_MAX_CLUSTERS   1
#define GENERICAPP_CLUSTERID      1
/* 结构体或枚举 ----------------------------------------------------------------*/
/* 函数声明----------------------------------------------------------------*/
extern void GenericApp_Init(byte task_id);
extern UINT16 GenericApp_ProcessEvent(byte task_id,UINT16 events);
/* 外部变量引用 ----------------------------------------------------------------*/

#endif // _COORDINATOR_H

