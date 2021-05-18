/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

* 文件名  ：Coordinator
* 作者    ：caiyu
* 时间    ：2021/5/17
* 描述    ：协调器驱动
********************************************************************
* 副本
*
*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/* 头文件 ----------------------------------------------------------------*/
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include <string.h>
#include "Coordinator.h"
#include "DebugTrace.h"
#if !defined( WIN32 )
#include "OnBoard.h"
#endif
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
//实现点对点通信  接收
const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS] =
{
    GENERICAPP_CLUSTERID
};// GENERICAPP_MAX_CLUSTERS是在 Coordinator.h 文件中定义的宏
const SimpleDescriptionFormat_t GenericApp_SimpleDesc =
{
    GENERICAPP_ENDPOINT,
    GENERICAPP_PROFID,
    GENERICAPP_DEVICEID,
    GENERICAPP_DEVICE_VERSION,
    GENERICAPP_FLAGS,
    GENERICAPP_MAX_CLUSTERS,
    (cId_t *)GenericApp_ClusterList,
    0,
    (cId_t *)NULL
};//上面的数据结构可以用来描述一个 ZigBee 设备节点
endPointDesc_t GenericApp_epDesc;  //节点描述符GenericApp_epDesc  在ZigBee协议中新定义的类型一般以"t"结尾
byte GenericApp_TaskID;   //任务优先级GenericApp_TaskID
byte GenericApp_TransID;  //数据发送序列号GenericApp_TransID
//unsigned char uartbuf[128];
void GenericApp_MessageMSGCB ( afIncomingMSGPacket_t *pckt );//消息梳理函数GenericApp_MessageMSGCB
void GenericApp_sendTheMessage ( void ) ;//数据发送函数GenericApp_sendTheMessage
//static void rxCB (uint8 port,uint8 event);
//声明了两个函数，1、消息处理函数，2、数据发送函数
/* 宏定义 ----------------------------------------------------------------*/
/* 结构体或枚举 ----------------------------------------------------------------*/
/* 内部函数声明 ----------------------------------------------------------------*/


/* 函数 ----------------------------------------------------------------*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

* 函数名  ：GenericApp
* 参数    ：byte  task_id
* 返回    ：void
* 作者    ：caiyu
* 时间    ：2021/5/17
* 描述    ：任务初始化函数
----------------------------------------------------------------*/
void GenericApp_Init( byte task_id )
{
    halUARTCfg_t uartConfig;
    GenericApp_TaskID              =task_id; //初始化了任务优先级,任务优先级有协议栈的操作系统OSAL分配
    GenericApp_TransID             = 0;      //将发送数据包的序号初始化为 0，在 ZigBee 协议栈中，每发送一个数据包,该发送序号自动加1
    GenericApp_epDesc.endPoint     = GENERICAPP_ENDPOINT;//对节点描述符进行的初始化
    GenericApp_epDesc.task_id      = &GenericApp_TaskID;
    GenericApp_epDesc.simpleDesc   = 
        (SimpleDescriptionFormat_t *) &GenericApp_SimpleDesc;
    GenericApp_epDesc.latencyReq   = noLatencyReqs;
    afRegister( &GenericApp_epDesc );//使用 afRegister 函数将节点描述符进行注册，只有注册以后,才可以使用OSAL提供的系统服务
    uartConfig.configured          = TRUE;
    uartConfig.baudRate            = HAL_UART_BR_115200;
    uartConfig.flowControl         = FALSE;
    uartConfig.callBackFunc        = NULL;
    HalUARTOpen (0,&uartConfig);
}
//消息处理函数
UINT16 GenericApp_ProcessEvent( byte task_id,UINT16 events )
{
    
    afIncomingMSGPacket_t *MSGpkt;//定义了一个指向接收消息结构体的指针 MSGpkt
    if ( events & SYS_EVENT_MSG )
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive (GenericApp_TaskID ) ;//使用osal_msg_receive 函数从消息队列上接收消息，该消息中包含了接收到的无线数据包
        while ( MSGpkt )
        {
            switch ( MSGpkt->hdr.event )
            {
            case AF_INCOMING_MSG_CMD://对接收到的消息进行判断，如果是接收到了无线数据，则调用下一行的函数对数据进行相应的处理。
                GenericApp_MessageMSGCB ( MSGpkt ) ;//完成对数据接收的处理
                break;
            default:
                break;
            }
            osal_msg_deallocate( (uint8 *)MSGpkt );//接收到的消息处理完后,需要调用 osal_msg_ deallocate 函数将其占据的堆内存释放，否则容易引起"内存泄漏"。
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive//处理完一个消息后，再从消息队列里接收消息，然后对其进行相应的处理，直到所有消息都处理完为止。
                ( GenericApp_TaskID ) ;
        }
        return (events ^ SYS_EVENT_MSG) ;
    }
    return 0;
    
}
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

* 函数名  ：rxCB
* 参数    ：uint8 port,uint8 avent
* 返回    ：void
* 作者    ：caiyu
* 时间    ：2021/5/17
* 描述    ：回调函数
----------------------------------------------------------------*/
/*
static void rxCB(uint8 port,uint8 event)
{
    HalUARTRead(0,uartbuf,16);
    if(osal_memcmp(uartbuf,"www.wlwmaker.com",16))
    {
        HalUARTWrite(0,uartbuf,16);
    }
}
*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

* 函数名  ：GenericApp_MessageMSGCB
* 参数    ：afIncomingMSGPacket_t *pkt
* 返回    ：void
* 作者    ：caiyu
* 时间    ：2021/5/17
* 描述    ：接收数据
----------------------------------------------------------------*/
void GenericApp_MessageMSGCB ( afIncomingMSGPacket_t *pkt )
{
    /*unsigned char buffer[4] = "    ";*/
    unsigned char buffer[10];
    switch ( pkt->clusterId )
    {
    case GENERICAPP_CLUSTERID:
        /*osal_memcpy (buffer,pkt->cmd.Data,3);*/
        osal_memcpy (buffer,pkt->cmd.Data,10);//将收到的数据拷贝到缓冲区 buffer 中
        HalUARTWrite(0,buffer,10);
        /*
        if ((buffer[0] =='L') || (buffer[1] == 'E') || (buffer [2]=='D'))//判断接收到的数据是不是"LED"三个字符
        {
            HalLedBlink (HAL_LED_2,0,50,500);//是这三个字符，则执行，使LED2闪烁(HalLedBlink功能是使某个LED闪烁)
        }
        else
        {
            HalLedSet (HAL_LED_2,HAL_LED_MODE_ON);//如果接收到的不是这三个字符则点亮LED2(HalLedSet功能是;设置某个LED的状态) 
        }
        */
        break;
    }
}  

