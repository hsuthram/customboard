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
//#include "compile_flags.h"
//#define GMSG_TRACE
/********************           INCLUDE FILES                ******************/
#include "datatype.h"
#include "database_msg.h"
#include "gsocket.h"
#include "error.h"
#include "service.h"
#include "u_gmsg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#if MODEL_HAS_NETWORKING
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define GMSG_SEND_THREAD_HASH_SIZE        23

#define GMSG_SIGNATURE                    "GMSG"

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/

/********************        FUNCTION PROTOTYPES             ******************/
PRIVATE GMSG_MAP_INFO    * GMSG_pGetMsgMapInfo(GMSG_MAP_INFO *pMsgMapInfoArray, GMSG_INFO *pMsgInfo);
#ifndef __RTXC__
PRIVATE GMSG_FILTER_NODE * GMSG_pFindMsgFilter(GMSG_FILTER_INFO *pMsgFilterInfo, GMSG_ID_TYPE eMsgId);
#endif

/********************          LOCAL VARIABLES               ******************/
PRIVATE GMSG_MAP_INFO s_DefaultMspMapInfo = {GMSG_ID_DEFAULT_UNKNOWN, GMSG_bDefaultMsgHandler, NULL, NULL, NULL};

#ifdef __RTXC__
PRIVATE U64          s_u64Token;
#else
//send thread pool
PRIVATE GTHREAD_POOL s_SendThreadPool;
PRIVATE BOOLEAN      s_bSendThreadPoolInit = FALSE;
PRIVATE GMUTEX       s_SendThreadPoolMutex = STATIC_MUTEX_INITIALIZER;
#endif

/********************          GLOBAL VARIABLES              ******************/
DT_STRUCT_TYPE(GLOBAL, DT_GMSG_STATUS, 1, NULL, NULL, sizeof(GMSG_STATUS), NULL, DT_UNSIGNED32, DT_UNSIGNED32, DT_UNSIGNED64);
DT_STRUCT_TYPE(GLOBAL, DT_GSOCKET_ADDRESS, 1, NULL, NULL, sizeof(GSOCKET_ADDRESS), NULL, DT_UNSIGNED32, DT_UNSIGNED32,
               DT_UNSIGNED32, DT_UNSIGNED32);
DT_STRUCT_TYPE(GLOBAL, DT_GMSG_ADDRESS, 1, NULL, NULL, sizeof(GMSG_ADDRESS), NULL, DT_UNSIGNED32, DT_GSOCKET_ADDRESS);

/********************              FUNCTIONS                 ******************/

/******************************************************************************
 *
 *    Function: GMSG_bDefaultMsgHandler
 *
 *    Args:    pMsgInfo - Message Information
 *             pvParam1 - Not used
 *             pvParam2 - Not used
 *
 *    Return:  BOOLEAN - TRUE on Handled
 *
 *    Purpose: Default Message Handler
 *
 *    Notes:   Called for unhandled messages
 *
 ******************************************************************************/
#ifdef __RTXC__
GLOBAL BOOLEAN GMSG_bDefaultMsgHandler(MBOX Mailbox, GMSG_INFO *pMsgInfo, void *pvParam1, void *pvParam2)
#else
GLOBAL BOOLEAN GMSG_bDefaultMsgHandler(GMSG_MAILBOX *pMsgMailbox, GMSG_INFO *pMsgInfo, void *pvParam1, void *pvParam2)
#endif
{
   SERVICE_CLASS_TYPE eServiceClass = (SERVICE_CLASS_TYPE)GMSG_SERVICE_CLASS(pMsgInfo->pMsgHeader->u32MsgId);
   int nOffset = (int)GMSG_ID_OFFSET(pMsgInfo->pMsgHeader->u32MsgId);

   //only reply with unhandled status when the orignal message is a request which expects a response
   if (pMsgInfo->pMsgHeader->u32MsgFlags & GMSG_FLAG_REQUEST)
   {
      U_GMSG_bReplyWithStatus(Mailbox, pMsgInfo, GMSG_STATUS_UNHANDLED);
   }
   printf("Unhandled Message - Class: %s, Offset: %d", s_cServiceClass[eServiceClass], nOffset);

   return( TRUE ); //we handled it (kind of)
}


/******************************************************************************
 *
 *    Function: GMSG_pFindRequest
 *
 *    Args:    pMsgRequestInfo - message request information
 *             __u64ReplyToken - private gmsg token to identify message
 *
 *    Return:  ptr to GMSG_REQUEST_NODE
 *
 *    Purpose: Find a message request node by the token it
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL GMSG_REQUEST_NODE * GMSG_pFindRequest(GMSG_REQUEST_INFO *pMsgRequestInfo, U64 __u64ReplyToken)
{
   GMSG_REQUEST_NODE *pNode;

   MUTEX_bLock(&pMsgRequestInfo->RequestMutex);
   pNode = pMsgRequestInfo->pHead;
   while (pNode != NULL && pNode->MsgRequest.__u64ReplyToken != __u64ReplyToken)
   {
      pNode = pNode->pNext;
   }
   MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
   return(pNode);
}

/******************************************************************************
 *
 *    Function: GMSG_bAddRequest
 *
 *    Args:    pMsgRequestInfo - Message request info
 *             pMsgInfo        - Messag information
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Add a request to the request timeout list
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bAddRequest(GMSG_REQUEST_INFO *pMsgRequestInfo, GMSG_INFO *pMsgInfo)
{
   GMSG_REQUEST_NODE  *pNewNode, *pNode;
#ifndef __RTXC__
   GMSG_STATUS         MsgStatus;
#endif
   int                 nRemaining, nSize;

   MUTEX_bLock(&pMsgRequestInfo->RequestMutex);
#ifdef GMSG_TRACE
   TRACE("Add Request- MSG_ID: %08X (Class: %s, Offset %d), User Token: 0x%016llX, Token: 0x%016llX", pMsgInfo->pMsgHeader->u32MsgId,
         s_cServiceClass[GMSG_SERVICE_CLASS(pMsgInfo->pMsgHeader->u32MsgId)],
         GMSG_ID_OFFSET(pMsgInfo->pMsgHeader->u32MsgId), pMsgInfo->pMsgHeader->u64Token,
         pMsgInfo->pMsgHeader->__u64ReplyToken);
#endif
   if (NULL != GMSG_pFindRequest(pMsgRequestInfo, pMsgInfo->pMsgHeader->__u64ReplyToken))
   {
      MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
      printf("GMSG_bAddRequest: token not unique.");
      return(FALSE);
   }

   pNewNode = (GMSG_REQUEST_NODE*)malloc(sizeof(GMSG_REQUEST_NODE));
   pNewNode->MsgRequest.u64Token   = pMsgInfo->pMsgHeader->u64Token;
   pNewNode->MsgRequest.__u64ReplyToken     = pMsgInfo->pMsgHeader->__u64ReplyToken;
   pNewNode->MsgRequest.pDataType  = pMsgInfo->pDataType;
   pNewNode->MsgRequest.pData      = NULL;


   nRemaining = 0;
   nSize = 0;
   if (pNewNode->MsgRequest.pDataType != NULL)
   {
      (void)pNewNode->MsgRequest.pDataType->pfDeserialize(&pNewNode->MsgRequest.pData, pMsgInfo->pRawMsg,
                                                    &nRemaining, &nSize, pNewNode->MsgRequest.pDataType);
   }

   pNewNode->pNext = NULL;

#ifndef __RTXC__
   MsgStatus.u32MsgId     = (U32)pMsgInfo->pMsgHeader->u32MsgId;
   MsgStatus.u32MsgStatus = GMSG_STATUS_TIMEOUT;
   MsgStatus.u64Token     = pMsgInfo->pMsgHeader->u64Token;

   if (!GTIMER_bCreateMsgTimerEx(&pNewNode->MsgRequest.RequestTimeout, &pMsgRequestInfo->RequestTimeoutContext,
                                 &pMsgInfo->pMsgHeader->DstAddress, &pMsgInfo->pMsgHeader->SrcAddress,
                                 GMSG_ID_STATUS, D_GMSG_STATUS, &MsgStatus, pMsgInfo->pMsgHeader->u64Token,
                                 pMsgInfo->pMsgHeader->__u64ReplyToken))
   {
      free(pNewNode);
      MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
      printf("GMSG_bAddRequest: failed to create request timer");
      return(FALSE);
   }

   if (!GTIMER_bStartTimer(&pNewNode->MsgRequest.RequestTimeout, GMSG_REQ_TIMEOUT, 0))
   {
      GTIMER_bStopTimer(&pNewNode->MsgRequest.RequestTimeout);
      GTIMER_bDestroyTimer(&pNewNode->MsgRequest.RequestTimeout);
      free(pNewNode);
      MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
      printf("GMSG_bAddRequest: failed to start request timer");
      return(FALSE);
   }
#else
   pNewNode->MsgRequest.u32MsgId = pMsgInfo->pMsgHeader->u32MsgId;
   pNewNode->MsgRequest.s32RequestTimeout = 4 * GMSG_REQ_TIMEOUT / 1000;
#endif

   if (pMsgRequestInfo->pHead == NULL)
   {
      pMsgRequestInfo->pHead = pNewNode;
   }
   else
   {
      pNode = pMsgRequestInfo->pHead;
      while (pNode->pNext != NULL)
      {
        pNode = pNode->pNext;
      }
      pNode->pNext = pNewNode;
   }
   MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bDeleteRequest
 *
 *    Args:    pMsgRequestInfo - Message request info
 *             pNodeToDelete   - Message reuest node (to delete)
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Remove a request from the request info list
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bDeleteRequest(GMSG_REQUEST_INFO *pMsgRequestInfo, GMSG_REQUEST_NODE *pNodeToDelete)
{
   GMSG_REQUEST_NODE *pNode;

   MUTEX_bLock(&pMsgRequestInfo->RequestMutex);

   if (pMsgRequestInfo->pHead == NULL || pNodeToDelete == NULL)
   {
      MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
      printf("GMSG_bDeleteRequest: Node not found.");
      return(FALSE);
   }
   else if (pMsgRequestInfo->pHead == pNodeToDelete)
   {
      pMsgRequestInfo->pHead = pMsgRequestInfo->pHead->pNext;
#ifndef __RTXC__
      GTIMER_bStopTimer(&pNodeToDelete->MsgRequest.RequestTimeout);
      GTIMER_bDestroyTimer(&pNodeToDelete->MsgRequest.RequestTimeout);
#endif
      free(pNodeToDelete);
      MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
      return(TRUE);
   }
   else
   {
      pNode = pMsgRequestInfo->pHead;
      while(pNode->pNext != NULL)
      {
         if (pNode->pNext == pNodeToDelete)
         {
            pNode->pNext = pNode->pNext->pNext;
#ifndef __RTXC__
            GTIMER_bStopTimer(&pNodeToDelete->MsgRequest.RequestTimeout);
            GTIMER_bDestroyTimer(&pNodeToDelete->MsgRequest.RequestTimeout);
#endif
            free(pNodeToDelete);
            MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
            return(TRUE);
         }
         pNode = pNode->pNext;
      }
   }
   MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
   printf("GMSG_bDeleteRequest: Node not found.");
   return(FALSE);
}

/******************************************************************************
 *
 *    Function: GMSG_bDeleteRequestByToken
 *
 *    Args:    pMsgRequestInfo - message request info
 *             u64Token        - token of message request to remove
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Delete of message request from the message request list by
 *             matching its token
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bDeleteRequestByToken(GMSG_REQUEST_INFO *pMsgRequestInfo, U64 __u64ReplyToken)
{
   MUTEX_bLock(&pMsgRequestInfo->RequestMutex);
   if (!GMSG_bDeleteRequest(pMsgRequestInfo, GMSG_pFindRequest(pMsgRequestInfo, __u64ReplyToken)))
   {
      MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
      return(FALSE);
   }
   MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
   return(TRUE);
}

#ifdef __RTXC__
/******************************************************************************/
/**
 * Find an Expired Node where s32RequestTimeout equals 0
 *
 * @param pMsgRequestInfo message request info
 *
 * @return GMSG_REQUEST_NODE* Expired node, or null, if none exist
 */
/******************************************************************************/
GLOBAL GMSG_REQUEST_NODE * GMSG_pFindExpiredRequest(GMSG_REQUEST_INFO *pMsgRequestInfo)
{
   GMSG_REQUEST_NODE *pNode;

   MUTEX_bLock(&pMsgRequestInfo->RequestMutex);
   pNode = pMsgRequestInfo->pHead;
   while (pNode != NULL)
   {
      if (pNode->MsgRequest.s32RequestTimeout == 0)
      {
         pNode->MsgRequest.s32RequestTimeout = -1;
         break;
      }
      pNode = pNode->pNext;
   }
   MUTEX_bUnlock(&pMsgRequestInfo->RequestMutex);
   return(pNode);
}
#else
/******************************************************************************/
/**
 * Init message filter information - C++ interface
 *
 * @param pMsgFilterInfo ptr to Message Filter Information
 *
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bInitMsgFilterInfoEx(GMSG_FILTER_INFO *pMsgFilterInfo)
{
   pMsgFilterInfo->pHead = NULL;
   return(MUTEX_bInit(&pMsgFilterInfo->FilterMutex, TRUE));
}

/******************************************************************************/
/**
 * Init message filter information
 *
 * @param pMyMailbox ptr to my mailbox
 *
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bInitMsgFilterInfo(GMSG_MAILBOX *pMyMailbox)
{
   return(GMSG_bInitMsgFilterInfoEx(&pMyMailbox->MsgFilterInfo));
}

/******************************************************************************/
/**
 * Clear message filter information - C++ interface
 *
 * @param pMsgFilterInfo ptr to Message Filter Information
 *
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bClearMsgFilterInfoEx(GMSG_FILTER_INFO *pMsgFilterInfo)
{
   GMSG_FILTER_NODE *pNode;
   GMSG_FILTER_NODE *pNextNode;

   MUTEX_bLock(&pMsgFilterInfo->FilterMutex);
   pNode = pMsgFilterInfo->pHead;
   while (pNode != NULL)
   {
      pNextNode = pNode->pNext;
      free(pNode);
      pNode = pNextNode;
   }
   pNode = pMsgFilterInfo->pHead = NULL;
   MUTEX_bUnlock(&pMsgFilterInfo->FilterMutex);
   return(TRUE);
}

/******************************************************************************/
/**
 * Clear message filter information
 *
 * @param pMyMailbox ptr to my mailbox
 *
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bClearMsgFilterInfo(GMSG_MAILBOX *pMyMailbox)
{
   return(GMSG_bClearMsgFilterInfoEx(&pMyMailbox->MsgFilterInfo));
}

/******************************************************************************/
/**
 * Find a message filter node
 *
 * @param pMsgFilterInfo message filter information
 * @param eMsgId         message id to find
 *
 * @return GMSG_FILTER_NODE* filter that was found or NULL if not found
 */
/******************************************************************************/
PRIVATE GMSG_FILTER_NODE * GMSG_pFindMsgFilter(GMSG_FILTER_INFO *pMsgFilterInfo, GMSG_ID_TYPE eMsgId)
{
   GMSG_FILTER_NODE *pNode;

   MUTEX_bLock(&pMsgFilterInfo->FilterMutex);
   pNode = pMsgFilterInfo->pHead;
   while (pNode != NULL)
   {
      if (pNode->eMsgId == eMsgId)
      {
         break;
      }
      pNode = pNode->pNext;
   }
   MUTEX_bUnlock(&pMsgFilterInfo->FilterMutex);
   return(pNode);
}

/******************************************************************************/
/**
 * Add a message filter to message filter information - C++ interface
 *
 * @param pMsgFilterInfo message filter information
 * @param eMsgId         message id to add
 *
 * @return BOOLEAN - TRUE if successfully added, FALSE if already added
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bAddMsgFilterEx(GMSG_FILTER_INFO *pMsgFilterInfo, GMSG_ID_TYPE eMsgId)
{
   GMSG_FILTER_NODE *pNode = NULL;
   GMSG_FILTER_NODE *pNodeFound;

   MUTEX_bLock(&pMsgFilterInfo->FilterMutex);
   if (pMsgFilterInfo->pHead == NULL)
   {
      pMsgFilterInfo->pHead = malloc(sizeof(GMSG_FILTER_NODE));
      pNode = pMsgFilterInfo->pHead;
   }
   else if (NULL != (pNodeFound = GMSG_pFindMsgFilter(pMsgFilterInfo,eMsgId)))
   {
      MUTEX_bUnlock(&pMsgFilterInfo->FilterMutex);
      return(FALSE);
   }
   else
   {
      pNode = pMsgFilterInfo->pHead;
      while (pNode->pNext != NULL)
      {
         pNode = pNode->pNext;
      }
      pNode->pNext = malloc(sizeof(GMSG_FILTER_NODE));
      pNode = pNode->pNext;
   }
   pNode->eMsgId = eMsgId;
   pNode->pNext  = NULL;
   MUTEX_bUnlock(&pMsgFilterInfo->FilterMutex);
   return(TRUE);
}

/******************************************************************************/
/**
 * Add a message filter to message filter information
 *
 * @param pMyMailbox ptr to my mailbox
 * @param eMsgId     message id to add
 *
 * @return BOOLEAN - TRUE if successfully added, FALSE if already added
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bAddMsgFilter(GMSG_MAILBOX *pMyMailbox, GMSG_ID_TYPE eMsgId)
{
   return(GMSG_bAddMsgFilterEx(&pMyMailbox->MsgFilterInfo, eMsgId));
}

/******************************************************************************/
/**
 * Remove a message filter from message filter information - C++ interface
 *
 * @param pMsgFilterInfo message filter information
 * @param eMsgId         message id to remove
 *
 * @return BOOLEAN - TRUE if Removed, FALSE if Not Found
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bRemoveMsgFilterEx(GMSG_FILTER_INFO *pMsgFilterInfo, GMSG_ID_TYPE eMsgId)
{
   GMSG_FILTER_NODE *pNodeToRemove;

   MUTEX_bLock(&pMsgFilterInfo->FilterMutex);
   if (NULL == (pNodeToRemove = GMSG_pFindMsgFilter(pMsgFilterInfo,eMsgId)))
   {
      //nothing to do
   }
   else if (pMsgFilterInfo->pHead == pNodeToRemove)
   {
      pMsgFilterInfo->pHead = pMsgFilterInfo->pHead->pNext;
   }
   else
   {
      GMSG_FILTER_NODE *pNode = pMsgFilterInfo->pHead;

      while (pNode->pNext != pNodeToRemove)
      {
         if (pNode->pNext == NULL)
         {
            //something's really wrong here, lock's not working?
            MUTEX_bUnlock(&pMsgFilterInfo->FilterMutex);
            return(FALSE);
         }
         pNode = pNode->pNext;
      }
      pNode->pNext = pNode->pNext->pNext;
   }
   MUTEX_bUnlock(&pMsgFilterInfo->FilterMutex);
   if (pNodeToRemove != NULL)
   {
      free(pNodeToRemove);
      return(TRUE);
   }
   else
   {
      return(FALSE);
   }
}

/******************************************************************************/
/**
 * Remove a message filter from message filter information
 *
 * @param pMyMailbox ptr to my mailbox
 * @param eMsgId     message id to remove
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bRemoveMsgFilter(GMSG_MAILBOX *pMyMailbox, GMSG_ID_TYPE eMsgId)
{
   return(GMSG_bRemoveMsgFilterEx(&pMyMailbox->MsgFilterInfo, eMsgId));
}

/******************************************************************************/
/**
 * Is a particular message filtered - C++ Interface
 *
 * @param pMsgFilterInfo Message Filter Information
 * @param eMsgId         Message ID
 *
 * @return BOOLEAN - TRUE if message is filtered (found in filter list)
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bIsMsgFilteredEx(GMSG_FILTER_INFO *pMsgFilterInfo, GMSG_ID_TYPE eMsgId)
{
   return((NULL != GMSG_pFindMsgFilter(pMsgFilterInfo, eMsgId)) ? TRUE : FALSE);
}

/******************************************************************************/
/**
 * Is a particular message filtered
 *
 * @param pMyMailbox
 * @param eMsgId
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bIsMsgFiltered(GMSG_MAILBOX *pMyMailbox, GMSG_ID_TYPE eMsgId)
{
   return((NULL != GMSG_pFindMsgFilter(&pMyMailbox->MsgFilterInfo, eMsgId)) ? TRUE : FALSE);
}

/******************************************************************************/
/**
 * Check if a Message is Filtered - and if so, handle the filtered message
 *
 * @param pMsgFilterInfo Message Filter Information
 * @param pMsgInfo       Message Information
 *
 * @return BOOLEAN - TRUE if filtered & handled, FALSE if not filtered
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bHandleFilteredMsg(GMSG_FILTER_INFO *pMsgFilterInfo, GMSG_INFO *pMsgInfo)
{
   //is the message filtered
   if (GMSG_bIsMsgFilteredEx(pMsgFilterInfo, (GMSG_ID_TYPE)pMsgInfo->pMsgHeader->u32MsgId))
   {
      //if it's a request - reply with "filtered" status
      if (pMsgInfo->pMsgHeader->u32MsgFlags & GMSG_FLAG_REQUEST)
      {
         GMSG_bReplyWithStatus(pMsgInfo, GMSG_STATUS_FILTERED);
      }
      return(TRUE);
   }
   else
   {
      return(FALSE);
   }
}

#endif

/******************************************************************************
 *
 *    Function: GMSG_bInitSocketMailbox
 *
 *    Args:    pMyMailbox     - GMSG_MAILBOX to init
 *             pMsgMapInfo    - message map of message ids and handlers
 *             u16Port        - Port to listen on
 *             nBacklog       - listen backlog length
 *             pfMailboxHandler - handler to call on mailbox event
 *             pEventParam      - handler parameter
 *             u32EventPriority - priority of event
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Initialize a socket-based mailbox
 *
 *    Notes:
 *
 ******************************************************************************/
#ifdef __RTXC__
GLOBAL BOOLEAN GMSG_bInitSocketMailbox(GMSG_MAILBOX *pMyMailbox, GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port, int nBacklog,
                                       MBOX Mailbox, RESOURCE Resource, SEMA Sema, BOOLEAN bUdp, U32 u32McIp)
#else
GLOBAL BOOLEAN GMSG_bInitSocketMailbox(GMSG_MAILBOX *pMyMailbox, GMSG_MAP_INFO *pMsgMapInfo, U16 u16Port, int nBacklog,
                                      EVENT_HANDLER_FP pfMailboxHandler, void *pEventParam, U32 u32EventPriority)
#endif
{
   U32 u32Address;

   if (!bUdp)
   {
      if(!(GSOCKET_bGetLocalAddress(&u32Address, "eth0") || GSOCKET_bGetLocalAddress(&u32Address, "eth0:avahi")))
      {
         printf("GMSG_bInitSocketMailbox: error getting address");
         return(FALSE);
      }

      if (!GSOCKET_bCreate(&pMyMailbox->MsgObject.Socket, u32Address, u16Port, TRUE, nBacklog, TRUE))
      {
         printf("GMSG_bInitSocketMailbox: failed to create TCP socket");
         return(FALSE);
      }
   }
   else
   {
      u32Address = INADDR_ANY;
      if (!GSOCKET_bCreateUdp(&pMyMailbox->MsgObject.Socket, u32Address, u16Port, TRUE, nBacklog, TRUE))
      {
         printf("GMSG_bInitSocketMailbox: failed to create UDP socket");
         return(FALSE);
      }

      if (u32McIp == 0)
      {
         if (!GSOCKET_bSetBroadcastOptions(&pMyMailbox->MsgObject.Socket))
         {
            printf("GMSG_bInitSocketMailbox: fail to set as broadcast");
            GSOCKET_bClose(&pMyMailbox->MsgObject.Socket);
            return(FALSE);
         }
      }
      else
      {
         GSOCKET_bSetMulticastOptions(&pMyMailbox->MsgObject.Socket, u32McIp, TRUE);
      }
   }

#ifndef __RTXC__
   if (!GSOCKET_bCreateEvent(&pMyMailbox->MsgEvent.SocketEvent, &pMyMailbox->MsgObject.Socket, pfMailboxHandler,
                            pEventParam, EVENT_MODE_READ, u32EventPriority))
   {
      printf("GMSG_bInitSocketMailbox: error creating socket event");
      GSOCKET_bClose(&pMyMailbox->MsgObject.Socket);
      return(FALSE);
   }
#endif

   pMyMailbox->MsgMailboxType = GMSG_MAILBOX_SOCKET;
   pMyMailbox->MsgAddress.MsgAddressType = GMSG_ADDRESS_SOCKET;
   pMyMailbox->MsgAddress.uAddr.Socket.u32Address = u32Address;
   pMyMailbox->MsgAddress.uAddr.Socket.u32Port    = (U32)u16Port;
   pMyMailbox->pMsgMapInfoArray = pMsgMapInfo;
   pMyMailbox->bUdp = bUdp;
   pMyMailbox->u32MulticastAddress = u32McIp;

#ifdef __RTXC__
   pMyMailbox->RtxcMailbox = Mailbox;
   pMyMailbox->MsgRequestInfo.RequestMutex = Resource;
   pMyMailbox->MsgRequestInfo.pHead = NULL;
   pMyMailbox->MsgSocketInfo.pHead = NULL;
   pMyMailbox->MsgSocketInfo.SocketInfoMutex = Resource;
   pMyMailbox->RtxcSema = Sema;
   pMyMailbox->bMarkForDelete = FALSE;
   pMyMailbox->nReading = 0;
   pMyMailbox->nAccess = 0;
#else
   GTIMER_bCreateContext(&pMyMailbox->MsgRequestInfo.RequestTimeoutContext);
   GTIMER_bCreateEvent(&pMyMailbox->MsgRequestInfo.RequestTimeoutEvent, &pMyMailbox->MsgRequestInfo.RequestTimeoutContext,
                       GTIMER_bHandleTimer, NULL, u32EventPriority);
   MUTEX_bInit(&pMyMailbox->MsgRequestInfo.RequestMutex, TRUE);
   pMyMailbox->MsgRequestInfo.pHead = NULL;
   GMSG_bInitMsgFilterInfo(pMyMailbox);
#endif

   return(TRUE);
}

#ifndef __RTXC__
/******************************************************************************
 *
 *    Function: GMSG_bInitQueueMailbox
 *
 *    Args:    pMyMailbox     - GMSG_MAILBOX to init
 *             pMsgMapInfo    - message map of message ids and handlers
 *             pQueueName     - name of queue to crate
 *             nQueueLength   - length of queue
 *             pfMailboxHandler - handler to call on mailbox event
 *             pEventParam      - handler parameter
 *             u32EventPriority - priority of event
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Initialize a socket-based mailbox
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bInitQueueMailbox(GMSG_MAILBOX *pMyMailbox, GMSG_MAP_INFO *pMsgMapInfo, char *pQueueName, int nQueueLength,
                                     EVENT_HANDLER_FP pfMailboxHandler, void *pEventParam, U32 u32EventPriority)
{
   if (!QUEUE_bCreate(&pMyMailbox->MsgObject.Queue, pQueueName, QUEUE_MODE_RDONLY, TRUE, nQueueLength, GMSG_INIT_SIZE))
   {
      printf("GMSG_bInitQueueMailbox: failed to create queue");
      return(FALSE);
   }

   if (!QUEUE_bCreateEvent(&pMyMailbox->MsgEvent.QueueEvent, &pMyMailbox->MsgObject.Queue, pfMailboxHandler,
                            pEventParam, EVENT_MODE_READ, u32EventPriority))
   {
      printf("GMSG_bInitQueueMailbox: error creating queue event");
      return(FALSE);
   }

   pMyMailbox->MsgMailboxType = GMSG_MAILBOX_QUEUE;
   pMyMailbox->MsgAddress.MsgAddressType = GMSG_ADDRESS_QUEUE;
   strncpy(pMyMailbox->MsgAddress.uAddr.Queue, pQueueName, sizeof(QUEUE_ADDRESS)-1);
   pMyMailbox->MsgAddress.uAddr.Queue[sizeof(QUEUE_ADDRESS)-1] = '\0';
   pMyMailbox->pMsgMapInfoArray = pMsgMapInfo;

   GTIMER_bCreateContext(&pMyMailbox->MsgRequestInfo.RequestTimeoutContext);
   GTIMER_bCreateEvent(&pMyMailbox->MsgRequestInfo.RequestTimeoutEvent, &pMyMailbox->MsgRequestInfo.RequestTimeoutContext,
                           GTIMER_bHandleTimer, NULL, u32EventPriority);
   MUTEX_bInit(&pMyMailbox->MsgRequestInfo.RequestMutex, TRUE);
   pMyMailbox->MsgRequestInfo.pHead = NULL;
   GMSG_bInitMsgFilterInfo(pMyMailbox);

   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bAddMailboxEvent
 *
 *    Args:    pEventList - event list
 *             pMailbox   - mailbox to add to event list
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Add a mailbox to an event list
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bAddMailboxEvent(EVENT_LIST *pEventList, GMSG_MAILBOX *pMailbox)
{
   switch (pMailbox->MsgMailboxType)
   {
   case GMSG_MAILBOX_SOCKET:
      if(!GSOCKET_bAddEvent(pEventList, &pMailbox->MsgEvent.SocketEvent))
      {
         return(FALSE);
      }
      break;
   case GMSG_MAILBOX_QUEUE:
      if(!QUEUE_bAddEvent(pEventList, &pMailbox->MsgEvent.QueueEvent))
      {
         return(FALSE);
      }
      break;
   default:
      return(FALSE);
   }
   if(!GTIMER_bAddEvent(pEventList, &pMailbox->MsgRequestInfo.RequestTimeoutEvent))
   {
      return(FALSE);
   }

   return(TRUE);
}
#else
/******************************************************************************/
/**
 *
 *
 * @param pMyMailbox
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bCloseMailbox(GMSG_MAILBOX *pMyMailbox)
{
   MUTEX_bLock(&pMyMailbox->MsgRequestInfo.RequestMutex);
   while (pMyMailbox->MsgRequestInfo.pHead != NULL)
   {
      GMSG_bDeleteRequest(&pMyMailbox->MsgRequestInfo, pMyMailbox->MsgRequestInfo.pHead);
   }
   MUTEX_bUnlock(&pMyMailbox->MsgRequestInfo.RequestMutex);
   if (pMyMailbox->u32MulticastAddress != 0)
   {
      GSOCKET_bSetMulticastOptions(&pMyMailbox->MsgObject.Socket, pMyMailbox->u32MulticastAddress, FALSE);
   }
   return(GSOCKET_bClose(&pMyMailbox->MsgObject.Socket));
}
#endif

/******************************************************************************
 *
 *    Function: GMSG_bAddressEqual
 *
 *    Args:    pMsgAddress1 - address 1
 *             pMsgAddress2 - address 2
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Compare to addresses for equality
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bAddressEqual(const GMSG_ADDRESS *pMsgAddress1, const GMSG_ADDRESS *pMsgAddress2)
{
   if (pMsgAddress1->MsgAddressType == pMsgAddress2->MsgAddressType)
   {
      switch (pMsgAddress1->MsgAddressType)
      {
      case GMSG_ADDRESS_SOCKET:
         if (pMsgAddress1->uAddr.Socket.u32Address == pMsgAddress2->uAddr.Socket.u32Address &&
             pMsgAddress1->uAddr.Socket.u32Port    == pMsgAddress2->uAddr.Socket.u32Port)
         {
            return(TRUE);
         }
         break;
#ifndef __RTXC__
      case GMSG_ADDRESS_QUEUE:
         if(strcmp(pMsgAddress1->uAddr.Queue, pMsgAddress2->uAddr.Queue) == 0)
         {
            return(TRUE);
         }
         break;
#endif
      default:
         break;
      }
   }
   return(FALSE);
}

#ifdef __RTXC__
/******************************************************************************
 *
 *    Function: GMSG_bReceiveSocket
 *
 *    Args:    pReceiveSocket - Receive Socket
 *             pMsgInfo      - GMSG_INFO to receive message
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Receive a message from a receive socket
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bReceiveSocket(GSOCKET *pReceiveSocket, GMSG_INFO *pMsgInfo)
{
   int nReceived, nTotalReceived;

   pMsgInfo->pMsgHeader = malloc(sizeof(GMSG_HEADER));
   pMsgInfo->pDataType  = NULL;
   pMsgInfo->pStructMsg = NULL;
   pMsgInfo->pRequestDataType = NULL;
   pMsgInfo->pRequestMsg = NULL;
   pMsgInfo->bReceived  = TRUE;
   pMsgInfo->bReply     = FALSE;

   if (-1 == (nReceived = GSOCKET_nReceive(pReceiveSocket, (char*)pMsgInfo->pMsgHeader, sizeof(GMSG_HEADER))))
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      printf("GMSG_nReceiveSocket: msg header receive error");
      return(FALSE);
   }
   else if (nReceived != sizeof(GMSG_HEADER))
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      printf("GMSG_nReceiveSocket: msg header size error");
      return(FALSE);
   }
   else if (0 != memcmp(&pMsgInfo->pMsgHeader->u32MsgSignature, GMSG_SIGNATURE, sizeof(U32)))
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      printf("GMSG_nReceiveSocket: msg header signature error");
      return(FALSE);
   }

   if (pMsgInfo->pMsgHeader->u32MsgSize == 0)
   {
      pMsgInfo->pRawMsg = NULL;
   }
   else
   {
      int nReceiveSize;

      pMsgInfo->pMsgHeader = (GMSG_HEADER*)realloc(pMsgInfo->pMsgHeader, sizeof(GMSG_HEADER) + pMsgInfo->pMsgHeader->u32MsgSize);
      pMsgInfo->pRawMsg = ((char*)pMsgInfo->pMsgHeader) + sizeof(GMSG_HEADER);
      nReceiveSize = (int)pMsgInfo->pMsgHeader->u32MsgSize;
      nTotalReceived = 0;

      while (nTotalReceived < nReceiveSize)
      {
         int            nError;
         fd_set         readFds;
         struct timeval timeout = {5, 0};

         FD_ZERO(&readFds);
         FD_SET(pReceiveSocket->nSocket, &readFds);

         if (-1 == (nError = select(pReceiveSocket->nSocket + 1, &readFds, NULL, NULL, &timeout)))
         {
            printf("GMSG_nReceiveSocket: select error.");
            break;
         }
         else if(nError == 0)
         {
            printf("GMSG_nReceiveSocket: select timeout");
            break;
         }
         else if (-1 == (nReceived = GSOCKET_nReceive(pReceiveSocket, &((char*)pMsgInfo->pRawMsg)[nTotalReceived], MIN(GSOCKET_MAX_PACKET, nReceiveSize-nTotalReceived))))
         {
            free(pMsgInfo->pMsgHeader);
            pMsgInfo->pMsgHeader = NULL;
            printf("GMSG_nReceiveSocket: receive error");
            return(FALSE);
         }
         else if (nReceived == 0)
         {
            printf("GMSG_nReceiveSocket: received 0 bytes on read-ready socket");
            break;
         }

         nTotalReceived += nReceived;
      }

      if (nTotalReceived != nReceiveSize)
      {
         free(pMsgInfo->pMsgHeader);
         pMsgInfo->pMsgHeader = NULL;
         printf("GMSG_nReceiveSocket: msg body receive size error");
         printf("GMSG_nReceiveSocket: receive = %d, msg size = %d", nTotalReceived, nReceiveSize);
         return(FALSE);
      }
   }

   return(TRUE);
}
/******************************************************************************
 *
 *    Function: GMSG_bReceiveSocketAdapter
 *
 *    Args:    pReceiveSocket - Receive Socket
 *             pMsgInfo      - GMSG_INFO to receive message
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Receive a message from a receive socket
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bReceiveSocketAdapter(unsigned char *messageBody, int messageLen, GMSG_INFO *pMsgInfo)
{
   pMsgInfo->pMsgHeader = (GMSG_HEADER *) messageBody;
   pMsgInfo->pDataType  = NULL;
   pMsgInfo->pStructMsg = NULL;
   pMsgInfo->pRequestDataType = NULL;
   pMsgInfo->pRequestMsg = NULL;
   pMsgInfo->bReceived  = TRUE;
   pMsgInfo->bReply     = FALSE;

   if (0 != memcmp(&pMsgInfo->pMsgHeader->u32MsgSignature, GMSG_SIGNATURE, sizeof(U32)))
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      printf("GMSG_nReceiveSocketAdapter: msg header signature error");
      return(FALSE);
   }

   if (pMsgInfo->pMsgHeader->u32MsgSize == 0)
   {
      pMsgInfo->pRawMsg = NULL;
   }
   else
   {
      int nReceiveSize;
      pMsgInfo->pRawMsg = ((char*)pMsgInfo->pMsgHeader) + sizeof(GMSG_HEADER);
      nReceiveSize = (int)pMsgInfo->pMsgHeader->u32MsgSize;
   }
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bReceiveUDPSocket
 *
 *    Args:    pListenSocket - Listening Socket
 *             pMsgInfo      - GMSG_INFO to receive message
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Receive a message from listening socket
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bReceiveUDPSocket(GSOCKET *pListenSocket, GMSG_INFO *pMsgInfo)
{
   int nReceived = 0;

   pMsgInfo->pMsgHeader = NULL;
   pMsgInfo->pDataType  = NULL;
   pMsgInfo->pStructMsg = NULL;
   pMsgInfo->bReceived  = TRUE;

   pMsgInfo->pMsgHeader = (GMSG_HEADER*)malloc(GMSG_INIT_SIZE);
   memset(pMsgInfo->pMsgHeader, 0, GMSG_INIT_SIZE);
   pMsgInfo->pRawMsg = ((char*)pMsgInfo->pMsgHeader) + sizeof(GMSG_HEADER);
   pMsgInfo->pMsgHeader->u32MsgSize = 0;
   pMsgInfo->pMsgHeader->u32MsgFlags = 0;

   if (-1 == (nReceived = GSOCKET_nReceiveFrom(pListenSocket, pMsgInfo->pRawMsg, GMSG_INIT_SIZE-sizeof(GMSG_HEADER))))
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      printf("GMSG_nReceiveUDPSocket: msg receive error");
      return(FALSE);
   }
   else if (!nReceived)
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      printf("GMSG_nReceiveSocket: msg body receive size error");
      return(FALSE);
   }

   pMsgInfo->pMsgHeader->u32MsgSize = nReceived;
   pMsgInfo->pMsgHeader->u32MsgId = GMSG_ID_RADAR_RECEIVE_DATA;
   //log_me("ngd GMSG_bReceiveUDPSocket nReceived %d %02x%02x%02x%02x",nReceived,((char *)pMsgInfo->pRawMsg)[0],
   //      ((char *)pMsgInfo->pRawMsg)[1],((char *)pMsgInfo->pRawMsg)[2],((char *)pMsgInfo->pRawMsg)[3]);
   return(TRUE);
}

#else
/******************************************************************************
 *
 *    Function: GMSG_bReceiveSocket
 *
 *    Args:    pListenSocket - Listening Socket
 *             pMsgInfo      - GMSG_INFO to receive message
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Receive a message from listening socket
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bReceiveSocket(GSOCKET *pListenSocket, GMSG_INFO *pMsgInfo)
{
   GSOCKET ReceiveSocket;
   int nReceived, nTotalReceived;

   if (!GSOCKET_bAccept(pListenSocket, &ReceiveSocket, TRUE ))
   {
      printf("GMSG_nReceiveSocket: accept error");
      return(FALSE);
   }

   pMsgInfo->pMsgHeader = malloc(sizeof(GMSG_HEADER));
   pMsgInfo->pDataType  = NULL;
   pMsgInfo->pStructMsg = NULL;
   pMsgInfo->pRequestDataType = NULL;
   pMsgInfo->pRequestMsg = NULL;
   pMsgInfo->bReceived  = TRUE;
   pMsgInfo->bReply     = FALSE;

   if (-1 == (nReceived = GSOCKET_nReceive(&ReceiveSocket, (char*)pMsgInfo->pMsgHeader, sizeof(GMSG_HEADER))))
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      GSOCKET_bClose(&ReceiveSocket);
      printf("GMSG_nReceiveSocket: msg header receive error");
      return(FALSE);
   }
   else if (nReceived != sizeof(GMSG_HEADER))
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      GSOCKET_bClose(&ReceiveSocket);
      printf("GMSG_nReceiveSocket: msg header size error");
      return(FALSE);
   }

   if (pMsgInfo->pMsgHeader->u32MsgSize == 0)
   {
      pMsgInfo->pRawMsg = NULL;
   }
   else
   {
      pMsgInfo->pMsgHeader = (GMSG_HEADER*)realloc(pMsgInfo->pMsgHeader, sizeof(GMSG_HEADER) + pMsgInfo->pMsgHeader->u32MsgSize);
      pMsgInfo->pRawMsg = ((char*)pMsgInfo->pMsgHeader) + sizeof(GMSG_HEADER);

      nTotalReceived = 0;
      while (nTotalReceived < (int)pMsgInfo->pMsgHeader->u32MsgSize)
      {
         if (-1 == (nReceived = GSOCKET_nReceive(&ReceiveSocket, &((char*)pMsgInfo->pRawMsg)[nTotalReceived], MIN(GSOCKET_MAX_PACKET, (int)pMsgInfo->pMsgHeader->u32MsgSize-nTotalReceived))))
         {
            free(pMsgInfo->pMsgHeader);
            pMsgInfo->pMsgHeader = NULL;
            GSOCKET_bClose(&ReceiveSocket);
            printf("GMSG_nReceiveSocket: msg body receive error");
            return(FALSE);
         }
         nTotalReceived += nReceived;
      }
      if (nTotalReceived != (int)pMsgInfo->pMsgHeader->u32MsgSize)
      {
         U32 u32Size = pMsgInfo->pMsgHeader->u32MsgSize;
         free(pMsgInfo->pMsgHeader);
         pMsgInfo->pMsgHeader = NULL;
         GSOCKET_bClose(&ReceiveSocket);
         printf("GMSG_nReceiveSocket: msg body receive size error");
         printf("GMSG_nReceiveSocket: receive = %d, msg size = %d", nTotalReceived, (int)u32Size);
         return(FALSE);
      }
   }

   GSOCKET_bClose(&ReceiveSocket);
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bReceiveUDPSocket
 *
 *    Args:    pListenSocket - Listening Socket
 *             pMsgInfo      - GMSG_INFO to receive message
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Receive a message from listening socket
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bReceiveUDPSocket(GSOCKET *pListenSocket, GMSG_INFO *pMsgInfo)
{
   int nReceived;

   pMsgInfo->pMsgHeader = NULL;
   pMsgInfo->pDataType  = NULL;
   pMsgInfo->pStructMsg = NULL;
   pMsgInfo->bReceived  = TRUE;

   pMsgInfo->pMsgHeader = (GMSG_HEADER*)malloc(GMSG_INIT_SIZE);
   pMsgInfo->pRawMsg = ((char*)pMsgInfo->pMsgHeader) + sizeof(GMSG_HEADER);
   pMsgInfo->pMsgHeader->u32MsgSize = 0;
   pMsgInfo->pMsgHeader->u32MsgFlags = 0;

   if (-1 == (nReceived = GSOCKET_DGRAM_nReceiveFrom(pListenSocket, pMsgInfo->pRawMsg, GMSG_INIT_SIZE-sizeof(GMSG_HEADER))))
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      printf("GMSG_nReceiveUDPSocket: msg receive error");
      return(FALSE);
   }
   else if (!nReceived)
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      printf("GMSG_nReceiveSocket: msg body receive size error");
      return(FALSE);
   }

   pMsgInfo->pMsgHeader->u32MsgSize = nReceived;
   pMsgInfo->pMsgHeader->u32MsgId = GMSG_ID_UDP_MSG;
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bReceiveQueue
 *
 *    Args:    pQueue   - queue
 *             pMsgInfo - message info
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Receive a message from a queue
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bReceiveQueue(QUEUE *pQueue, GMSG_INFO *pMsgInfo)
{
   int nMsgSize;

   pMsgInfo->pMsgHeader = (GMSG_HEADER*)malloc(GMSG_INIT_SIZE);
   pMsgInfo->pDataType  = NULL;
   pMsgInfo->pStructMsg = NULL;
   pMsgInfo->pRequestDataType = NULL;
   pMsgInfo->pRequestMsg = NULL;
   pMsgInfo->bReceived  = TRUE;
   pMsgInfo->bReply     = FALSE;

   if (-1 == (nMsgSize = QUEUE_nReceive(pQueue, (char*)pMsgInfo->pMsgHeader, GMSG_INIT_SIZE, NULL)))
   {
      free(pMsgInfo->pMsgHeader);
      printf("GMSG_bReceiveQueue: msg receive error");
      return(FALSE);
   }
   if (nMsgSize < sizeof(GMSG_HEADER))
   {
      free(pMsgInfo->pMsgHeader);
      printf("GMSG_bReceiveQueue: got less than a header");
      return(FALSE);
   }
   pMsgInfo->pMsgHeader = realloc(pMsgInfo->pMsgHeader, sizeof(GMSG_HEADER) + pMsgInfo->pMsgHeader->u32MsgSize);
   if ((nMsgSize == GMSG_INIT_SIZE) && ((sizeof(GMSG_HEADER) + pMsgInfo->pMsgHeader->u32MsgSize) > GMSG_INIT_SIZE))
   {
      int nReceived = GMSG_INIT_SIZE;

      while (nReceived < (sizeof(GMSG_HEADER) + (int)pMsgInfo->pMsgHeader->u32MsgSize))
      {
         if (-1 == (nMsgSize = QUEUE_nReceive(pQueue, &((char*)pMsgInfo->pMsgHeader)[nReceived], GMSG_INIT_SIZE, NULL)))
         {
            free(pMsgInfo->pMsgHeader);
            printf("GMSG_bReceiveQueue: msg receive error");
            return(FALSE);
         }
         nReceived += nMsgSize;
      }
      nMsgSize = nReceived;
   }
   if (nMsgSize != (sizeof(GMSG_HEADER) + (int)pMsgInfo->pMsgHeader->u32MsgSize))
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
      printf("GMSG_bReceiveQueue: msg size error");
      return(FALSE);
   }
   if (pMsgInfo->pMsgHeader->u32MsgSize == 0)
   {
      pMsgInfo->pRawMsg = NULL;
   }
   else
   {
      pMsgInfo->pRawMsg = ((char*)pMsgInfo->pMsgHeader) + sizeof(GMSG_HEADER);
   }
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bReceive
 *
 *    Args:    pMailbox - mailbox
 *             pMsgInfo - message info
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Receive a message from a mailbox
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bReceive(GMSG_MAILBOX *pMailbox, GMSG_INFO *pMsgInfo)
{
   switch(pMailbox->MsgMailboxType)
   {
   case GMSG_MAILBOX_SOCKET:
      return(GMSG_bReceiveSocket(&pMailbox->MsgObject.Socket, pMsgInfo));
   case GMSG_MAILBOX_QUEUE:
      return(GMSG_bReceiveQueue(&pMailbox->MsgObject.Queue, pMsgInfo));
   default:
      return(FALSE);
   }
}
#endif

/******************************************************************************
 *
 *    Function: GMSG_bHandleReplies
 *
 *    Args:    pMsgRequestInfo - list of message requests waiting reply
 *             pMsgInfo - message information
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Check incoming messages for a reply
 *
 *    Notes:   Call this after receiving a message
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bHandleReplies(GMSG_REQUEST_INFO *pMsgRequestInfo, GMSG_INFO *pMsgInfo)
{
   if (pMsgInfo->pMsgHeader->u32MsgFlags & GMSG_FLAG_REPLY)
   {
      GMSG_REQUEST_NODE *pMsgRequestNode;

#ifdef GMSG_TRACE
      TRACE("Handle Reply- MSG_ID: %08X (Class: %s, Offset %d), TOKEN: %llu", pMsgInfo->pMsgHeader->u32MsgId,
            s_cServiceClass[GMSG_SERVICE_CLASS(pMsgInfo->pMsgHeader->u32MsgId)],
            GMSG_ID_OFFSET(pMsgInfo->pMsgHeader->u32MsgId), pMsgInfo->pMsgHeader->u64Token);
#endif
      if (NULL == (pMsgRequestNode = GMSG_pFindRequest(pMsgRequestInfo, pMsgInfo->pMsgHeader->__u64ReplyToken)))
      {
         printf("GMSG_bHandleReplies: unable to find request");
         return(FALSE);
      }
      pMsgInfo->bReply = TRUE;
      pMsgInfo->pRequestDataType = pMsgRequestNode->MsgRequest.pDataType;
      pMsgInfo->pRequestMsg      = pMsgRequestNode->MsgRequest.pData;
      if (!GMSG_bDeleteRequest(pMsgRequestInfo, pMsgRequestNode))
      {
         printf("GMSG_bHandleReplies: unable to delete request");
         return(FALSE);
      }
      return(TRUE);
   }
   else
   {
      return(TRUE);
   }
}

/******************************************************************************
 *
 *    Function: GMSG_pGetMsgMapInfo
 *
 *    Args:    MsgMapInfoArray     - Msg Map information array
 *             pMsgInfo            - ptr to received message
 *
 *    Return:  ptr to GMSG_MAP_INFO
 *
 *    Purpose: Get Msg Map info for message
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE GMSG_MAP_INFO * GMSG_pGetMsgMapInfo(GMSG_MAP_INFO *pMsgMapInfoArray, GMSG_INFO *pMsgInfo)
{
   GMSG_ID_TYPE MsgIdType = (GMSG_ID_TYPE)pMsgInfo->pMsgHeader->u32MsgId;
   int i = 0;

   while (pMsgMapInfoArray[i].MsgIdType != GMSG_ID_DEFAULT_UNKNOWN)
   {
      printf("[%d]%s: looking for gmsg id %d (%d:%d)\n", __LINE__, __FUNCTION__, MsgIdType, i, pMsgMapInfoArray[i].MsgIdType);

      if (pMsgMapInfoArray[i].MsgIdType == MsgIdType)
      {
         return(&pMsgMapInfoArray[i]);
      }

      i++;
   }

   return(&s_DefaultMspMapInfo);
}

/******************************************************************************
 *
 *    Function: GMSG_bParse
 *
 *    Args:    pMsgInfo    - Messsage Information
 *             pDataType   - Datatype of message
 *
 *    Return:  TRUE - always successful
 *
 *    Purpose: deserialize message data
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bParse(GMSG_INFO *pMsgInfo, const G_DATA_TYPE *pDataType)
{
   int nRemaining, nSize;

   pMsgInfo->pDataType  = pDataType;
   pMsgInfo->pStructMsg = NULL;

   //get data type
   if (pDataType != NULL)
   {
      nRemaining = pMsgInfo->pMsgHeader->u32MsgSize;
      nSize = 0;
      //deserialize into structure
      (void)pDataType->pfDeserialize(&pMsgInfo->pStructMsg, pMsgInfo->pRawMsg, &nRemaining, &nSize, pDataType);
   }
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bFree
 *
 *    Args:    pMsg - ptr to Message
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Free a message
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bFree(GMSG_INFO *pMsgInfo)
{
   if (pMsgInfo->bReceived && pMsgInfo->pDataType != NULL && pMsgInfo->pStructMsg != NULL)
   {
      (void)pMsgInfo->pDataType->pfDeallocate(pMsgInfo->pStructMsg, TRUE, pMsgInfo->pDataType);
      pMsgInfo->pStructMsg = NULL;
   }
   if (pMsgInfo->bReceived && pMsgInfo->bReply && pMsgInfo->pRequestDataType != NULL && pMsgInfo->pRequestMsg != NULL)
   {
      (void)pMsgInfo->pRequestDataType->pfDeallocate(pMsgInfo->pRequestMsg, TRUE, pMsgInfo->pRequestDataType);
      pMsgInfo->pRequestMsg = NULL;
   }
   if (pMsgInfo->pMsgHeader != NULL)
   {
      free(pMsgInfo->pMsgHeader);
      pMsgInfo->pMsgHeader = NULL;
   }
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bHandleMsg
 *
 *    Args:    MsgHandlerInfoArray - Msg Handler information array
 *             pMsg                - ptr to received message
 *
 *    Return:  BOOLEAN - TRUE if Success
 *
 *    Purpose: Given a Msg Handler Array and a message execute the message
 *             Handler for that message
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bHandleMsg(GMSG_MAILBOX *pMsgMailbox, GMSG_INFO *pMsgInfo)
{
   GMSG_MAP_INFO *pMsgMapInfo;

   if (NULL != (pMsgMapInfo = GMSG_pGetMsgMapInfo(pMsgMailbox->pMsgMapInfoArray, pMsgInfo)))
   {
//      printf("[%d]%s: u32MsgSignature 0x%x\n", __LINE__, __FUNCTION__, pMsgInfo->pMsgHeader->u32MsgSignature);
      printf("[%d]%s: u32MsgId 0x%x\n", __LINE__, __FUNCTION__, pMsgInfo->pMsgHeader->u32MsgId);
//      printf("[%d]%s: Src Address 0x%x\n", __LINE__, __FUNCTION__, pMsgInfo->pMsgHeader->SrcAddress.uAddr.Socket.u32Address);

      if (GMSG_bParse(pMsgInfo, pMsgMapInfo->pDataType))
      {
         if (pMsgMapInfo->pfHandler(pMsgMailbox->RtxcMailbox, pMsgInfo, pMsgMapInfo->pvParam1, pMsgMapInfo->pvParam2))
         {
      printf("[%d]%s: here\n", __LINE__, __FUNCTION__);
            return(TRUE);
         }
         else
         {
      printf("[%d]%s: here\n", __LINE__, __FUNCTION__);
            return(GMSG_bDefaultMsgHandler(pMsgMailbox->RtxcMailbox, pMsgInfo, NULL, NULL));
         }
      }
      else
      {
         return(GMSG_bDefaultMsgHandler(pMsgMailbox->RtxcMailbox, pMsgInfo, NULL, NULL));
      }
   }
   else
   {
      return(GMSG_bDefaultMsgHandler(pMsgMailbox->RtxcMailbox, pMsgInfo, NULL, NULL));
   }
}

#ifdef __RTXC__
/******************************************************************************/
/**
 *
 *
 * @param pMailbox
 * @param pListenSocket
 * @param pReceiveSocket
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bAcceptMessage(GMSG_MAILBOX *pMailbox, GSOCKET *pReceiveSocket)
{
   if (!GSOCKET_bAccept(&pMailbox->MsgObject.Socket, pReceiveSocket, TRUE))
   {
      printf("GMSG_nReceiveSocket: accept error");
      return(FALSE);
   }
   return(TRUE);
}

/******************************************************************************/
/**
 *
 *
 * @param pMailbox
 * @param pReceiveSocket
 * @param pMsgInfo
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bReceiveMessage(GMSG_MAILBOX *pMailbox, GSOCKET *pReceiveSocket, GMSG_INFO *pMsgInfo)
{
   if (!GMSG_bReceiveSocket(pReceiveSocket, pMsgInfo))
   {
      return(FALSE);
   }
   if (!GMSG_bHandleReplies(&pMailbox->MsgRequestInfo, pMsgInfo))
   {
      //warn, but don't fail
      printf("GMSG_bHandleSynchronously: Got unmatched reply.");
   }
   return(TRUE);
}
#else
/******************************************************************************
 *
 *    Function: GMSG_vHandleMsgThreadEntry
 *
 *    Args:    pArg - ptr to GMSG_THREAD_INFO
 *
 *    Return:  -none-
 *
 *    Purpose:  Thread entry point for message handling
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void GMSG_vHandleMsgThreadEntry(void *pArg)
{
   GMSG_THREAD_INFO  *pMsgThreadInfo = (GMSG_THREAD_INFO*)pArg;

   GMSG_bHandleMsg(pMsgThreadInfo->pMsgMailbox, &pMsgThreadInfo->MsgInfo);
   GMSG_bFree(&pMsgThreadInfo->MsgInfo);

   free(pMsgThreadInfo);
}

/******************************************************************************
 *
 *    Function: GMSG_bHandleMsgThread
 *
 *    Args:    pEventContext - Message mailbox cast as event context
 *             pEventParam   - Not used
 *
 *    Return:  BOOLEAN - TRUE if success
 *
 *    Purpose: Handle a message by spawning off a thread
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bHandleMsgThread(GTHREAD_POOL *pThreadPool, EVENT_CONTEXT *pEventContext, void *pEventParam)
{
   GMSG_MAILBOX *pMailbox = (GMSG_MAILBOX*)pEventContext;
   GMSG_INFO     MsgInfo;

   if (!GMSG_bReceive(pMailbox, &MsgInfo))
   {
      return(FALSE);
   }

   //match replies with requests and remove from linked list
   if(!GMSG_bHandleReplies(&pMailbox->MsgRequestInfo, &MsgInfo))
   {
      //warn, but don't fail
      printf("GMSG_bHandleMsgThread: Got unmatched reply.");
   }

   if(GMSG_bHandleFilteredMsg(&pMailbox->MsgFilterInfo, &MsgInfo))
   {
#ifdef GMSG_TRACE
      TRACE("GMSG_bHandleMsgThread: Handled filtered message");
#endif
      GMSG_bFree(&MsgInfo);
      return(TRUE);
   }
   else
   {
      GMSG_THREAD_INFO *pMsgThreadInfo = malloc(sizeof(GMSG_THREAD_INFO));

      memcpy(&pMsgThreadInfo->MsgInfo, &MsgInfo, sizeof(GMSG_INFO));
      pMsgThreadInfo->pMsgMailbox = pMailbox;

      if (GTHREAD_bPushWorkByAddress(pThreadPool, &pMsgThreadInfo->MsgInfo.pMsgHeader->SrcAddress,
                                     GMSG_vHandleMsgThreadEntry, (void *)pMsgThreadInfo))
      {
         return(TRUE);
      }
      else
      {
         GMSG_bFree(&pMsgThreadInfo->MsgInfo);
         free(pMsgThreadInfo);
         return(FALSE);
      }
   }
}
/******************************************************************************
 *
 *    Function: GMSG_bHandleMsgSynchronously
 *
 *    Args:    pEventContext - GSOCKET event ptr cast as EVENT_CONTEXT ptr
 *             pEventParam   - ptr to GMSG_CONTEXT
 *
 *    Return:  BOOLEAN - TRUE if successful
 *
 *    Purpose: Handle a message from a socket synchronously
 *
 *    Notes:   Choose this Message event handler function if posting to a queue
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bHandleSynchronously(EVENT_CONTEXT *pEventContext, void *pEventParam)
{
   GMSG_MAILBOX *pMailbox = (GMSG_MAILBOX*)pEventContext;
   GMSG_INFO     MsgInfo;

   if (!GMSG_bReceive(pMailbox, &MsgInfo))
   {
      return(FALSE);
   }

   if (!GMSG_bHandleReplies(&pMailbox->MsgRequestInfo, &MsgInfo))
   {
      //warn, but don't fail
      printf("GMSG_bHandleSynchronously: Got unmatched reply.");
   }

   if (GMSG_bHandleFilteredMsg(&pMailbox->MsgFilterInfo, &MsgInfo))
   {
#ifdef GMSG_TRACE
      TRACE("GMSG_bHandleMsgThread: Handled filtered message");
#endif
   }
   else
   {
      GMSG_bHandleMsg(pMailbox, &MsgInfo);
   }
   GMSG_bFree(&MsgInfo);

   return(TRUE);
}
#endif

/******************************************************************************
 *
 *    Function: GMSG_bCreate
 *
 *    Args:    pMsgInfo    - message info (message block)
 *             pMyAddress  - my address
 *             eMsgId      - message id
 *             pDataType   - data type
 *             pData       - ptr to data structure
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Create a message (for sending)
 *
 *    Notes:   Still needs to be addressed
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bCreate(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pMyAddress, GMSG_ID_TYPE eMsgId,
                           const G_DATA_TYPE *pDataType, void *pData, U32 u32Flags)
{
   pMsgInfo->pMsgHeader = (GMSG_HEADER *)malloc(pDataType ? GMSG_INIT_SIZE : sizeof(GMSG_HEADER));
   pMsgInfo->pDataType  = pDataType;
   pMsgInfo->pStructMsg = pData;
   pMsgInfo->pRawMsg    = NULL;
   pMsgInfo->pRequestDataType = NULL;
   pMsgInfo->pRequestMsg = NULL;
   pMsgInfo->bReceived  = FALSE;
   pMsgInfo->bReply     = FALSE;
   pMsgInfo->u32UdpLength = 0;
   pMsgInfo->u32TimeoutInMs = 0;
   if (pMyAddress)
   {
      pMsgInfo->pMsgHeader->SrcAddress  = *pMyAddress;
   }
   else
   {
      memset(&pMsgInfo->pMsgHeader->SrcAddress, 0x00, sizeof(GMSG_ADDRESS));
   }
   memset(&pMsgInfo->pMsgHeader->DstAddress, 0x00, sizeof(GMSG_ADDRESS));
   memcpy(&pMsgInfo->pMsgHeader->u32MsgSignature, GMSG_SIGNATURE, sizeof(U32));
   pMsgInfo->pMsgHeader->u32MsgId    = (U32)eMsgId;
   pMsgInfo->pMsgHeader->u32MsgSize  = 0;
   pMsgInfo->pMsgHeader->u32MsgFlags = u32Flags;
   pMsgInfo->pMsgHeader->u64Token    = NO_TOKEN;

   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bSerialize
 *
 *    Args:    pMsgInfo - message info (message block)
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Serialize a message (for sending)
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bSerialize(GMSG_INFO *pMsgInfo)
{
   int nBufferSize, nRemaining, nSize, nReturn;

   if (pMsgInfo->pDataType != NULL)
   {
      nBufferSize = GMSG_INIT_SIZE;
      do
      {
         nRemaining        = nBufferSize - sizeof(GMSG_HEADER);
         nSize             = 0;
         pMsgInfo->pRawMsg = ((char*)pMsgInfo->pMsgHeader) + sizeof(GMSG_HEADER);
         nReturn = pMsgInfo->pDataType->pfSerialize(pMsgInfo->pStructMsg, &pMsgInfo->pRawMsg, &nRemaining, &nSize, pMsgInfo->pDataType);
         if (nReturn != DATA_BUFFER_OVERFLOW || nBufferSize >= GMSG_MAX_MSG_SIZE)
         {
            break;
         }
         nBufferSize *= 4;
         pMsgInfo->pMsgHeader = realloc(pMsgInfo->pMsgHeader, nBufferSize);
      } while (nReturn == DATA_BUFFER_OVERFLOW);

      if (0 > nReturn)
      {
         printf("GMSG_bSerialize: serialization error");
         return(FALSE);
      }
      pMsgInfo->pMsgHeader->u32MsgSize = nSize;
   }
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bAddress
 *
 *    Args:    pMsgInfo - message info (message block)
 *             pDstAddress - destination address
 *             u64Token - user token
 *
 *    Return:  BOOLEAN - TRUE if successful
 *
 *    Purpose: Address a message
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bAddress(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pDstAddress, U64 u64Token)
{
   pMsgInfo->pMsgHeader->DstAddress = *pDstAddress;
   pMsgInfo->pMsgHeader->u64Token   = u64Token;
   return(TRUE);
}

#ifdef __RTXC__
GLOBAL BOOLEAN GMSG_bNewSocket(GSOCKET *pSocket, GMSG_ADDRESS *pDstAddress)
{
   BOOLEAN bReturn;

   if (!GSOCKET_bCreate(pSocket, pDstAddress->uAddr.Socket.u32Address,
                        (U16)pDstAddress->uAddr.Socket.u32Port,
                        FALSE, 0, TRUE))
   {
      printf("GMSG_bNewSocket: create socket error");
      bReturn = FALSE;
   }
   else if(!GSOCKET_bConnect(pSocket))
   {
      printf("GMSG_bNewSocket: connect to socket error");
      bReturn = FALSE;
   }
   else
   {
      bReturn = TRUE;
   }
   return(bReturn);
}

/******************************************************************************
 *
 *    Function: GMSG_bSendSocket
 *
 *    Args:    pMsgInfo     - message info
 *             pDstAddress  - address to send message
 *                            NULL to send to address in message header
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: send a message to a socket mailbox
 *
 *    Notes:   header information already set
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bSendSocket(GMSG_INFO *pMsgInfo, GSOCKET *pSendSocket, BOOLEAN bFreeMsgInfo)
{
   int nSent;
   int nTotalSent;
   int retval;
   BOOLEAN bReturn = TRUE;
   int nTransmitSize = sizeof(GMSG_HEADER) + (int)pMsgInfo->pMsgHeader->u32MsgSize;

   nTotalSent = 0;
   while (nTotalSent < nTransmitSize)
   {
      fd_set writeFds;
      fd_set errFds;
      struct timeval timeout;

      if (!pMsgInfo->u32TimeoutInMs)
      {
         timeout.tv_sec = 5;
         timeout.tv_usec = 0;
      }
      else
      {
         timeout.tv_sec = 0;
         timeout.tv_usec = pMsgInfo->u32TimeoutInMs * 1000;
      }

      FD_ZERO(&writeFds);
      FD_SET(pSendSocket->nSocket, &writeFds);
      FD_ZERO(&errFds);
      FD_SET(pSendSocket->nSocket, &errFds);
      //if (-1 == (retval = select(pSendSocket->nSocket + 1, NULL, &writeFds, NULL, &timeout)))
      if (-1 == (retval = select(pSendSocket->nSocket + 1, NULL, &writeFds, &errFds, &timeout)))
      {
         printf("GMSG_bSendSocket: select() error.");
         break;
      }
      else if (retval == 0)
      {
         printf("GMSG_bSendSocket: select() timeout");
         break;
      }
      else if (FD_ISSET (pSendSocket->nSocket, &errFds))  // handle error on Socket
      {
         printf("GMSG_bSendSocket: select() error on socket");
         break;
      }
      else if(-1 == (nSent = GSOCKET_nSend(pSendSocket, &((char*)pMsgInfo->pMsgHeader)[nTotalSent],
                                     MIN(GSOCKET_MAX_PACKET, nTransmitSize-nTotalSent))))
      {
         printf("GMSG_bSendSocket: send() error.");
         break;
      }
      else if (nSent == 0)
      {
         printf("GMSG_bSendSocket: sent 0 bytes on write-ready socket");
         break;
      }
      nTotalSent += nSent;
   }
   if (nTotalSent != nTransmitSize)
   {
      printf("GMSG_bSendSocket: msg send size error");
      printf("GMSG_bSendSocket: send = %d, msg size = %d", nTotalSent, sizeof(GMSG_HEADER) + (int)pMsgInfo->pMsgHeader->u32MsgSize);
      bReturn = FALSE;
   }
   if (bFreeMsgInfo)
   {
      GMSG_bFree(pMsgInfo);
      free(pMsgInfo);
   }
   return(bReturn);
}

 /******************************************************************************
 *
 *    Function: GMSG_bSendSocketUdp
 *
 *    Args:    -none-
 *
 *    Return:  -none-
 *
 *    Purpose: send data to a socket
 *
 *    Notes:   header information already set
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bSendSocketUDP(GSOCKET socket, unsigned char *pBuffer, int nLen, struct sockaddr *dstAddr)
{
   int nSendLength = 0;
   socklen_t nSendToSize = sizeof(struct sockaddr);

   if (nLen)
   {
      nSendLength = sendto((int)socket.nSocket, pBuffer, nLen, 0, dstAddr, nSendToSize);
      if (nSendLength < 0)
      {
         printf("GMSG_bSendSocketUDP: send error");
         return(FALSE);
      }
      return(TRUE);
   }
   return(FALSE);
}

#else
/******************************************************************************/
/**
 *
 *
 * @param pArg
 */
/******************************************************************************/
PRIVATE void GMSG_vSendSocketEntry(void *pArg)
{
   GSOCKET SendSocket;
   GMSG_SEND_THREAD_INFO *pSendThreadInfo = (GMSG_SEND_THREAD_INFO*)pArg;
   GMSG_INFO *pMsgInfo = pSendThreadInfo->pMsgInfo;
   GMSG_ADDRESS DstAddress = pSendThreadInfo->DstAddress;
   BOOLEAN bFreeMsgInfo    = pSendThreadInfo->bFreeMsgInfo;

   free(pSendThreadInfo);
   if (!GSOCKET_bCreate(&SendSocket, DstAddress.uAddr.Socket.u32Address,
                        (U16)DstAddress.uAddr.Socket.u32Port,
                        FALSE, 0, FALSE))
   {
      printf("GMSG_bSendSocket: create socket error");
      if (pMsgInfo->pMsgHeader->u32MsgFlags & GMSG_FLAG_REQUEST)
      {
         GMSG_bReplyWithStatus(pMsgInfo, GMSG_STATUS_SEND_FAIL);
      }
      return;
   }
   if(!GSOCKET_bConnect(&SendSocket))
   {
      printf("GMSG_bSendSocket: connect to socket error");
      GSOCKET_bClose(&SendSocket);
      if (pMsgInfo->pMsgHeader->u32MsgFlags & GMSG_FLAG_REQUEST)
      {
         GMSG_bReplyWithStatus(pMsgInfo, GMSG_STATUS_SEND_FAIL);
      }
      return;
   }
   if(!GSOCKET_bSend(&SendSocket, (char*)pMsgInfo->pMsgHeader, sizeof(GMSG_HEADER) + (int)pMsgInfo->pMsgHeader->u32MsgSize))
   {
      printf("GMSG_bSendSocket: send error");
      GSOCKET_bClose(&SendSocket);
      if (pMsgInfo->pMsgHeader->u32MsgFlags & GMSG_FLAG_REQUEST)
      {
         GMSG_bReplyWithStatus(pMsgInfo, GMSG_STATUS_SEND_FAIL);
      }
      return;
   }
   GSOCKET_bClose(&SendSocket);
   if (bFreeMsgInfo)
   {
      GMSG_bFree(pMsgInfo);
      free(pMsgInfo);
   }
}

/******************************************************************************/
/**
 *
 *
 * @param pArg
 */
/******************************************************************************/
PRIVATE void GMSG_vSendQueueEntry(void *pArg)
{
   QUEUE SendQueue;
   GMSG_SEND_THREAD_INFO *pSendThreadInfo = (GMSG_SEND_THREAD_INFO*)pArg;
   GMSG_INFO *pMsgInfo = pSendThreadInfo->pMsgInfo;
   GMSG_ADDRESS DstAddress = pSendThreadInfo->DstAddress;
   BOOLEAN bFreeMsgInfo    = pSendThreadInfo->bFreeMsgInfo;

   free(pSendThreadInfo);
   if(!QUEUE_bOpen(&SendQueue, DstAddress.uAddr.Queue, QUEUE_MODE_WRONLY, FALSE))
   {
      printf("GMSG_bSendQueue: failed to open queue");
   }
   if(!QUEUE_bSend(&SendQueue, (char*)pMsgInfo->pMsgHeader, sizeof(GMSG_HEADER) + (int)pMsgInfo->pMsgHeader->u32MsgSize,
                   QUEUE_PRIORITY_NORMAL))
   {
      QUEUE_bClose(&SendQueue);
      printf("GMSG_bSendQueue: send error");
   }
   QUEUE_bClose(&SendQueue);
   if (bFreeMsgInfo)
   {
      GMSG_bFree(pMsgInfo);
      free(pMsgInfo);
   }
}

/******************************************************************************/
/**
 *
 *
 * @return GTHREAD_POOL*
 */
/******************************************************************************/
PRIVATE GTHREAD_POOL * GMSG_pGetMsgSendThreadPool(void)
{
   MUTEX_bLock(&s_SendThreadPoolMutex);
   if (!s_bSendThreadPoolInit)
   {
      GTHREAD_bInitPool(&s_SendThreadPool, FALSE);
      s_bSendThreadPoolInit = TRUE;
   }
   MUTEX_bUnlock(&s_SendThreadPoolMutex);
   return(&s_SendThreadPool);
}

/******************************************************************************
 *
 *    Function: GMSG_bSendSocket
 *
 *    Args:    pMsgInfo     - message info
 *             pDstAddress  - address to send message
 *                            NULL to send to address in message header
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: send a message to a socket mailbox
 *
 *    Notes:   header information already set
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bSendSocket(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pDstAddress, BOOLEAN bFreeMsgInfo)
{
   GMSG_SEND_THREAD_INFO *pSendThreadInfo = (GMSG_SEND_THREAD_INFO*)malloc(sizeof(GMSG_SEND_THREAD_INFO));
   GTHREAD_POOL *pMsgSendThreadPool = GMSG_pGetMsgSendThreadPool();

   if (pDstAddress == NULL)
   {
      pDstAddress = &pMsgInfo->pMsgHeader->DstAddress;
   }
   pSendThreadInfo->pMsgInfo     =  pMsgInfo;
   pSendThreadInfo->DstAddress   = *pDstAddress;
   pSendThreadInfo->bFreeMsgInfo =  bFreeMsgInfo;
   return(GTHREAD_bPushWorkByAddress(pMsgSendThreadPool, pDstAddress, GMSG_vSendSocketEntry, pSendThreadInfo));
}

/******************************************************************************
 *
 *    Function: GMSG_bSendQueue
 *
 *    Args:    pMsgInfo     - message info
 *             pDstAddress  - address to send message
 *                            NULL to send to address in message header
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: send a message to a queue mailbox
 *
 *    Notes:   header information already set
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bSendQueue(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pDstAddress, BOOLEAN bFreeMsgInfo)
{
   GMSG_SEND_THREAD_INFO *pSendThreadInfo = (GMSG_SEND_THREAD_INFO*)malloc(sizeof(GMSG_SEND_THREAD_INFO));
   GTHREAD_POOL *pMsgSendThreadPool = GMSG_pGetMsgSendThreadPool();

   if (pDstAddress == NULL)
   {
      pDstAddress = &pMsgInfo->pMsgHeader->DstAddress;
   }
   pSendThreadInfo->pMsgInfo     =  pMsgInfo;
   pSendThreadInfo->DstAddress   = *pDstAddress;
   pSendThreadInfo->bFreeMsgInfo =  bFreeMsgInfo;
   return(GTHREAD_bPushWorkByAddress(pMsgSendThreadPool, pDstAddress, GMSG_vSendQueueEntry, pSendThreadInfo));
}
/******************************************************************************
 *
 *    Function: GMSG_bSend
 *
 *    Args:    pMsgInfo    - message to send
 *             pDstAddress - destination address
 *                           NULL to send to address in message header
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Send a message to a mailbox (socket or queue)
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bSend(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pDstAddress, BOOLEAN bFreeMsgInfo)
{
   if (pDstAddress == NULL)
   {
      pDstAddress = &pMsgInfo->pMsgHeader->DstAddress;
   }
   if (pDstAddress->MsgAddressType == GMSG_ADDRESS_SOCKET)
   {
      return(GMSG_bSendSocket(pMsgInfo, pDstAddress, bFreeMsgInfo));
   }
   else if (pDstAddress->MsgAddressType == GMSG_ADDRESS_QUEUE)
   {
      return(GMSG_bSendQueue(pMsgInfo, pDstAddress, bFreeMsgInfo));
   }
   return(FALSE);
}


#endif


/******************************************************************************/
/**
 *
 *
 * @return U64
 */
/******************************************************************************/
PRIVATE U64 GMSG_u64GenerateToken(void)
{
#ifdef __RTXC__
  // return((U64)H_TICK_u32ReadFreeRunningTimerInSystemTicks());
  return(++s_u64Token);
#else
   GTIME tTime;
   U64 __u64ReplyToken;

   GTIME_bGetTime(&tTime);
   memcpy(&__u64ReplyToken, &tTime, sizeof(U64));
   return(__u64ReplyToken);
#endif
}

#ifdef __RTXC__
GLOBAL BOOLEAN GMSG_bMessageTimeout(GMSG_MAILBOX *pMailbox)
{
   GMSG_REQUEST_NODE *pNode;

   MUTEX_bLock(&pMailbox->MsgRequestInfo.RequestMutex);
   pNode = pMailbox->MsgRequestInfo.pHead;
   while (pNode != NULL)
   {
      if (pNode->MsgRequest.s32RequestTimeout > 0)
      {
         pNode->MsgRequest.s32RequestTimeout--;
      }
      pNode = pNode->pNext;
   }
   MUTEX_bUnlock(&pMailbox->MsgRequestInfo.RequestMutex);
   return(FALSE);
}

/******************************************************************************/
/**
 *
 *
 * @param pMsgInfo
 * @param pSrcAddress
 * @param pDstAddress
 * @param eMsgId
 * @param pDataType
 * @param pData
 * @param u64Token
 * @param __u64ReplyToken
 * @param pMsgRequestInfo
 * @param u32Flags
 *
 * @return BOOLDAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bCompose(GMSG_INFO *pMsgInfo, GMSG_ADDRESS *pSrcAddress, GMSG_ADDRESS *pDstAddress,
                             GMSG_ID_TYPE eMsgId, const G_DATA_TYPE *pDataType, void *pData, U64 u64Token,
                             U64 __u64ReplyToken, U32 u32Flags, BOOLEAN bUdp, U32 u32UdpLength)
{
   //create message
   if(!GMSG_bCreate(pMsgInfo, pSrcAddress, eMsgId, pDataType, pData, u32Flags))
   {
      return(FALSE);
   }

   //serialize message
   if (!bUdp)
   {
      if (!GMSG_bSerialize(pMsgInfo))
      {
         GMSG_bFree(pMsgInfo);
         return(FALSE);
      }
   }
   else
   {
      pMsgInfo->u32UdpLength = u32UdpLength;
   }

   //address message
   if(!GMSG_bAddress(pMsgInfo, pDstAddress, u64Token))
   {
      GMSG_bFree(pMsgInfo);
      return(FALSE);
   }

   if (u32Flags & GMSG_FLAG_REQUEST)
   {
      pMsgInfo->pMsgHeader->__u64ReplyToken = GMSG_u64GenerateToken();
   }
   else
   {
      pMsgInfo->pMsgHeader->__u64ReplyToken = __u64ReplyToken;
   }
   return(TRUE);
}

/******************************************************************************/
/**
 *
 *
 * @param pMsgRequestInfo
 * @param pMsgInfo
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bHandleRequests(GMSG_REQUEST_INFO *pMsgRequestInfo, GMSG_INFO *pMsgInfo)
{
   if (pMsgInfo->pMsgHeader->u32MsgFlags & GMSG_FLAG_REQUEST)
   {
      if (!GMSG_bAddRequest(pMsgRequestInfo, pMsgInfo))
      {
         printf("GMSG_bCommonSend: add request fail");
         return(FALSE);
      }
   }
   return(TRUE);
}

#else
/******************************************************************************
 *
 *    Function: GMSG_bCommonSend
 *
 *    Args:    pSrcAddress     - source GMSG_ADDRESS
 *             pDstAddress     - destination GMSG_ADDRESS
 *             eMsgId          - message id
 *             pDataType       - data type
 *             pData           - ptr to data parameter structure
 *             u64Token    - user-defined, passed back to user on response
 *             __u64ReplyToken      - internal token (unique generated by system) set
 *                               as NO_TOKEN to generate
 *             pMsgRequestInfo - if a request - set ptr to request info
 *             u32Flags        - flags
 *                                  GMSG_FLAG_REQUEST
 *                                  GMSG_FLAG_REPLY
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bCommonSend(GMSG_ADDRESS *pSrcAddress, GMSG_ADDRESS *pDstAddress,
                                GMSG_ID_TYPE eMsgId, const DATA_TYPE *pDataType, void *pData,
                                U64 u64Token, U64 __u64ReplyToken, GMSG_REQUEST_INFO *pMsgRequestInfo, U32 u32Flags)
{
   GMSG_INFO *pMsgInfo = (GMSG_INFO*)malloc(sizeof(GMSG_INFO));

   //create message
   if(!GMSG_bCreate(pMsgInfo, pSrcAddress, eMsgId, pDataType, pData, u32Flags))
   {
      free(pMsgInfo);
      return(FALSE);
   }

   //serialize message
   if (!GMSG_bSerialize(pMsgInfo))
   {
      GMSG_bFree(pMsgInfo);
      free(pMsgInfo);
      return(FALSE);
   }

   //address message
   if(!GMSG_bAddress(pMsgInfo, pDstAddress, u64Token))
   {
      GMSG_bFree(pMsgInfo);
      free(pMsgInfo);
      return(FALSE);
   }

   pMsgInfo->pMsgHeader->__u64ReplyToken = __u64ReplyToken;

   //handle requests
   if (u32Flags & GMSG_FLAG_REQUEST)
   {
      pMsgInfo->pMsgHeader->__u64ReplyToken = GMSG_u64GenerateToken();
      if (!GMSG_bAddRequest(pMsgRequestInfo, pMsgInfo))
      {
         printf("GMSG_bCommonSend: add request fail");
         GMSG_bFree(pMsgInfo);
         free(pMsgInfo);
         return(FALSE);
      }
   }

   //send message
   if(!GMSG_bSend(pMsgInfo, pDstAddress, TRUE))
   {
      printf("GMSG_bCommonSend: send fail");
      if (u32Flags & GMSG_FLAG_REQUEST)
      {
         GMSG_bDeleteRequestByToken(pMsgRequestInfo, pMsgInfo->pMsgHeader->__u64ReplyToken);
      }
      GMSG_bFree(pMsgInfo);
      free(pMsgInfo);
      return(FALSE);
   }

   //pthread_yield();
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bRequest
 *
 *    Args:    pMyMailbox     - source GMSG_MAILBOX
 *             pDstMailbox    - destination GMSG_MAILBOX
 *             eMsgId         - message id
 *             pDataType      - data type
 *             pData          - ptr to data parameter structure
 *             u64Token       - user-defined, passed back to user on response
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Send a message (with response) to a mailbox
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bRequest(GMSG_MAILBOX *pMyMailbox, GMSG_ADDRESS *pDstAddress,
                             GMSG_ID_TYPE eMsgId,const G_DATA_TYPE *pDataType,
                             void *pData, U64 u64Token)
{
   if (!GMSG_bCommonSend(&pMyMailbox->MsgAddress, pDstAddress, eMsgId, pDataType, pData,
                        u64Token, NO_TOKEN, &pMyMailbox->MsgRequestInfo, GMSG_FLAG_REQUEST))
   {
      printf("GMSG_bRequest: send request fail");
      return(FALSE);
   }
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GMSG_bSendNoReply
 *
 *             pSrcAddress    - source address
 *             pDstMailbox    - destination address
 *             eMsgId         - message id
 *             pDataType      - data type
 *             pData          - ptr to data parameter structure
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Send a message (no response) to a mailbox
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bSendNoReply(GMSG_ADDRESS *pSrcAddress, GMSG_ADDRESS *pDstAddress,
                                GMSG_ID_TYPE eMsgId, const G_DATA_TYPE *pDataType, void *pData)
{
   return(GMSG_bCommonSend(pSrcAddress, pDstAddress, eMsgId, pDataType, pData, NO_TOKEN, NO_TOKEN, NULL, 0));
}

/******************************************************************************/
/**
 * Send a message without a response
 *
 * @param pSrcAddress
 * @param pDstAddress
 * @param eMsgId
 * @param pDataType
 * @param pData
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bSendNoReplyWithToken(GMSG_ADDRESS *pSrcAddress, GMSG_ADDRESS *pDstAddress,
                                          GMSG_ID_TYPE eMsgId, const G_DATA_TYPE *pDataType,
                                          void *pData, U64 u64Token)
{
   return(GMSG_bCommonSend(pSrcAddress, pDstAddress, eMsgId, pDataType, pData, u64Token, NO_TOKEN, NULL, 0));
}

/******************************************************************************
 *
 *    Function: GMSG_bReply
 *
 *    Args:    pMsgInfo  - message info struct (from request)
 *             eMsgId    - message id of reply
 *             pDataType - data type of reply
 *             pData     - data for reply
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Reply to a message request
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bReply(GMSG_INFO *pMsgInfo, GMSG_ID_TYPE eMsgId, const G_DATA_TYPE *pDataType, void *pData)
{
   return(GMSG_bCommonSend(&pMsgInfo->pMsgHeader->DstAddress, &pMsgInfo->pMsgHeader->SrcAddress, eMsgId,
                           pDataType, pData, pMsgInfo->pMsgHeader->u64Token, pMsgInfo->pMsgHeader->__u64ReplyToken,
                           NULL, GMSG_FLAG_REPLY));
}

/******************************************************************************
 *
 *    Function: GMSG_bReplyWithStatus
 *
 *    Args:    pMsgInfo     - ptr to GMSG_INFO
 *             u32MsgStatus - Status code
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: reply to a message request with GMSG_ID_STATUS message
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bReplyWithStatus(GMSG_INFO *pMsgInfo, U32 u32MsgStatus)
{
   GMSG_STATUS MsgStatus;

   MsgStatus.u32MsgId      = pMsgInfo->pMsgHeader->u32MsgId;
   MsgStatus.u32MsgStatus  = u32MsgStatus;
   MsgStatus.u64Token      = pMsgInfo->pMsgHeader->u64Token;
   return(GMSG_bReply(pMsgInfo, GMSG_ID_STATUS, DT_GMSG_STATUS, &MsgStatus));
}

/******************************************************************************
 *
 *    Function: GMSG_bSendSocketUDP
 *
 *    Args:    -none-
 *
 *    Return:  -none-
 *
 *    Purpose: send data to a socket
 *
 *    Notes:   header information already set
 *
 ******************************************************************************/
GLOBAL BOOLEAN GMSG_bSendSocketUDP(GSOCKET socket, unsigned char *pBuffer, int nLen, struct sockaddr *dstAddr)
{
   int nSendLength = 0;
   socklen_t nSendToSize = sizeof(struct sockaddr);

   if (nLen)
   {
      nSendLength = sendto((int)socket.nSocket, pBuffer, nLen, 0, (struct sockaddr *)dstAddr, nSendToSize);
      if (nSendLength < 0)
      {
         printf("GMSG_bSendSocketUDP: send error");
         return(FALSE);
      }
      return(TRUE);
   }
   return(FALSE);
}
#endif

/******************************************************************************/
/**
 *
 *
 * @param pAddress
 * @param u32Address
 * @param u32Port
 */
/******************************************************************************/
GLOBAL void    GMSG_vMakeSocketAddress(GMSG_ADDRESS *pAddress, U32 u32Address, U32 u32Port)
{
   pAddress->MsgAddressType = GMSG_ADDRESS_SOCKET;
   pAddress->uAddr.Socket.u32Address = u32Address;
   pAddress->uAddr.Socket.u32Port    = u32Port;
   pAddress->uAddr.Socket.u32Unused1 = 0;
   pAddress->uAddr.Socket.u32Unused2 = 0;
}

/******************************************************************************/
/**
 * Peek to see if there is a msg available
 *
 * @param pSocket
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bMsgAvailable(GSOCKET *pSocket, GMSG_HEADER *pHeader)
{
   GMSG_HEADER Header;

   if (pHeader == NULL)
   {
      pHeader = &Header;
   }
   return ((sizeof(GMSG_HEADER) == GSOCKET_nDataAvailable(pSocket, (char*)pHeader, sizeof(GMSG_HEADER))) ? TRUE : FALSE);
}

/******************************************************************************/
/**
 * Peek to see if there is a msg available
 *
 * @param pSocket
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bMsgAvailableUdp(GSOCKET *pSocket, GMSG_HEADER *pHeader)
{
   GMSG_HEADER Header;

   if (pHeader == NULL)
   {
      pHeader = &Header;
   }
   return ((1 == GSOCKET_nDataAvailableUdp(pSocket, (char*)pHeader, 1))?TRUE:FALSE);
}

/******************************************************************************/
/**
 * Peek to see if there is a msg available
 *
 * @param pSocket
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN GMSG_bMsgAvailableAll(GSOCKET *pSocket, GMSG_HEADER *pHeader, BOOLEAN bUdp)
{
   if (bUdp)
      return(GMSG_bMsgAvailableUdp(pSocket,pHeader));
   else
      return(GMSG_bMsgAvailable(pSocket,pHeader));
}
//#endif //MODEL_HAS_NETWORKING

