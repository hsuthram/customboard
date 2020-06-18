/*******************************************************************************
*           Copyright (c) 2008  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: Functions to handle messaging over sockets and data
                        queues. Messages can contain a payload of data serialized
                        according to defined data types from datatype.h

      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      + This software is the proprietary property of:                   +
      +                                                                 +
      +  Techsonic Industries, Inc.                                     +
      +                                                                 +
      +  1220 Old Alpharetta Rd           1 Humminbird Lane             +
      +  Suite 340                        Eufaula, AL  36027            +
      +  Alpharetta, GA  30005                                          +
      +                                                                 +
      + Use, duplication, or disclosure of this material, in any form,  +
      + is forbidden without permission from Techsonic Industries, Inc. +
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

*******************************************************************************/


/********************           COMPILE FLAGS                ******************/
#ifndef GMSG_H
   #define GMSG_H
#ifdef __cplusplus
   extern "C" {
#endif
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include "global.h"
#include "types.h"
#include "gmsg_id.h"
#include "gmsg_addr.h"
#include "gmsg_info.h"
#include "gmsg_req.h"
#include "datatype.h"
#include "gsocket.h"
#include "service.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define NO_TOKEN                 0xffffffffffffffffULL

#define GMSG_DEFAULT_STATUS       GMSG_STATUS_BASE(SERVICE_CLASS_DEFAULT)
#define GMSG_STATUS_OK            (GMSG_DEFAULT_STATUS+0)
#define GMSG_STATUS_ERROR         (GMSG_DEFAULT_STATUS+1)
#define GMSG_STATUS_UNHANDLED     (GMSG_DEFAULT_STATUS+2)
#define GMSG_STATUS_TIMEOUT       (GMSG_DEFAULT_STATUS+3)
#define GMSG_STATUS_FILTERED      (GMSG_DEFAULT_STATUS+4)
#define GMSG_STATUS_SEND_FAIL     (GMSG_DEFAULT_STATUS+5)

#define GMSG_INIT_SIZE            4096
#define GMSG_MAX_MSG_SIZE         262144

#define GMSG_MAILBOX_SOCKET       0
#define GMSG_MAILBOX_QUEUE        1

#define GMSG_DEFAULT_BACKLOG      16

#define GMSG_FLAG_REQUEST         0x00000001
#define GMSG_FLAG_REPLY           0x00000002
#define GMSG_FLAG_FORWARD         0x00000004

#define GMSG_ANY_ADDR             0x00000000

#define GMSG_SET_READING          1
#define GMSG_SET_WRITING          1
#define GMSG_DONE_READING        -1
#define GMSG_DONE_WRITING        -1

/********************               MACROS                   ******************/

#define MAILBOX_ADDRESS(mbox)    ((mbox).MsgAddress)
#define MAILBOX_TYPE(mbox)       ((mbox).MsgMailboxType)
#define MAILBOX_SOCKET(mbox)     ((mbox).MsgObject.Socket)

#define GMSG_pGetMessageBody(ctype, minfo)   ((ctype *)(minfo)->pStructMsg)
#define GMSG_u64GetToken(minfo)              ((minfo)->pMsgHeader->u64Token)

#define ON_GMSG_STATUS(handler) \
            ON_MESSAGE(GMSG_ID_STATUS, handler, DT_GMSG_STATUS, NULL, NULL)

//let's not clash with cpp message map
#ifndef __cplusplus
#define BEGIN_MESSAGE_MAP(map) \
   PRIVATE GMSG_MAP_INFO (map)[] = \
   {
#define ON_MESSAGE(id, handler, data_type, param1, param2) \
      {(GMSG_ID_TYPE)(id), (GMSG_HANDLER_FP)(handler), (const G_DATA_TYPE*)(data_type), (void*)(param1), (void*)(param2)},
#define END_MESSAGE_MAP() \
      {GMSG_ID_DEFAULT_UNKNOWN, NULL, NULL, NULL, NULL} \
   };
#endif //#ifndef __cplusplus

/********************         TYPE DEFINITIONS               ******************/
typedef U32 GMSG_MAILBOX_TYPE;

typedef union
{
   GSOCKET Socket;
} GMSG_OBJECT;

struct __tag_GMSG_MAP_INFO; //forward definition

typedef struct
{
   GMSG_ADDRESS MsgAddress;
   GSOCKET      Socket;
} GMSG_SOCKET;

typedef struct __tag_GMSG_SOCKET_NODE
{
   GMSG_SOCKET  MsgSocket;
   int          nReading;
   int          nWriting;
   BOOLEAN      bAddressSet;
   BOOLEAN      bReadOnly;
   BOOLEAN      bWriteOnly;
   BOOLEAN      bMarkForDelete;
   struct __tag_GMSG_SOCKET_NODE *pNext;
} GMSG_SOCKET_NODE;

typedef struct
{
   GMSG_SOCKET_NODE *pHead;
   GMUTEX            SocketInfoMutex;
}GMSG_SOCKET_INFO;

typedef struct
{
   MBOX                        RtxcMailbox;
   SEMA                        RtxcSema;
   GMSG_MAILBOX_TYPE           MsgMailboxType;
   GMSG_ADDRESS                MsgAddress;
   GMSG_OBJECT                 MsgObject;
   struct __tag_GMSG_MAP_INFO *pMsgMapInfoArray;
   GMSG_REQUEST_INFO           MsgRequestInfo;
   GMSG_SOCKET_INFO            MsgSocketInfo;
   int                         nReading;
   int                         nAccess;
   BOOLEAN                     bMarkForDelete;
   BOOLEAN                     bUdp;
   U32                         u32MulticastAddress;
} GMSG_MAILBOX;

typedef BOOLEAN (*GMSG_HANDLER_FP)(MBOX Mailbox, GMSG_INFO *pMsgInfo, void *pvParam1, void *pvParam2);

typedef struct __tag_GMSG_MAP_INFO
{
   GMSG_ID_TYPE       MsgIdType;
   GMSG_HANDLER_FP    pfHandler;
   const G_DATA_TYPE  *pDataType;
   void             *pvParam1;
   void             *pvParam2;
} GMSG_MAP_INFO;

typedef struct
{
   U32 u32MsgStatus;
   U32 u32MsgId;
   U64 u64Token;
} GMSG_STATUS;

typedef struct
{
   BOOLEAN bRaw;
   U32 u32DataLength;
   U32 u32MultiPacket;
} GMSG_ALT_TRANSPORT;

/********************        FUNCTION PROTOTYPES             ******************/
//Mailbox handling functions
extern BOOLEAN GMSG_bInitSocketMailbox(GMSG_MAILBOX *pMyMailbox, GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port, int nBacklog,
                                       MBOX Mailbox, RESOURCE Resource, SEMA Sema, BOOLEAN bUdp, U32 u32McIp);
extern BOOLEAN GMSG_bCloseMailbox(GMSG_MAILBOX *pMyMailbox);
extern BOOLEAN GMSG_bHandleMsg(GMSG_MAILBOX *pMsgMailbox, GMSG_INFO *pMsgInfo);
extern BOOLEAN GMSG_bDefaultMsgHandler(MBOX Mailbox, GMSG_INFO *pMsgInfo, void *pvParam1, void *pvParam2);

//Request/reply list functions
extern GMSG_REQUEST_NODE * GMSG_pFindRequest(GMSG_REQUEST_INFO *pMsgRequestInfo, U64 u64Token);
extern GMSG_REQUEST_NODE * GMSG_pFindExpiredRequest(GMSG_REQUEST_INFO *pMsgRequestInfo);
extern BOOLEAN GMSG_bAddRequest(GMSG_REQUEST_INFO *pMsgRequestInfo, GMSG_INFO *pMsgInfo);
extern BOOLEAN GMSG_bDeleteRequestByToken(GMSG_REQUEST_INFO *pMsgRequestInfo, U64 u64Token);
extern BOOLEAN GMSG_bDeleteRequest(GMSG_REQUEST_INFO *pMsgRequestInfo, GMSG_REQUEST_NODE *pNodeToDelete);
extern BOOLEAN GMSG_bHandleReplies(GMSG_REQUEST_INFO *pMsgRequestInfo, GMSG_INFO *pMsgInfo);
extern BOOLEAN GMSG_bHandleRequests(GMSG_REQUEST_INFO *pMsgRequestInfo, GMSG_INFO *pMsgInfo);


//application-level send/receive
extern BOOLEAN GMSG_bMessageTimeout(GMSG_MAILBOX *pMailbox);
extern BOOLEAN GMSG_bAcceptMessage(GMSG_MAILBOX *pMailbox, GSOCKET *pReceiveSocket);
extern BOOLEAN GMSG_bReceiveMessage(GMSG_MAILBOX *pMailbox, GSOCKET *pReceiveSocket, GMSG_INFO *pMsgInfo);
extern BOOLEAN GMSG_bCompose(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pSrcAddress, GMSG_ADDRESS *pDstAddress,
                             GMSG_ID_TYPE eMsgId, const G_DATA_TYPE *pDataType, void *pData, U64 u64Token,
                             U64 __u64ReplyToken, U32 u32Flags, BOOLEAN bUdp, U32 u32UdpLength);

extern BOOLEAN GMSG_bRequest(GMSG_MAILBOX *pMyMailbox, GMSG_ADDRESS *pDstAddress,
                            GMSG_ID_TYPE eMsgId,const G_DATA_TYPE *pDataType,
                            void *pData, U64 u64Token);
extern BOOLEAN GMSG_bSendNoReply(GMSG_ADDRESS *pSrcAddress, GMSG_ADDRESS *pDstAddress,
                                GMSG_ID_TYPE eMsgId, const G_DATA_TYPE *pDataType, void *pData);
extern BOOLEAN GMSG_bSendNoReplyWithToken(GMSG_ADDRESS *pSrcAddress, GMSG_ADDRESS *pDstAddress,
                                          GMSG_ID_TYPE eMsgId, const G_DATA_TYPE *pDataType,
                                          void *pData, U64 u64Token);
extern BOOLEAN GMSG_bReply(GMSG_INFO *pMsgInfo, GMSG_ID_TYPE eMsgId, const G_DATA_TYPE *pDataType, void *pData);
extern BOOLEAN GMSG_bReplyWithStatus(GMSG_INFO *pMsgInfo, U32 u32MsgStatus);

//misc
extern BOOLEAN GMSG_bAddressEqual(const GMSG_ADDRESS *pMsgAddress1, const GMSG_ADDRESS *pMsgAddress2);

//low-level send/receive
extern BOOLEAN GMSG_bReceiveSocket(GSOCKET *pReceiveSocket, GMSG_INFO *pMsgInfo);
extern BOOLEAN GMSG_bReceiveSocketAdapter(unsigned char *messageBody, int messageLen, GMSG_INFO *pMsgInfo);
extern BOOLEAN GMSG_bReceiveUDPSocket(GSOCKET *pListenSocket, GMSG_INFO *pMsgInfo);
extern BOOLEAN GMSG_bParse(GMSG_INFO *pMsgInfo, const G_DATA_TYPE *pDataType);
extern BOOLEAN GMSG_bCommonSend(GMSG_ADDRESS *pSrcAddress, GMSG_ADDRESS *pDstAddress,
                                GMSG_ID_TYPE eMsgId, const G_DATA_TYPE *pDataType,
                                void *pData, U64 u64Token, U64 __u64ReplyToken,
                                GMSG_REQUEST_INFO *pMsgRequestInfo, U32 u32Flags);
extern BOOLEAN GMSG_bCreate(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pMyAddress, GMSG_ID_TYPE eMsgId,
                           const G_DATA_TYPE *pDataType, void *pData, U32 u32Flags);
extern BOOLEAN GMSG_bSerialize(GMSG_INFO *pMsgInfo);
extern BOOLEAN GMSG_bAddress(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pDstAddress, U64 u64Token);
extern BOOLEAN GMSG_bNewSocket(GSOCKET *pSocket, GMSG_ADDRESS *pDstAddress);
extern BOOLEAN GMSG_bSendSocket(GMSG_INFO *pMsgInfo, GSOCKET *pSendSocket, BOOLEAN bFreeMsgInfo);
extern BOOLEAN GMSG_bSendQueue(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pDstAddress, BOOLEAN bFreeMsgInfo);
extern BOOLEAN GMSG_bSend(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pDstAddress, BOOLEAN bFreeMsgInfo);
extern BOOLEAN GMSG_bFree(GMSG_INFO *pMsgInfo);
extern BOOLEAN GMSG_bSendSocketUDP(GSOCKET socket, unsigned char *pMsgBuffer, int nLen, struct sockaddr *dstAddr);

extern void    GMSG_vMakeSocketAddress(GMSG_ADDRESS *pAddress, U32 u32Address, U32 u32Port);
extern BOOLEAN GMSG_bMsgAvailable(GSOCKET *pSocket, GMSG_HEADER *pHeader);
extern BOOLEAN GMSG_bMsgAvailableUdp(GSOCKET *pSocket, GMSG_HEADER *pHeader);
extern BOOLEAN GMSG_bMsgAvailableAll(GSOCKET *pSocket, GMSG_HEADER *pHeader, BOOLEAN bUdp);

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/
DT_EXTERN_TYPE(DT_GMSG_STATUS);

/********************              FUNCTIONS                 ******************/
#ifdef __cplusplus
   }
#endif
#endif      // GMSG_H
