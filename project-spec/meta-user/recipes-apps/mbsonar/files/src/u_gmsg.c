/*******************************************************************************
*           Copyright (c) 2010  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
*******************************************************************************/

/**
 * @file u_gmsg.c
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
/********************           INCLUDE FILES                ******************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lsvc.h"
#include "gmsg.h"
#include "t_gmsg.h"
#include "u_gmsg.h"
#include "error.h"
#include "t_sudp_out.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define MSG_DEFAULT_TIMEOUT            (0)

/********************               MACROS                   ******************/
#define U_GMSG_SET_FLAG(flags, mask)               (flags) |= (mask)
#define U_GMSG_CLR_FLAG(flags, mask)               (flags) &= (~(mask))
#define U_GMSG_IS_FLAG_SET(flags, mask)            (((flags) & (mask)) == (mask))
#define U_GMSG_IS_FLAG_CLR(flags, mask)            (((flags) & (mask)) == 0)

/********************         TYPE DEFINITIONS               ******************/

/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/
PRIVATE GMSG_REG_MAILBOX_LIST s_RegMailboxList;

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/

/******************************************************************************/
/**
 * Register an RTXC MailBox with a listening TCP/IP or UDP port.
 *
 * CMD_ID_GMSG_RECEIVE is sent to Mailbox for each incoming message. Call
 * U_GMSG_bHandleMessage with your received message.
 *
 * @param Mailbox     RTXC Mailbox
 * @param ResLock     RTXC Resource
 * @param Sema        RTXC Semaphore
 * @param pMsgMapInfo Message Map Array
 * @param u16Port     IP Listening Port (see service.h for port range)
 *
 * @return BOOLEAN    TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bRegisterMailbox(MBOX Mailbox, RESOURCE ResLock, SEMA Sema, GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port)
{
   return(U_GMSG_bRegisterMailboxAll(Mailbox, ResLock, Sema, pMsgMapInfo, u16Port, FALSE));
}

/******************************************************************************/
/**
 * Register an RTXC MailBox with a listening TCP/IP or UDP port.
 *
 * CMD_ID_GMSG_RECEIVE is sent to Mailbox for each incoming message. Call
 * U_GMSG_bHandleMessage with your received message.
 *
 * @param Mailbox     RTXC Mailbox
 * @param ResLock     RTXC Resource
 * @param Sema        RTXC Semaphore
 * @param pMsgMapInfo Message Map Array
 * @param u16Port     IP Listening Port (see service.h for port range)
 * @param bUdp        Udp Port
 *
 * @return BOOLEAN    TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bRegisterMailboxAll(MBOX Mailbox, RESOURCE ResLock, SEMA Sema, GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port, BOOLEAN bUdp)
{
   MSG_GMSG_REGISTER *pMsgRegister = (MSG_GMSG_REGISTER*)malloc(sizeof(MSG_GMSG_REGISTER));
   BOOLEAN            bSuccess;

   pMsgRegister->header.Cmd   = CMD_ID_GMSG_REGISTER;
   pMsgRegister->Mailbox      = Mailbox;
   pMsgRegister->ResLock      = ResLock;
   pMsgRegister->Sema         = Sema;
   pMsgRegister->pMsgMapInfo  = pMsgMapInfo;
   pMsgRegister->u16Port      = u16Port;
   pMsgRegister->bSuccess     = FALSE;
   pMsgRegister->bUdp         = bUdp;
   pMsgRegister->u32McIp      = 0;
   //KS_sendw(M_GMSG, (RTXCMSG*)pMsgRegister, NORMAL_PRIORITY, Sema);
   KS_send(M_GMSG, (RTXCMSG*)pMsgRegister, NORMAL_PRIORITY, Sema);
   bSuccess = pMsgRegister->bSuccess;
   free(pMsgRegister);
   printf("[%d]%s: bSuccess = %d\n", __LINE__, __FUNCTION__, bSuccess);
   return(bSuccess);
}

/******************************************************************************/
/**
 * Register an RTXC MailBox with a listening TCP/IP or UDP port.
 *
 * CMD_ID_GMSG_RECEIVE is sent to Mailbox for each incoming message. Call
 * U_GMSG_bHandleMessage with your received message.
 *
 * @param Mailbox     RTXC Mailbox
 * @param ResLock     RTXC Resource
 * @param Sema        RTXC Semaphore
 * @param pMsgMapInfo Message Map Array
 * @param u16Port     IP Listening Port (see service.h for port range)
 * @param bUdp        Udp Port
 * @param sMcIp       Multicast Ip address
 *
 * @return BOOLEAN    TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bRegisterMailboxMulticast(MBOX Mailbox, RESOURCE ResLock, SEMA Sema, GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port, BOOLEAN bUdp, U32 u32McIp)
{
   BOOLEAN bSuccess;

   MSG_GMSG_REGISTER *pMsgRegister = (MSG_GMSG_REGISTER*)malloc(sizeof(MSG_GMSG_REGISTER));
   pMsgRegister->header.Cmd   = CMD_ID_GMSG_REGISTER;
   pMsgRegister->Mailbox      = Mailbox;
   pMsgRegister->ResLock      = ResLock;
   pMsgRegister->Sema         = Sema;
   pMsgRegister->pMsgMapInfo  = pMsgMapInfo;
   pMsgRegister->u16Port      = u16Port;
   pMsgRegister->bSuccess     = FALSE;
   pMsgRegister->bUdp         = bUdp;
   pMsgRegister->u32McIp      = u32McIp;   
   KS_sendw(M_GMSG, (RTXCMSG*)pMsgRegister, NORMAL_PRIORITY, Sema);
   bSuccess = pMsgRegister->bSuccess;
   free(pMsgRegister);
   return(bSuccess);
}


/******************************************************************************/
/**
 * Unregister an RTXC MailBox from a listening TCP/IP port.
 *
 * @param Mailbox RTXC Mailbox
 *
 * @return BOOLEAN TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bUnregisterMailbox(MBOX Mailbox)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode;
   BOOLEAN bSuccess = FALSE;

   if (NULL != (pMailboxNode = U_GMSG_pAcquireMailboxNodeById(Mailbox, FALSE)))
   {
      MSG_GMSG_UNREGISTER *pMsgUnregister;
      SEMA Sema;

      Sema = pMailboxNode->Mailbox.RtxcSema;
      U_GMSG_bReleaseMailboxNode(pMailboxNode, FALSE);
      pMsgUnregister = (MSG_GMSG_UNREGISTER*)malloc(sizeof(MSG_GMSG_UNREGISTER));
      pMsgUnregister->header.Cmd = CMD_ID_GMSG_UNREGISTER;
      pMsgUnregister->Mailbox    = Mailbox;
      pMsgUnregister->bSuccess   = FALSE;
      KS_sendw(M_GMSG, (RTXCMSG*)pMsgUnregister, NORMAL_PRIORITY, Sema);
      bSuccess = pMsgUnregister->bSuccess;
      free(pMsgUnregister);
   }
   return(bSuccess);
}

/******************************************************************************/
/**
 *
 *
 * @param pMsgReceive
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bHandleReceive(MSG_GMSG_RECEIVE *pMsgReceive)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode;

   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeById(pMsgReceive->Mailbox)))
   {
//      printf("[%d]%s: u32MsgSignature 0x%x\n", __LINE__, __FUNCTION__, pMsgReceive->MsgInfo.pMsgHeader->u32MsgSignature);
//      printf("[%d]%s: u32MsgId 0x%x\n", __LINE__, __FUNCTION__, pMsgReceive->MsgInfo.pMsgHeader->u32MsgId);
//      printf("[%d]%s: Src Address 0x%x\n", __LINE__, __FUNCTION__, pMsgReceive->MsgInfo.pMsgHeader->SrcAddress.uAddr.Socket.u32Address);
      GMSG_bHandleMsg(&pMailboxNode->Mailbox, &pMsgReceive->MsgInfo);
      GMSG_bFree(&pMsgReceive->MsgInfo);
      U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
   }
   U_GMSG_bSignal(pMsgReceive->Mailbox);
   return(TRUE);
}

/******************************************************************************/
/**
 *
 *
 * @param pMsgReceive
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bSkipReceive(MSG_GMSG_RECEIVE *pMsgReceive)
{
   GMSG_REG_MAILBOX_NODE  *pMailboxNode;

   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeById(pMsgReceive->Mailbox)))
   {
      GMSG_bFree(&pMsgReceive->MsgInfo);
      U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
   }
   U_GMSG_bSignal(pMsgReceive->Mailbox);
   return(TRUE);
}

/******************************************************************************/
/**
 *
 *
 * @param Mailbox
 * @param pDstAddress
 * @param eMsgId
 * @param pDataType
 * @param pData
 * @param u64UserToken
 * @param __u64ReplyToken
 * @param u32Flags
 * @param bAsync
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bSend(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                             const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken,
                             U64 __u64ReplyToken, U32 u32Flags, GMSG_ALT_TRANSPORT *altTransport,
                             BOOLEAN bUdp, U32 u32UdpLength, BOOLEAN bAsync, U32 u32TimeoutInMs)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode = NULL;
   BOOLEAN                bSuccess = FALSE;

   if (NULL != (pMailboxNode = U_GMSG_pAcquireMailboxNodeById(Mailbox, FALSE)))
   {
      if (NULL == altTransport)
      {
         MSG_GMSG_SEND *pMsgSend;
         SEMA           Sema = pMailboxNode->Mailbox.RtxcSema;

         U_GMSG_bReleaseMailboxNode(pMailboxNode, FALSE);
         pMsgSend = (MSG_GMSG_SEND*)malloc(sizeof(MSG_GMSG_SEND));

         if (GMSG_bCompose(&pMsgSend->MsgInfo, NULL, pDstAddress, eMsgId, pDataType, pData, u64UserToken, __u64ReplyToken, u32Flags, bUdp, u32UdpLength))
         {
            pMsgSend->header.Cmd = CMD_ID_GMSG_SEND;
            pMsgSend->Mailbox    = Mailbox;
            pMsgSend->bSuccess   = FALSE;
            pMsgSend->MsgInfo.u32TimeoutInMs = u32TimeoutInMs;

            if (bAsync)
            {
               KS_send(M_GMSG, (RTXCMSG *)pMsgSend, NORMAL_PRIORITY, 0);
               bSuccess = TRUE;
            }
            else
            {
               KS_sendw(M_GMSG, (RTXCMSG *)pMsgSend, NORMAL_PRIORITY, Sema);
               bSuccess = pMsgSend->bSuccess;
               free(pMsgSend);
            }
         }
         else
         {
            printf("[%d]%s: compose failed\n", __LINE__, __FUNCTION__);
            free(pMsgSend);
         }
      }
      else
      {
         MSG_SUDP_OUT_SEND *pMsgSend = (MSG_SUDP_OUT_SEND*)malloc(sizeof(MSG_SUDP_OUT_SEND));

         pMsgSend->header.Cmd = CMD_ID_SUDP_OUT_SEND;
         pMsgSend->u32Address = pDstAddress->uAddr.Socket.u32Address;
         pMsgSend->u32Port = pDstAddress->uAddr.Socket.u32Port;
         pMsgSend->u32Raw = altTransport->bRaw;
         pMsgSend->u32MultiPacket = altTransport->u32MultiPacket;

         if (altTransport->bRaw)
         {
            pMsgSend->ucpMessage = pData;
            pMsgSend->u32MessageLength = altTransport->u32DataLength;
            bSuccess = TRUE;
         }
         else
         {
            GMSG_INFO MsgInfo;
            GMSG_INFO *pMsgInfo = &MsgInfo;

            if (GMSG_bCompose(&MsgInfo, NULL, pDstAddress, eMsgId, pDataType, pData, u64UserToken, __u64ReplyToken, u32Flags, bUdp, u32UdpLength))
            {
               pMsgSend->ucpMessage = (unsigned char*) pMsgInfo->pMsgHeader;
               pMsgSend->u32MessageLength = pMsgInfo->pMsgHeader->u32MsgSize+sizeof(GMSG_HEADER);
               bSuccess = TRUE;
            }
            else
            {
               printf("Compose failed!\n");
            }
         }
         if (bSuccess)
         {
            KS_send(M_SUDPOUT, (RTXCMSG *)pMsgSend, NORMAL_PRIORITY, 0);
         }
         else
         {
            free(pMsgSend);
         }
      }
   }
   return(bSuccess);
}

/******************************************************************************/
/**
 * Send a Request message (expecting a reply)
 *
 * @param Mailbox      RTXC Mailbox
 * @param pDstAddress  ptr to destination address
 * @param eMsgId       Message ID of data payload
 * @param pDataType    Data Type of data payload
 * @param pData        ptr to data of payload
 * @param u64UserToken User token (to match reply)
 *
 * @return BOOLEAN     TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bRequest(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                               const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken)
{
   return(U_GMSG_bSend(Mailbox, pDstAddress, eMsgId, pDataType, pData, u64UserToken,
                       NO_TOKEN, GMSG_FLAG_REQUEST, NULL, FALSE, 0, FALSE, MSG_DEFAULT_TIMEOUT));
}

/******************************************************************************/
/**
 * Reply to a message with another message
 *
 * @param Mailbox   RTXC Mailbox
 * @param pMsgInfo  ptr to original message
 * @param eMsgId    Message ID of reply
 * @param pDataType Data Type of reply
 * @param pData     ptr to data of reply, cast as (void*)
 *
 * @return BOOLEAN  TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bReply(MBOX Mailbox, GMSG_INFO *pMsgInfo, GMSG_ID_TYPE eMsgId,
                             const G_DATA_TYPE *pDataType, void *pData)
{
   return(U_GMSG_bSend(Mailbox, &pMsgInfo->pMsgHeader->SrcAddress, eMsgId,
                       pDataType, pData, pMsgInfo->pMsgHeader->u64Token,
                       pMsgInfo->pMsgHeader->__u64ReplyToken, GMSG_FLAG_REPLY, NULL, FALSE, 0, FALSE, MSG_DEFAULT_TIMEOUT));
}

/******************************************************************************/
/**
 * Send a Message Status Reply
 *
 * @param Mailbox      RTXC Mailbox
 * @param pMsgInfo     ptr to original message
 * @param u32MsgStatus Message Status
 *
 * @return BOOLEAN     TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bReplyWithStatus(MBOX Mailbox, GMSG_INFO *pMsgInfo, U32 u32MsgStatus)
{
   GMSG_STATUS MsgStatus;
   MsgStatus.u32MsgId = pMsgInfo->pMsgHeader->u32MsgId;
   MsgStatus.u32MsgStatus = u32MsgStatus;
   MsgStatus.u64Token = pMsgInfo->pMsgHeader->u64Token;
   return(U_GMSG_bReply(Mailbox,pMsgInfo,GMSG_ID_STATUS,DT_GMSG_STATUS,(void*)&MsgStatus));
}

/******************************************************************************/
/**
 * Send a message without expecting a reply
 *
 * @param Mailbox      RTXC Mailbox
 * @param pDstAddress  Destination Address
 * @param eMsgId       Message ID to send
 * @param pDataType    Data Type
 * @param pData        ptr to data, cast as (void*)
 * @param u64UserToken User Token (NO_TOKEN if don't care)
 *
 * @return BOOLEAN     TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bSendMsgNoReply(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                      const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken)
{
   return(U_GMSG_bSend(Mailbox, pDstAddress, eMsgId, pDataType, pData, u64UserToken,
                       NO_TOKEN, 0, NULL, FALSE, 0, FALSE, MSG_DEFAULT_TIMEOUT));
}

/******************************************************************************/
/**
 * Send a message without expecting a reply
 *
 * @param Mailbox      RTXC Mailbox
 * @param pDstAddress  Destination Address
 * @param eMsgId       Message ID to send
 * @param pDataType    Data Type
 * @param pData        ptr to data, cast as (void*)
 * @param u64UserToken User Token (NO_TOKEN if don't care)
 *
 * @return BOOLEAN     TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bSendMsgNoWait(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                      const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken, U32 u32TimeoutInMs)
{
   return(U_GMSG_bSend(Mailbox, pDstAddress, eMsgId, pDataType, pData, u64UserToken,
                       NO_TOKEN, 0, NULL, FALSE, 0, TRUE, u32TimeoutInMs));
}

/******************************************************************************/
/**
 * Send a message within the provided timeout period
 * 
 * @param Mailbox      RTXC Mailbox
 * @param pDstAddress  Destination Address
 * @param eMsgId       Message ID to send
 * @param pDataType    Data Type
 * @param pData        ptr to data, cast as (void*)
 * @param u64UserToken User Token (NO_TOKEN if don't care)
 * @param u32TimeoutInMs timeout in ms
 * 
 * @return BOOLEAN     TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bSendMsgNoWaitAsync(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                      const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken, U32 u32TimeoutInMs)
{
   return(U_GMSG_bSend(Mailbox, pDstAddress, eMsgId, pDataType, pData, u64UserToken,
                       NO_TOKEN, 0, NULL, FALSE, 0, FALSE, u32TimeoutInMs));
}


/******************************************************************************/
/**
 * Send a message without expecting a reply
 *
 * @param Mailbox      RTXC Mailbox
 * @param pDstAddress  Destination Address
 * @param eMsgId       Message ID to send
 * @param pDataType    Data Type
 * @param pData        ptr to data, cast as (void*)
 * @param u64UserToken User Token (NO_TOKEN if don't care)
 *
 * @return BOOLEAN     TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bSendMsgNoReplyAlt(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                         const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken, GMSG_ALT_TRANSPORT *altTransport)
{
   return(U_GMSG_bSend(Mailbox, pDstAddress, eMsgId, pDataType, pData, u64UserToken,
                       NO_TOKEN, 0, altTransport, FALSE, 0, FALSE, MSG_DEFAULT_TIMEOUT));
}

/******************************************************************************/
/**
 * Send a message without expecting a reply
 *
 * @param Mailbox      RTXC Mailbox
 * @param pDstAddress  Destination Address
 * @param pData        ptr to data, cast as (void*)
 * @param u64UserToken User Token (NO_TOKEN if don't care)
 *
 * @return BOOLEAN     TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bSendUdpMsgNoReply(MBOX Mailbox, GMSG_ADDRESS *pDstAddress, GMSG_ID_TYPE eMsgId,
                                      const G_DATA_TYPE *pDataType, void *pData, U64 u64UserToken, U32 u32MsgLength)
{
   return(U_GMSG_bSend(Mailbox, pDstAddress, (GMSG_ID_TYPE)0, NULL, pData, u64UserToken,
                       NO_TOKEN, 0, NULL, TRUE, u32MsgLength, FALSE, MSG_DEFAULT_TIMEOUT ));
}

/******************************************************************************/
/**
 * Disconnect a unused socket to an address
 *
 * @param Mailbox  Task Mailbox
 * @param pAddress Address of socket to disconnect
 *
 * @return BOOLEAN TRUE if socket was disconnect.
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bDisconnect(MBOX Mailbox, GMSG_ADDRESS *pAddress)
{
   GMSG_REG_MAILBOX_NODE  *pMailboxNode;
   GMSG_SOCKET_NODE       *pSocketNode;
   BOOLEAN                 bSuccess = FALSE;

   if (NULL != (pMailboxNode = U_GMSG_pAcquireMailboxNodeById(Mailbox, FALSE)))
   {
      if (NULL != (pSocketNode = U_GMSG_pAcquireSocketNodeByAddress(&pMailboxNode->Mailbox, pAddress, FALSE)))
      {
         bSuccess = U_GMSG_bDeleteSocketNode(&pMailboxNode->Mailbox, pSocketNode);
         U_GMSG_bReleaseSocketNode(&pMailboxNode->Mailbox, pSocketNode, FALSE);
      }
      U_GMSG_bReleaseMailboxNode(pMailboxNode, FALSE);
   }
   return(bSuccess);
}

/******************************************************************************/
/**
 * Shudown Connections and Mailboxes
 */
/******************************************************************************/
GLOBAL void U_GMSG_vShutdown(void)
{
   GMSG_REG_MAILBOX_NODE  *pMailboxNode;
   GMSG_SOCKET_NODE       *pSocketNode;

   KS_lockw(RES_GMSG_MBOX_LIST);
   pMailboxNode = s_RegMailboxList.pHead;
   while (pMailboxNode != NULL)
   {
      GMSG_REG_MAILBOX_NODE  *pMailboxNodeForDelete = pMailboxNode;
      pMailboxNode = pMailboxNode->pNext;
      MUTEX_bLock(&pMailboxNodeForDelete->Mailbox.MsgSocketInfo.SocketInfoMutex);
      pSocketNode = pMailboxNodeForDelete->Mailbox.MsgSocketInfo.pHead;
      while (pSocketNode != NULL)
      {
         GMSG_SOCKET_NODE *pSocketNodeForDelete = pSocketNode;
         pSocketNode = pSocketNode->pNext;
         U_GMSG_bDeleteSocketNode(&pMailboxNodeForDelete->Mailbox, pSocketNodeForDelete);
      }
      MUTEX_bUnlock(&pMailboxNodeForDelete->Mailbox.MsgSocketInfo.SocketInfoMutex);
      U_GMSG_bDeleteMailboxNode(pMailboxNodeForDelete);
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
}

/******************************************************************************/
/**
 *
 */
/******************************************************************************/
GLOBAL void U_GMSG_vOnTimer(void)
{
   GMSG_REG_MAILBOX_NODE  *pMailboxNode;
   GMSG_REQUEST_NODE      *pRequestNode;

   KS_lockw(RES_GMSG_MBOX_LIST);
   pMailboxNode = s_RegMailboxList.pHead;
   while (pMailboxNode != NULL)
   {
      GMSG_bMessageTimeout(&pMailboxNode->Mailbox);
      pMailboxNode = pMailboxNode->pNext;
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   while (NULL != (pMailboxNode = U_GMSG_pAcquireMailboxNodeWithExpiredRequest(&pRequestNode)))
   {
      GMSG_STATUS MsgStatus;
      MsgStatus.u32MsgId = pRequestNode->MsgRequest.u32MsgId;
      MsgStatus.u32MsgStatus = GMSG_STATUS_TIMEOUT;
      MsgStatus.u64Token = pRequestNode->MsgRequest.u64Token;
      U_GMSG_bSend(pMailboxNode->Mailbox.RtxcMailbox, &pMailboxNode->Mailbox.MsgAddress, GMSG_ID_STATUS, DT_GMSG_STATUS,
                   &MsgStatus, pRequestNode->MsgRequest.u64Token, pRequestNode->MsgRequest.__u64ReplyToken, GMSG_FLAG_REPLY,
                   NULL, FALSE, 0, TRUE, MSG_DEFAULT_TIMEOUT);
      U_GMSG_bReleaseMailboxNode(pMailboxNode, FALSE);
   }
}

/******************************************************************************/
/**
 * Add A Mailbox node to the mailbox list
 *
 * @param pMailboxNode node to add
 *
 * @return BOOLEAN TRUE on success
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bAddMailboxNode(GMSG_REG_MAILBOX_NODE *pMailboxNode)
{
   KS_lockw(RES_GMSG_MBOX_LIST);
   pMailboxNode->pNext = s_RegMailboxList.pHead;
   s_RegMailboxList.pHead = pMailboxNode;
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(TRUE);
}

/******************************************************************************/
/**
 * Add a new Mailbox node
 *
 * @param pMsgMapInfo message map
 * @param u16Port     port number
 * @param Mailbox     RTXC Mailbox
 * @param ResLock     Mailbox Rource Lock
 *
 * @return GMSG_SOCKET_NODE* Node created
 */
/******************************************************************************/
GLOBAL GMSG_REG_MAILBOX_NODE *U_GMSG_pAddNewMailboxNode(GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port,
                                                        MBOX Mailbox, RESOURCE ResLock, SEMA Sema, BOOLEAN bUdp, U32 u32McIp)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode = (GMSG_REG_MAILBOX_NODE*)malloc(sizeof(GMSG_REG_MAILBOX_NODE));

   if (!GMSG_bInitSocketMailbox(&pMailboxNode->Mailbox, pMsgMapInfo, u16Port, GMSG_DEFAULT_BACKLOG,
                                Mailbox, ResLock, Sema, bUdp, u32McIp) || !U_GMSG_bAddMailboxNode(pMailboxNode))
   {
      free(pMailboxNode);
      pMailboxNode = NULL;
   }

   return(pMailboxNode);
}

/******************************************************************************/
/**
 * Delete a Mailox node
 *
 * @param pMailboxNode node to delete
 *
 * @return BOOLEAN TRUE on success (found and deleted)
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bDeleteMailboxNode(GMSG_REG_MAILBOX_NODE *pMailboxNode)
{
   GMSG_REG_MAILBOX_NODE   *pNodeToDelete = NULL;

   KS_lockw(RES_GMSG_MBOX_LIST);
   if (s_RegMailboxList.pHead != NULL)
   {
      if (pMailboxNode->Mailbox.nReading > 0 || pMailboxNode->Mailbox.nAccess > 0)
      {
         pMailboxNode->Mailbox.bMarkForDelete = TRUE;
      }
      else if (s_RegMailboxList.pHead == pMailboxNode)
      {
         pNodeToDelete = s_RegMailboxList.pHead;
         s_RegMailboxList.pHead = s_RegMailboxList.pHead->pNext;
      }
      else
      {
         GMSG_REG_MAILBOX_NODE *pNode = s_RegMailboxList.pHead;

         while (pNode->pNext != NULL)
         {
            if (pNode->pNext == pMailboxNode)
            {
               pNodeToDelete = pNode->pNext;
               pNode->pNext = pNode->pNext->pNext;
               break;
            }
            pNode = pNode->pNext;
         }
      }
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   if (pNodeToDelete != NULL)
   {
      GMSG_bCloseMailbox(&pNodeToDelete->Mailbox);
      free(pNodeToDelete);
      return(TRUE);
   }
   else
   {
      return(FALSE);
   }
}

/******************************************************************************/
/**
 * Delete a Mailbox Node by its Mailbox ID
 *
 * @param Mailbox RTXC Mailbox ID
 *
 * @return BOOLEAN TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bDeleteMailboxNodeById(MBOX Mailbox)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode;
   BOOLEAN bReturn = FALSE;

   KS_lockw(RES_GMSG_MBOX_LIST);
   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeById(Mailbox)))
   {
      bReturn = U_GMSG_bDeleteMailboxNode(pMailboxNode);
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(bReturn);
}


/******************************************************************************/
/**
 * Get's the Address of a Mailbox by Mailbox ID
 *
 * @param pAddress ptr to output Address
 * @param Mailbox  RTXC Mailbox ID
 *
 * @return BOOLEAN TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bGetMailboxAddress(GMSG_ADDRESS *pAddress, MBOX Mailbox)
{
   GMSG_REG_MAILBOX_NODE  *pMailboxNode;

   KS_lockw(RES_GMSG_MBOX_LIST);
   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeById(Mailbox)))
   {
      if (pAddress != NULL)
      {
         *pAddress = pMailboxNode->Mailbox.MsgAddress;
      }
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(pMailboxNode ? TRUE : FALSE);
}

/******************************************************************************/
/**
 * Search for a Mailbox node by RTXC Mailbox ID
 *
 * @param Mailbox RTXC Mailbox ID
 *
 * @return GMSG_REG_MAILBOX_NODE* found node or NULL if not found
 */
/******************************************************************************/
GLOBAL GMSG_REG_MAILBOX_NODE *U_GMSG_pFindMailboxNodeById(MBOX Mailbox)
{
   GMSG_REG_MAILBOX_NODE  *pMailboxNode;

   KS_lockw(RES_GMSG_MBOX_LIST);
   pMailboxNode = s_RegMailboxList.pHead;
   while (pMailboxNode != NULL)
   {
      if (!pMailboxNode->Mailbox.bMarkForDelete && (Mailbox == pMailboxNode->Mailbox.RtxcMailbox))
      {
         break;
      }
      pMailboxNode = pMailboxNode->pNext;
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(pMailboxNode);
}

/******************************************************************************/
/**
 * Find Mailbox Node by file descriptor
 *
 * @param nSocket socket fd
 * @param pbListenSocket set to true if this is the mailbox listening socket
 *
 * @return GMSG_REG_MAILBOX_NODE* found node or NULL if not found
 */
/******************************************************************************/
GLOBAL GMSG_REG_MAILBOX_NODE *U_GMSG_pFindMailboxNodeByFd(int nSocket, BOOLEAN *pbListenSocket)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode;

   KS_lockw(RES_GMSG_MBOX_LIST);
   pMailboxNode = s_RegMailboxList.pHead;
   while (pMailboxNode != NULL)
   {
      if (!pMailboxNode->Mailbox.bMarkForDelete)
      {
         if (pMailboxNode->Mailbox.MsgObject.Socket.nSocket == nSocket)
         {
            *pbListenSocket = TRUE;
            break;
         }
         if (NULL != U_GMSG_pFindSocketNodeByFd(&pMailboxNode->Mailbox, nSocket))
         {
            *pbListenSocket = FALSE;
            break;
         }
      }
      pMailboxNode = pMailboxNode->pNext;
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(pMailboxNode);
}

/******************************************************************************/
/**
 *
 *
 * @return GMSG_REG_MAILBOX_NODE*
 */
/******************************************************************************/
GLOBAL GMSG_REG_MAILBOX_NODE *U_GMSG_pFindMailboxNodeWithExpiredRequest(GMSG_REQUEST_NODE **ppRequestNode)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode;
   *ppRequestNode = NULL;

   KS_lockw(RES_GMSG_MBOX_LIST);
   pMailboxNode = s_RegMailboxList.pHead;
   while (pMailboxNode != NULL)
   {
      if (!pMailboxNode->Mailbox.bMarkForDelete)
      {
         if (NULL != (*ppRequestNode = GMSG_pFindExpiredRequest(&pMailboxNode->Mailbox.MsgRequestInfo)))
         {
            break;
         }
      }
      pMailboxNode = pMailboxNode->pNext;
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(pMailboxNode);
}

/******************************************************************************/
/**
 *
 *
 * @param Mailbox
 *
 * @return GMSG_REG_MAILBOX_NODE*
 */
/******************************************************************************/
GLOBAL GMSG_REG_MAILBOX_NODE *U_GMSG_pAcquireMailboxNodeById(MBOX Mailbox, BOOLEAN bReading)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode;

   KS_lockw(RES_GMSG_MBOX_LIST);
   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeById(Mailbox)))
   {
      if (bReading)
      {
         pMailboxNode->Mailbox.nReading++;
      }
      else
      {
         pMailboxNode->Mailbox.nAccess++;
      }
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(pMailboxNode);
}

/******************************************************************************/
/**
 * Acquire a Mailbox node (won't be deleted until released) by file descriptor
 *
 * @param nSocket        socket fd
 * @param pbListenSocket set to true if this is the mailbox listening socket
 *
 * @return GMSG_REG_MAILBOX_NODE* acquired node or NULL if not found
 */
/******************************************************************************/
GLOBAL GMSG_REG_MAILBOX_NODE *U_GMSG_pAcquireMailboxNodeByFd(int nSocket, BOOLEAN *pbListenSocket, BOOLEAN bReading)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode;

   KS_lockw(RES_GMSG_MBOX_LIST);
   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeByFd(nSocket, pbListenSocket)))
   {
      if (bReading)
      {
         pMailboxNode->Mailbox.nReading++;
      }
      else
      {
         pMailboxNode->Mailbox.nAccess++;
      }
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(pMailboxNode);
}

/******************************************************************************/
/**
 *
 *
 * @return GMSG_REG_MAILBOX_NODE*
 */
/******************************************************************************/
GLOBAL GMSG_REG_MAILBOX_NODE *U_GMSG_pAcquireMailboxNodeWithExpiredRequest(GMSG_REQUEST_NODE **ppRequestNode)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode;

   KS_lockw(RES_GMSG_MBOX_LIST);
   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeWithExpiredRequest(ppRequestNode)))
   {
      pMailboxNode->Mailbox.nAccess++;
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(pMailboxNode);
}

/******************************************************************************/
/**
 *
 *
 * @param pMailboxNode
 * @param bReading
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bAcquireMailboxNode(GMSG_REG_MAILBOX_NODE *pMailboxNode, BOOLEAN bReading)
{
   KS_lockw(RES_GMSG_MBOX_LIST);
   if (bReading)
   {
      pMailboxNode->Mailbox.nReading++;
   }
   else
   {
      pMailboxNode->Mailbox.nAccess++;
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(TRUE);
}


/******************************************************************************/
/**
 * Release a Mailbox node that was acquired
 *
 * @param pMailboxNode Mailbox Node to release
 *
 * @return BOOLEAN TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bReleaseMailboxNode(GMSG_REG_MAILBOX_NODE *pMailboxNode, BOOLEAN bReading)
{
   BOOLEAN bReturn = TRUE;

   if (!pMailboxNode)
      return(bReturn);

   KS_lockw(RES_GMSG_MBOX_LIST);
   if (bReading)
   {
      pMailboxNode->Mailbox.nReading--;
   }
   else
   {
      pMailboxNode->Mailbox.nAccess--;
   }
   if (pMailboxNode->Mailbox.bMarkForDelete && (pMailboxNode->Mailbox.nReading <= 0) && (pMailboxNode->Mailbox.nAccess <= 0))
   {
      bReturn = U_GMSG_bDeleteMailboxNode(pMailboxNode);
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(bReturn);
}

/******************************************************************************/
/**
 *
 *
 * @param Mailbox
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bSignal(MBOX Mailbox)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode;
   BOOLEAN bReturn = FALSE;

   KS_lockw(RES_GMSG_MBOX_LIST);

   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeById(Mailbox)))
   {
      if (pMailboxNode->Mailbox.nReading == 0)
      {
         KS_signal(SEMA_GMSG_SELECT);
         bReturn = TRUE;
      }
   }
   else
   {
      KS_signal(SEMA_GMSG_SELECT);
      bReturn = TRUE;
   }

   KS_unlock(RES_GMSG_MBOX_LIST);
   return(bReturn);
}

/******************************************************************************/
/**
 * Creates a File Descriptor Set for all mailboxes and their sockets
 *
 * @param pReadFdSet  ptr to Read FD set
 * @param pnMaxSocket ptr to max socket (input/output)
 *
 * @return BOOLEAN TRUE if at least 1 socket in set
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bGetMailboxFdSet(fd_set *pReadFdSet, int *pnMaxSocket)
{
   GMSG_REG_MAILBOX_NODE  *pMailboxNode;
   BOOLEAN                 bReturn = FALSE;

   KS_lockw(RES_GMSG_MBOX_LIST);
   if (s_RegMailboxList.pHead != NULL)
   {
      pMailboxNode = s_RegMailboxList.pHead;
      while (pMailboxNode != NULL)
      {
         if (pMailboxNode->Mailbox.nReading <= 0)
         {
            FD_SET( pMailboxNode->Mailbox.MsgObject.Socket.nSocket, pReadFdSet );
            if (pMailboxNode->Mailbox.MsgObject.Socket.nSocket > *pnMaxSocket)
            {
               *pnMaxSocket = pMailboxNode->Mailbox.MsgObject.Socket.nSocket;
            }
            (void)U_GMSG_bGetSocketInfoFdSet(pMailboxNode, pReadFdSet, pnMaxSocket);
            bReturn = TRUE;
         }
         pMailboxNode = pMailboxNode->pNext;
      }
   }
   KS_unlock(RES_GMSG_MBOX_LIST);
   return(bReturn);
}

/******************************************************************************/
/**
 * Add a socket node to a mailbox's list of sockets
 *
 * @param pMailbox    Mailbox to add socket node to
 * @param pSocketNode socket node to add
 *
 * @return BOOLEAN    TRUE on success
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bAddSocketNode(GMSG_MAILBOX *pMailbox, GMSG_SOCKET_NODE *pSocketNode)
{
   MUTEX_bLock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   pSocketNode->pNext = pMailbox->MsgSocketInfo.pHead;
   pMailbox->MsgSocketInfo.pHead = pSocketNode;
   MUTEX_bUnlock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   return(TRUE);
}

/******************************************************************************/
/**
 * Allocate and add a new socket node with the specified parameters
 *
 * @param pMailbox Mailbox to add socket node to
 * @param pAddress Address of Socket (NULL if accepting from mbox listen socket)
 * @param bReading Acquire the socket node for reading (!writing)
 *
 * @return GMSG_SOCKET_NODE* newly added socket node (or NULL on failure)
 */
/******************************************************************************/
GLOBAL GMSG_SOCKET_NODE *U_GMSG_pAddNewSocketNode(GMSG_MAILBOX *pMailbox, GMSG_ADDRESS *pAddress, BOOLEAN bReading, BOOLEAN bAcquire)
{
   GMSG_SOCKET_NODE *pSocketNode = (GMSG_SOCKET_NODE*)malloc(sizeof(GMSG_SOCKET_NODE));
   memset(pSocketNode, 0, sizeof(GMSG_SOCKET_NODE));
   if (pAddress == NULL)
   {
      if (!GMSG_bAcceptMessage(pMailbox, &pSocketNode->MsgSocket.Socket))
      {
         free(pSocketNode);
         return(NULL);
      }
   }
   else
   {
      if (!GMSG_bNewSocket(&pSocketNode->MsgSocket.Socket, pAddress))
      {
         free(pSocketNode);
         return(NULL);
      }

      pSocketNode->MsgSocket.MsgAddress = *pAddress;

      if (GMSG_bAddressEqual(&pMailbox->MsgAddress, pAddress))
      {
         pSocketNode->bWriteOnly = TRUE;
      }
   }
   if (bAcquire)
   {
      if (bReading)
      {
         pSocketNode->nReading = 1;
      }
      else
      {
         pSocketNode->nWriting = 1;
      }
   }
   if (!U_GMSG_bAddSocketNode(pMailbox, pSocketNode))
   {
      free(pSocketNode);
      return(NULL);
   }
   return(pSocketNode);
}

/******************************************************************************/
/**
 * Allocate and add a new socket node with the specified parameters
 *
 * @param pMailbox Mailbox to add socket node to
 * @param pAddress Address of Socket (NULL if accepting from mbox listen socket)
 * @param bReading Acquire the socket node for reading (!writing)
 *
 * @return GMSG_SOCKET_NODE* newly added socket node (or NULL on failure)
 */
/******************************************************************************/
GLOBAL GMSG_SOCKET_NODE *U_GMSG_pAddNewSocketNodeUdp(GMSG_MAILBOX *pMailbox, GMSG_ADDRESS *pAddress, BOOLEAN bReading, BOOLEAN bAcquire)
{
   GMSG_SOCKET_NODE *pSocketNode = (GMSG_SOCKET_NODE*)malloc(sizeof(GMSG_SOCKET_NODE));
   memset(pSocketNode, 0, sizeof(GMSG_SOCKET_NODE));

   pSocketNode->MsgSocket.Socket.nSocket             = pMailbox->MsgObject.Socket.nSocket;
   pSocketNode->MsgSocket.Socket.Address             = pMailbox->MsgObject.Socket.Address;
   pSocketNode->MsgSocket.Socket.bListen             = FALSE;
   pSocketNode->MsgSocket.Socket.bNonBlocking        = TRUE;
   pSocketNode->MsgSocket.Socket.nBacklog            = 0;

   if (bAcquire)
   {
      if (bReading)
      {
         pSocketNode->nReading = 1;
      }
      else
      {
         pSocketNode->nWriting = 1;
      }
   }
   if (!U_GMSG_bAddSocketNode(pMailbox, pSocketNode))
   {
      free(pSocketNode);
      return(NULL);
   }

   return(pSocketNode);
}

/******************************************************************************/
/**
 *
 *
 * @param pMailbox
 * @param pSocketNode
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bDeleteSocketNode(GMSG_MAILBOX *pMailbox, GMSG_SOCKET_NODE *pSocketNode)
{
   GMSG_SOCKET_NODE *pNode;
   BOOLEAN bReturn = FALSE;
   BOOLEAN bDelete = FALSE;

   MUTEX_bLock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   if (pSocketNode->nReading > 0 || pSocketNode->nWriting > 0)
   {
      pSocketNode->bMarkForDelete = TRUE;
      bReturn = TRUE;
   }
   else if (pSocketNode == pMailbox->MsgSocketInfo.pHead)
   {
      pMailbox->MsgSocketInfo.pHead = pMailbox->MsgSocketInfo.pHead->pNext;
      bReturn = TRUE;
      bDelete = TRUE;
   }
   else if (pMailbox->MsgSocketInfo.pHead != NULL)
   {
      pNode = pMailbox->MsgSocketInfo.pHead;
      while (pNode->pNext != NULL)
      {
         if (pSocketNode == pNode->pNext)
         {
            pNode->pNext = pNode->pNext->pNext;
            bReturn = TRUE;
            bDelete = TRUE;
            break;
         }
         pNode = pNode->pNext;
      }
   }
   MUTEX_bUnlock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   if (bDelete)
   {
      MSG_GMSG_DISCONNECT *pMsgDisconnect = (MSG_GMSG_DISCONNECT*)malloc(sizeof(MSG_GMSG_DISCONNECT));

      pMsgDisconnect->header.Cmd = CMD_ID_GMSG_DISCONNECT;
      pMsgDisconnect->Mailbox = pMailbox->RtxcMailbox;
      pMsgDisconnect->Address = pSocketNode->MsgSocket.MsgAddress;
      KS_send(pMailbox->RtxcMailbox, (RTXCMSG*)pMsgDisconnect, NORMAL_PRIORITY, (SEMA)0);
      GSOCKET_bClose(&pSocketNode->MsgSocket.Socket);
      free(pSocketNode);
   }
   return(bReturn);
}

/******************************************************************************/
/**
 *
 *
 * @param pMailbox
 * @param nSocket
 *
 * @return GMSG_SOCKET_NODE*
 */
/******************************************************************************/
GLOBAL GMSG_SOCKET_NODE *U_GMSG_pFindSocketNodeByFd(GMSG_MAILBOX *pMailbox, int nSocket)
{
   GMSG_SOCKET_NODE       *pSocketNode;

   MUTEX_bLock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   pSocketNode = pMailbox->MsgSocketInfo.pHead;
   while (pSocketNode != NULL)
   {
      if (!pSocketNode->bMarkForDelete && pSocketNode->MsgSocket.Socket.nSocket == nSocket)
      {
         break;
      }
      pSocketNode = pSocketNode->pNext;
   }
   MUTEX_bUnlock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   return (pSocketNode);
}

/******************************************************************************/
/**
 *
 *
 * @param pMailbox
 * @param pAddress
 * @param bReading
 *
 * @return GMSG_SOCKET_NODE*
 */
/******************************************************************************/
GLOBAL GMSG_SOCKET_NODE *U_GMSG_pFindSocketNodeByAddress(GMSG_MAILBOX *pMailbox, GMSG_ADDRESS *pAddress, BOOLEAN bReading)
{
   GMSG_SOCKET_NODE       *pSocketNode;

   MUTEX_bLock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   pSocketNode = pMailbox->MsgSocketInfo.pHead;
   while (pSocketNode != NULL)
   {
      if (!pSocketNode->bMarkForDelete && ((bReading && !pSocketNode->bWriteOnly) || (!bReading && !pSocketNode->bReadOnly) )
          && GMSG_bAddressEqual(&pSocketNode->MsgSocket.MsgAddress, pAddress))
      {
         break;
      }
      pSocketNode = pSocketNode->pNext;
   }
   MUTEX_bUnlock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   return (pSocketNode);
}

/******************************************************************************/
/**
 *
 *
 * @param pMailbox Mailbox to find socket
 * @param nSocket  Socket File Descriptor
 * @param bReading TRUE/Reading, FALSE/Writing
 *
 * @return GMSG_SOCKET_NODE*
 */
/******************************************************************************/
GLOBAL GMSG_SOCKET_NODE *U_GMSG_pAcquireSocketNodeByFd(GMSG_MAILBOX *pMailbox, int nSocket, BOOLEAN bReading)
{
   GMSG_SOCKET_NODE       *pSocketNode;

   MUTEX_bLock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   if (NULL != (pSocketNode = U_GMSG_pFindSocketNodeByFd(pMailbox, nSocket)))
   {
      if (bReading)
      {
         pSocketNode->nReading++;
      }
      else
      {
         pSocketNode->nWriting++;
      }
   }
   MUTEX_bUnlock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   return (pSocketNode);
}

/******************************************************************************/
/**
 *
 *
 * @param pMailbox
 * @param pAddress
 * @param bReading TRUE/Reading, FALSE/Writing
 *
 * @return GMSG_SOCKET_NODE*
 */
/******************************************************************************/
GLOBAL GMSG_SOCKET_NODE *U_GMSG_pAcquireSocketNodeByAddress(GMSG_MAILBOX *pMailbox, GMSG_ADDRESS *pAddress, BOOLEAN bReading)
{
   GMSG_SOCKET_NODE       *pSocketNode;

   MUTEX_bLock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   if (NULL != (pSocketNode = U_GMSG_pFindSocketNodeByAddress(pMailbox, pAddress, bReading)))
   {
      if (bReading)
      {
         pSocketNode->nReading++;
      }
      else
      {
         pSocketNode->nWriting++;
      }
   }
   MUTEX_bUnlock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   return (pSocketNode);
}

/******************************************************************************/
/**
 *
 *
 * @param pSocketNode
 * @param pAddress
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bUpdateSocketNodeAddress(GMSG_MAILBOX *pMailbox, GMSG_SOCKET_NODE *pSocketNode, GMSG_ADDRESS *pAddress)
{
   MUTEX_bLock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   pSocketNode->MsgSocket.MsgAddress = *pAddress;
   if (GMSG_bAddressEqual(&pMailbox->MsgAddress, pAddress))
   {
      pSocketNode->bReadOnly = TRUE;
   }
   pSocketNode->bAddressSet = TRUE;
   MUTEX_bUnlock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   return(TRUE);
}

/******************************************************************************/
/**
 *
 *
 * @param pMailbox
 * @param pSocketNode
 * @param bWasReading
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bReleaseSocketNode(GMSG_MAILBOX *pMailbox, GMSG_SOCKET_NODE *pSocketNode, BOOLEAN bWasReading)
{
   BOOLEAN bReturn = TRUE;

   MUTEX_bLock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   if (bWasReading)
   {
      pSocketNode->nReading--;
   }
   else
   {
      pSocketNode->nWriting--;
   }
   if (pSocketNode->bMarkForDelete && (pSocketNode->nReading <= 0) && (pSocketNode->nWriting <= 0))
   {
      bReturn = U_GMSG_bDeleteSocketNode(pMailbox, pSocketNode);
   }
   MUTEX_bUnlock(&pMailbox->MsgSocketInfo.SocketInfoMutex);
   return(bReturn);
}

/******************************************************************************/
/**
 *
 *
 * @param pMailboxNode
 * @param pReadFdSet
 * @param pnMaxSocket
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN U_GMSG_bGetSocketInfoFdSet(GMSG_REG_MAILBOX_NODE *pMailboxNode, fd_set *pReadFdSet, int *pnMaxSocket)
{
   GMSG_SOCKET_NODE       *pSocketNode;
   BOOLEAN                 bReturn = FALSE;

   MUTEX_bLock(&pMailboxNode->Mailbox.MsgSocketInfo.SocketInfoMutex);
   if (pMailboxNode->Mailbox.MsgSocketInfo.pHead != NULL)
   {
      pSocketNode = pMailboxNode->Mailbox.MsgSocketInfo.pHead;
      while (pSocketNode != NULL)
      {
         //add this node to fd set if we aren't currently reading from it
         if (!pSocketNode->bMarkForDelete && pSocketNode->nReading <= 0 && !pSocketNode->bWriteOnly)
         {
            FD_SET( pSocketNode->MsgSocket.Socket.nSocket, pReadFdSet );
            if (pSocketNode->MsgSocket.Socket.nSocket > *pnMaxSocket)
            {
               *pnMaxSocket = pSocketNode->MsgSocket.Socket.nSocket;
            }
            bReturn = TRUE;
         }
         pSocketNode = pSocketNode->pNext;
      }
   }
   MUTEX_bUnlock(&pMailboxNode->Mailbox.MsgSocketInfo.SocketInfoMutex);
   return(bReturn);
}
