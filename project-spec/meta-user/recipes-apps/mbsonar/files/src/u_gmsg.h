/*******************************************************************************
*           Copyright (c) 2010  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
*******************************************************************************/

/**
 * @file u_gmsg.h
 *
 */

/*******************************************************************************

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
#ifndef U_GMSG_H
   #define U_GMSG_H
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include "global.h"
#include "types.h"
#include "message.h"
#include "gmsg.h"
#ifndef linux
   #ifndef USE_LWIP_STACK
      #define USE_LWIP_STACK 0
   #endif
   #if USE_LWIP_STACK
      #include "lwip/sockets.h"
   #else
      #include "fns_bsdsockapi.h"
   #endif
#endif
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/

// CMD_ID_GMSG_REGISTER
//    Register a Mailbox to receive messages from a socket
typedef struct
{
   GENERIC_MESSAGE   header;
   MBOX              Mailbox;
   RESOURCE          ResLock;
   SEMA              Sema;
   GMSG_MAP_INFO    *pMsgMapInfo;
   U16               u16Port;
   BOOLEAN           bSuccess;
   BOOLEAN           bUdp;
   U32               u32McIp;
} MSG_GMSG_REGISTER;

// CMD_ID_GMSG_UNREGISTER
//    Unregister a Mailbox from receiving messages from a socket
typedef struct
{
   GENERIC_MESSAGE   header;
   MBOX              Mailbox;
   BOOLEAN           bSuccess;
} MSG_GMSG_UNREGISTER;

// CMD_ID_GMSG_RECEIVE
//    An application receives a message
typedef struct
{
   GENERIC_MESSAGE   header;
   MBOX              Mailbox;
   GMSG_INFO         MsgInfo;
} MSG_GMSG_RECEIVE;

// CMD_ID_GMSG_SEND
//    An application sends a message request, reply, or send (without reply)
typedef struct
{
   GENERIC_MESSAGE   header;
   MBOX              Mailbox;
   GMSG_INFO         MsgInfo;
   BOOLEAN           bSuccess;
} MSG_GMSG_SEND;

// CMD_ID_GMSG_DISCONNECT
//    An application is informed when sockets disconnect
typedef struct
{
   GENERIC_MESSAGE   header;
   MBOX              Mailbox;
   GMSG_ADDRESS      Address;
} MSG_GMSG_DISCONNECT;

typedef struct tag_GMSG_REG_MAILBOX_NODE
{
   GMSG_MAILBOX                      Mailbox;
   struct tag_GMSG_REG_MAILBOX_NODE *pNext;
} GMSG_REG_MAILBOX_NODE;

typedef struct
{
   GMSG_REG_MAILBOX_NODE *pHead;
} GMSG_REG_MAILBOX_LIST;

/********************        FUNCTION PROTOTYPES             ******************/
extern BOOLEAN U_GMSG_bRegisterMailbox(MBOX Mailbox, RESOURCE ResLock, SEMA Sema, GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port);
extern BOOLEAN U_GMSG_bRegisterMailboxAll(MBOX Mailbox, RESOURCE ResLock, SEMA Sema, GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port, BOOLEAN bUdp);
extern BOOLEAN U_GMSG_bRegisterMailboxMulticast(MBOX Mailbox, RESOURCE ResLock, SEMA Sema, GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port, BOOLEAN bUdp, U32 u32McIp);
extern BOOLEAN U_GMSG_bUnregisterMailbox(MBOX Mailbox);
extern BOOLEAN U_GMSG_bHandleReceive(MSG_GMSG_RECEIVE *pMsgReceive);
extern BOOLEAN U_GMSG_bSkipReceive(MSG_GMSG_RECEIVE *pMsgReceive);
extern BOOLEAN U_GMSG_bRequest(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                               const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken);
extern BOOLEAN U_GMSG_bReply(MBOX Mailbox, GMSG_INFO *pMsgInfo, GMSG_ID_TYPE eMsgId,
                             const G_DATA_TYPE *pDataType, void *pData);
extern BOOLEAN U_GMSG_bReplyWithStatus(MBOX Mailbox, GMSG_INFO *pMsgInfo, U32 u32MsgStatus);
extern BOOLEAN U_GMSG_bSendMsgNoReply(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                      const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken);
extern BOOLEAN U_GMSG_bSendMsgNoReplyAlt(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                      const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken, GMSG_ALT_TRANSPORT *altTransport);
/*
extern BOOLEAN U_GMSG_bSend(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                             const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken,
                             U64 __u64ReplyToken, U32 u32Flags, BOOLEAN bAsync);
                             U64 __u64ReplyToken, U32 u32Flags, BOOLEAN bAsync);
*/
extern BOOLEAN U_GMSG_bSend(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                             const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken,
                             U64 __u64ReplyToken, U32 u32Flags, GMSG_ALT_TRANSPORT *altTransport,
                             BOOLEAN bUdp, U32 u32UdpLength, BOOLEAN bAsync, U32 u32TimeoutInMs);
extern BOOLEAN U_GMSG_bDisconnect(MBOX Mailbox, GMSG_ADDRESS *pAddress);

extern BOOLEAN U_GMSG_bSendUdpMsgNoReply(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                      const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken,
                                      U32 u32MsgLength);

extern void U_GMSG_vShutdown(void);

extern void U_GMSG_vOnTimer(void);


extern BOOLEAN U_GMSG_bAddMailboxNode(GMSG_REG_MAILBOX_NODE *pMailboxNode);
extern GMSG_REG_MAILBOX_NODE *U_GMSG_pAddNewMailboxNode(GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port,
                                                        MBOX Mailbox, RESOURCE ResLock, SEMA Sema, BOOLEAN bUdp, U32 u32McIp);
extern BOOLEAN U_GMSG_bDeleteMailboxNode(GMSG_REG_MAILBOX_NODE *pMailboxNode);
extern BOOLEAN U_GMSG_bDeleteMailboxNodeById(MBOX Mailbox);
extern BOOLEAN U_GMSG_bGetMailboxAddress(GMSG_ADDRESS *pAddress, MBOX Mailbox);
extern GMSG_REG_MAILBOX_NODE *U_GMSG_pFindMailboxNodeById(MBOX Mailbox);
extern GMSG_REG_MAILBOX_NODE *U_GMSG_pFindMailboxNodeByFd(int nSocket, BOOLEAN *pbListenSocket);
extern GMSG_REG_MAILBOX_NODE *U_GMSG_pFindMailboxNodeWithExpiredRequest(GMSG_REQUEST_NODE **ppRequestNode);
extern GMSG_REG_MAILBOX_NODE *U_GMSG_pAcquireMailboxNodeById(MBOX Mailbox, BOOLEAN bReading);
extern GMSG_REG_MAILBOX_NODE *U_GMSG_pAcquireMailboxNodeByFd(int nSocket, BOOLEAN *pbListenSocket, BOOLEAN bReading);
extern GMSG_REG_MAILBOX_NODE *U_GMSG_pAcquireMailboxNodeWithExpiredRequest(GMSG_REQUEST_NODE **ppRequestNode);
extern BOOLEAN U_GMSG_bAcquireMailboxNode(GMSG_REG_MAILBOX_NODE *pMailboxNode, BOOLEAN bReading);
extern BOOLEAN U_GMSG_bReleaseMailboxNode(GMSG_REG_MAILBOX_NODE *pMailboxNode, BOOLEAN bReading);
extern BOOLEAN U_GMSG_bSignal(MBOX Mailbox);


extern BOOLEAN U_GMSG_bGetMailboxFdSet(fd_set *pReadFdSet, int *pnMaxSocket);
extern BOOLEAN U_GMSG_bUpdateSocketNode(GMSG_MAILBOX *pMailbox, GMSG_SOCKET_NODE *pSocketNode, GSOCKET *pSocket,
                                        GMSG_ADDRESS *pAddress, int nReading, int nWriting);
extern BOOLEAN U_GMSG_bAddSocketNode(GMSG_MAILBOX *pMailbox, GMSG_SOCKET_NODE *pSocketNode);
extern GMSG_SOCKET_NODE *U_GMSG_pAddNewSocketNode(GMSG_MAILBOX *pMailbox, GMSG_ADDRESS *pAddress, BOOLEAN bReading, BOOLEAN bAcquire);
extern GMSG_SOCKET_NODE *U_GMSG_pAddNewSocketNodeUdp(GMSG_MAILBOX *pMailbox, GMSG_ADDRESS *pAddress, BOOLEAN bReading, BOOLEAN bAcquire);
extern BOOLEAN U_GMSG_bDeleteSocketNode(GMSG_MAILBOX *pMailbox, GMSG_SOCKET_NODE *pSocketNode);
extern GMSG_SOCKET_NODE *U_GMSG_pFindSocketNodeByFd(GMSG_MAILBOX *pMailbox, int nSocket);
extern GMSG_SOCKET_NODE *U_GMSG_pFindSocketNodeByAddress(GMSG_MAILBOX *pMailbox, GMSG_ADDRESS *pAddress, BOOLEAN bReading);
extern GMSG_SOCKET_NODE *U_GMSG_pAcquireSocketNodeByFd(GMSG_MAILBOX *pMailbox, int nSocket, BOOLEAN bReading);
extern GMSG_SOCKET_NODE *U_GMSG_pAcquireSocketNodeByAddress(GMSG_MAILBOX *pMailbox, GMSG_ADDRESS *pAddress, BOOLEAN bReading);
extern BOOLEAN U_GMSG_bUpdateSocketNodeAddress(GMSG_MAILBOX *pMailbox, GMSG_SOCKET_NODE *pSocketNode, GMSG_ADDRESS *pAddress);
extern BOOLEAN U_GMSG_bReleaseSocketNode(GMSG_MAILBOX *pMailbox, GMSG_SOCKET_NODE *pSocketNode, BOOLEAN bWasReading);
extern BOOLEAN U_GMSG_bGetSocketInfoFdSet(GMSG_REG_MAILBOX_NODE *pMailboxNode, fd_set *pReadFdSet, int *pnMaxSocket);
extern BOOLEAN U_GMSG_bSendMsgNoWait(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                      const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken, U32 u32TimeoutInMs);
extern BOOLEAN U_GMSG_bSendMsgNoWaitAsync(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                      const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken, U32 u32TimeoutInMs);

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/
#endif //U_GMSG_H
