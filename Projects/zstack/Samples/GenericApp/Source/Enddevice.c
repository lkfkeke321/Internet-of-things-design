/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

* 文件名  ：Enddevice
* 作者    ：oyyp
* 时间    ：2021/5/17
* 描述    ：终端节点的主函数
********************************************************************
* 副本
*
*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
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

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */
const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS] = 
{
     // GENERICAPP_MAX_CLUSTERS是在 Coordinator.h 文件中定义的宏
    GENERICAPP_CLUSTERID
};

//下面的数据结构可以用来描述一个 ZigBee 设备节点
const SimpleDescriptionFormat_t GenericApp_SimpleDesc =
{
    GENERICAPP_ENDPOINT,
    GENERICAPP_PROFID,
    GENERICAPP_DEVICEID,
    GENERICAPP_DEVICE_VERSION,
    GENERICAPP_FLAGS,
    0,
    (cId_t *)NULL,
    GENERICAPP_MAX_CLUSTERS,
    (cId_t *)GenericApp_ClusterList
};

//节点描述符 GenericApp_epDesc
endPointDesc_t GenericApp_epDesc; 
//任务优先级 GenericApp_TaskID
byte GenericApp_TaskID;
//数据发送序列号 GenericApp_TransID
byte GenericApp_TransID;           
//保存节点状态的变量 GenericApp_NwkState，
//该变量的类型为 devStates t（deyStates t是一个枚举类型，记录了该设备的状态）
devStates_t GenericApp_NwkState;   
//是消息处理函数 GenericApp_MessageMSGCB
void GenericApp_MessageMSGCB ( afIncomingMSGPacket_t *pckt );
//数据发送函数 GenericApp_SendTheMessage
void GenericApp_SendTheMessage ( void );  

//声明了两个函数，1、消息处理函数，2、数据发送函数
/* 宏定义 ----------------------------------------------------------------*/
/* 结构体或枚举 ----------------------------------------------------------------*/
/* 内部函数声明 ----------------------------------------------------------------*/


/* 函数 ----------------------------------------------------------------*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

* 函数名  ：GenericApp_Init( byte task_id )
* 参数    ：byte  task_id
* 返回    ：void
* 作者    ：oyyp
* 时间    ：2021/5/17
* 描述    ：任务初始化函数
----------------------------------------------------------------*/
//任务初始化函数
void GenericApp_Init( byte task_id )
{
    //初始化了任务优先级
    GenericApp_TaskID               = task_id; 
    //将设备状态初始化为 DEV_INIT，表示该节点没有连接到 ZigBee网络
    GenericApp_NwkState             = DEV_INIT; 
    //将发送数据包的序号初始化为0
    GenericApp_TransID              = 0;        
    GenericApp_epDesc.endPoint      = GENERICAPP_ENDPOINT;
    GenericApp_epDesc.task_id       = &GenericApp_TaskID;
    GenericApp_epDesc.simpleDesc    = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
    //对节点描述符进行的初始化
    GenericApp_epDesc.latencyReq    = noLatencyReqs;  
    //使用 afRegister 函数将节点描述符进行注册，只有注册后，才可以使用 OSAL 提供的系统服务。
    afRegister ( &GenericApp_epDesc );   
}


//消息处理函数(大部分代码是固定的)
UINT16 GenericApp_ProcessEvent ( byte task_id, UINT16 events )
{
    afIncomingMSGPacket_t *MSGpkt;
    if ( events & SYS_EVENT_MSG )
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(GenericApp_TaskID );
        while( MSGpkt )
        {
            switch ( MSGpkt->hdr.event )
            {
            case ZDO_STATE_CHANGE:
                //读取节点的设备类型
                GenericApp_NwkState = (devStates_t)(MSGpkt ->hdr.status);
                //对节点设备类型进行判断
                if (GenericApp_NwkState == DEV_END_DEVICE)   
                {
                    //是终端节点（设备类型码为 DEV_END_DEVICE)实现无线数据发送
                    GenericApp_SendTheMessage();    
                }
                break;
            default:
                break;
            }
            osal_msg_deallocate( (uint8 *)MSGpkt );
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive
                ( GenericApp_TaskID );
        }
        return (events ^ SYS_EVENT_MSG);
    }
    return 0;
}

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

* 函数名  ：GenericApp_SendTheMessage
* 参数    ：void
* 返回    ：void
* 作者    ：oyyp
* 时间    ：2021/5/17
* 描述    ：消息发送函数
----------------------------------------------------------------*/
//实现了数据发送
void GenericApp_SendTheMessage( void )
{
    //定义了一个数组 theMessageData，用于存放要发送的数据
    unsigned char theMessageData[4] = "LED";
    //定义了一个afAddrType t类型的变量my_DstAddr
    afAddrType_t my_DstAddr; 
    //将发送地址模式设置为单播（Addr16Bit 表示单播）
    my_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;  
    //初始化端口号
    my_DstAddr.endPoint = GENERICAPP_ENDPOINT; 
    //在 ZigBee 网络中，协调器的网络地址是固定的，为 0x0000，因此，向协调器发送时，可以直接指定协调器的网络地址
    my_DstAddr.addr.shortAddr = 0x0000;  
    //调用数据发送函数 AF_DataRequest 进行无线数据的发送
    AF_DataRequest( &my_DstAddr, &GenericApp_epDesc,  
                   GENERICAPP_CLUSTERID,
                   3,
                   theMessageData,
                   &GenericApp_TransID,
                   AF_DISCV_ROUTE,
                   AF_DEFAULT_RADIUS );
    //调用 HalLedBlink 函数，使终端节点的LED2闪烁
    HalLedBlink(HAL_LED_2,0,50,500);
}