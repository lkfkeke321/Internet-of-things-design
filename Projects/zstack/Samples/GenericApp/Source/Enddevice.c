/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

* �ļ���  ��Enddevice
* ����    ��oyyp
* ʱ��    ��2021/5/17
* ����    ���ն˽ڵ��������
********************************************************************
* ����
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
     // GENERICAPP_MAX_CLUSTERS���� Coordinator.h �ļ��ж���ĺ�
    GENERICAPP_CLUSTERID
};

//��������ݽṹ������������һ�� ZigBee �豸�ڵ�
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

//�ڵ������� GenericApp_epDesc
endPointDesc_t GenericApp_epDesc; 
//�������ȼ� GenericApp_TaskID
byte GenericApp_TaskID;
//���ݷ������к� GenericApp_TransID
byte GenericApp_TransID;           
//����ڵ�״̬�ı��� GenericApp_NwkState��
//�ñ���������Ϊ devStates t��deyStates t��һ��ö�����ͣ���¼�˸��豸��״̬��
devStates_t GenericApp_NwkState;   
//����Ϣ������ GenericApp_MessageMSGCB
void GenericApp_MessageMSGCB ( afIncomingMSGPacket_t *pckt );
//���ݷ��ͺ��� GenericApp_SendTheMessage
void GenericApp_SendTheMessage ( void );  

//����������������1����Ϣ��������2�����ݷ��ͺ���
/* �궨�� ----------------------------------------------------------------*/
/* �ṹ���ö�� ----------------------------------------------------------------*/
/* �ڲ��������� ----------------------------------------------------------------*/


/* ���� ----------------------------------------------------------------*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

* ������  ��GenericApp_Init( byte task_id )
* ����    ��byte  task_id
* ����    ��void
* ����    ��oyyp
* ʱ��    ��2021/5/17
* ����    �������ʼ������
----------------------------------------------------------------*/
//�����ʼ������
void GenericApp_Init( byte task_id )
{
    //��ʼ�����������ȼ�
    GenericApp_TaskID               = task_id; 
    //���豸״̬��ʼ��Ϊ DEV_INIT����ʾ�ýڵ�û�����ӵ� ZigBee����
    GenericApp_NwkState             = DEV_INIT; 
    //���������ݰ�����ų�ʼ��Ϊ0
    GenericApp_TransID              = 0;        
    GenericApp_epDesc.endPoint      = GENERICAPP_ENDPOINT;
    GenericApp_epDesc.task_id       = &GenericApp_TaskID;
    GenericApp_epDesc.simpleDesc    = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
    //�Խڵ����������еĳ�ʼ��
    GenericApp_epDesc.latencyReq    = noLatencyReqs;  
    //ʹ�� afRegister �������ڵ�����������ע�ᣬֻ��ע��󣬲ſ���ʹ�� OSAL �ṩ��ϵͳ����
    afRegister ( &GenericApp_epDesc );   
}


//��Ϣ������(�󲿷ִ����ǹ̶���)
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
                //��ȡ�ڵ���豸����
                GenericApp_NwkState = (devStates_t)(MSGpkt ->hdr.status);
                //�Խڵ��豸���ͽ����ж�
                if (GenericApp_NwkState == DEV_END_DEVICE)   
                {
                    //���ն˽ڵ㣨�豸������Ϊ DEV_END_DEVICE)ʵ���������ݷ���
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

* ������  ��GenericApp_SendTheMessage
* ����    ��void
* ����    ��void
* ����    ��oyyp
* ʱ��    ��2021/5/17
* ����    ����Ϣ���ͺ���
----------------------------------------------------------------*/
//ʵ�������ݷ���
void GenericApp_SendTheMessage( void )
{
    //������һ������ theMessageData�����ڴ��Ҫ���͵�����
    unsigned char theMessageData[4] = "LED";
    //������һ��afAddrType t���͵ı���my_DstAddr
    afAddrType_t my_DstAddr; 
    //�����͵�ַģʽ����Ϊ������Addr16Bit ��ʾ������
    my_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;  
    //��ʼ���˿ں�
    my_DstAddr.endPoint = GENERICAPP_ENDPOINT; 
    //�� ZigBee �����У�Э�����������ַ�ǹ̶��ģ�Ϊ 0x0000����ˣ���Э��������ʱ������ֱ��ָ��Э�����������ַ
    my_DstAddr.addr.shortAddr = 0x0000;  
    //�������ݷ��ͺ��� AF_DataRequest �����������ݵķ���
    AF_DataRequest( &my_DstAddr, &GenericApp_epDesc,  
                   GENERICAPP_CLUSTERID,
                   3,
                   theMessageData,
                   &GenericApp_TransID,
                   AF_DISCV_ROUTE,
                   AF_DEFAULT_RADIUS );
    //���� HalLedBlink ������ʹ�ն˽ڵ��LED2��˸
    HalLedBlink(HAL_LED_2,0,50,500);
}