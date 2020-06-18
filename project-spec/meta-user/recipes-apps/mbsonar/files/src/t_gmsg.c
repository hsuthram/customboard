/*******************************************************************************
*           Copyright (c) 2010  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
*******************************************************************************/

/**
 * @file t_gmsg.c
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
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include <string.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>

//#include "matrixOS.h"
//#include "model.h"
#include "lsvc.h"
#include "gmsg.h"
#include "message.h"
#include "error.h"
#include "service.h"
#include "gsocket.h"
//#include "h_memory.h"
#include "h_database.h"
#include "u_gmsg.h"
#include "t_gmsg.h"
#include "h_ping.h"
#include "u_log.h"
//#include "t_ethernet.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define MAX_PROCESS_MAILBOX_COUNT   5

/********************               MACROS                   ******************/
#ifndef FD_COPY
   #define  FD_COPY(d, s)  memcpy((char *)(d), (char *)(s), sizeof(*(s)))
#endif

/********************         TYPE DEFINITIONS               ******************/

/********************        FUNCTION PROTOTYPES             ******************/
PRIVATE void T_GMSG_vInit(void);
PRIVATE void T_GMSG_vHandleShutdown(void);
PRIVATE void T_GMSG_vHandleRegister(MSG_GMSG_REGISTER *pMsgRegister);
PRIVATE void T_GMSG_vHandleUnregister(MSG_GMSG_UNREGISTER *pMsgUnregister);
PRIVATE void T_GMSG_vHandleNewMessage(MSG_GMSG_NEW_MESSAGE *pMsgNew);
PRIVATE void T_GMSG_vHandleNewMessageExt(MSG_GMSG_NEW_MESSAGE_EXT *pMsgNew);
PRIVATE void T_GMSG_vHandleSendMessage(MSG_GMSG_SEND *pMsgSend);
PRIVATE void T_GMSG_vOnTimer(void);

/********************          LOCAL VARIABLES               ******************/
PRIVATE CLKBLK               *s_pGmsgTimer;
PRIVATE BOOLEAN               s_bShutdown = FALSE;
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/

GLOBAL void T_GMSG_vMain(void)
{
   GENERIC_MESSAGE  *pMsg;
   SEMA              semalist[] = {
                                    SEMA_TIMER_GMSG,
                                    SEMA_MBOX_GMSG,
                                    0
                                  };

   T_GMSG_vInit();

   while (1)
   {
      switch (KS_waitm(semalist))
      {
         case SEMA_MBOX_GMSG:
            while ((pMsg = (GENERIC_MESSAGE *)KS_receive( M_GMSG, SELFTASK)) != NULL)
            {
               switch (pMsg->Cmd)
               {
                  case CMD_ID_SHUTDOWN:
                     T_GMSG_vHandleShutdown();
                     break;
                  case CMD_ID_GMSG_REGISTER:
		     U_PrintLog("got CMD_ID_GMSG_REGISTER");
                     T_GMSG_vHandleRegister((MSG_GMSG_REGISTER*)pMsg);
                     break;
                  case CMD_ID_GMSG_UNREGISTER:
                     T_GMSG_vHandleUnregister((MSG_GMSG_UNREGISTER*)pMsg);
                     break;
                  case CMD_ID_GMSG_NEW_MESSAGE:
                     T_GMSG_vHandleNewMessage((MSG_GMSG_NEW_MESSAGE *)pMsg);
                     break;
                  case CMD_ID_GMSG_NEW_MESSAGE_EXT:
                     T_GMSG_vHandleNewMessageExt((MSG_GMSG_NEW_MESSAGE_EXT *)pMsg);
                     break;
                  case CMD_ID_GMSG_SEND:
                     T_GMSG_vHandleSendMessage((MSG_GMSG_SEND *)pMsg);
                     break;
                  default:
                     break;
               }
               free((RTXCMSG *)pMsg);
            }
            break;
         case SEMA_TIMER_GMSG:
            T_GMSG_vOnTimer();
            break;
         default:
            break;
      }
   }
}

/******************************************************************************/
/**
 * Initialize the GMSG task
 */
/******************************************************************************/
PRIVATE void T_GMSG_vInit(void)
{
   // Announce the task mailbox
//   if ( H_DATABASE_bUpdateEverythingU32( TMSG, (U32)M_GMSG, P_NORMAL, D_MAILBOX, NULL, NO_TIMEOUT ) )
//   {
//      HANDLE_ERROR( "Somebody else registered the GMSG Task Maibox" );
//   }

   //set up a timer to check for message timeouts
   s_pGmsgTimer = KS_alloc_timer();
   KS_start_timer(s_pGmsgTimer, ONE_SEC/4, ONE_SEC/4, SEMA_TIMER_GMSG);
}

/******************************************************************************/
/**
 * Shutdown the GMSG task
 */
/******************************************************************************/
PRIVATE void T_GMSG_vHandleShutdown(void)
{
   /* Tasks are responsible for unregistering own mailboxes */
   s_bShutdown = TRUE;
   KS_stop_timer(s_pGmsgTimer);
   KS_free_timer(s_pGmsgTimer);
   U_GMSG_vShutdown();
}

/******************************************************************************/
/**
 *
 *
 * @param pMsgRegister
 */
/******************************************************************************/
PRIVATE void T_GMSG_vHandleRegister(MSG_GMSG_REGISTER *pMsgRegister)
{
   pMsgRegister->bSuccess = (NULL != U_GMSG_pAddNewMailboxNode(pMsgRegister->pMsgMapInfo, pMsgRegister->u16Port,
                             pMsgRegister->Mailbox, pMsgRegister->ResLock, pMsgRegister->Sema,pMsgRegister->bUdp, pMsgRegister->u32McIp)) ? TRUE : FALSE;
   printf("[%d]%s: bSuccess %d, signal mail box %d\n", __LINE__, __FUNCTION__, pMsgRegister->bSuccess, pMsgRegister->Mailbox);
   U_GMSG_bSignal(pMsgRegister->Mailbox);
}

/******************************************************************************/
/**
 *
 *
 * @param pMsgUnregister
 */
/******************************************************************************/
PRIVATE void T_GMSG_vHandleUnregister(MSG_GMSG_UNREGISTER *pMsgUnregister)
{
   pMsgUnregister->bSuccess = U_GMSG_bDeleteMailboxNodeById(pMsgUnregister->Mailbox);
   U_GMSG_bSignal(pMsgUnregister->Mailbox);
}

/******************************************************************************/
/**
 *
 *
 * @param pMsgNew
 */
/******************************************************************************/
PRIVATE void T_GMSG_vHandleNewMessage(MSG_GMSG_NEW_MESSAGE *pMsgNew)
{
   GMSG_REG_MAILBOX_NODE *pMailboxNode= NULL;
   GMSG_SOCKET_NODE      *pSocketNode = NULL;
   BOOLEAN                bError = FALSE;
   int                    nMsgCount = 0;
   BOOLEAN                bReceiveMessage = FALSE;

   //Find the Mailbox node (we already acquired it)
   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeById(pMsgNew->Mailbox)))
   {
      if (pMailboxNode->Mailbox.bUdp)
      {
         while ((nMsgCount < 1) && GMSG_bMsgAvailableAll(&pMailboxNode->Mailbox.MsgObject.Socket, NULL, pMailboxNode->Mailbox.bUdp))
         {
            MSG_GMSG_RECEIVE *pGmsgReceive = (MSG_GMSG_RECEIVE*)malloc(sizeof(MSG_GMSG_RECEIVE));

            memset(pGmsgReceive, 0, sizeof(MSG_GMSG_RECEIVE));
            bReceiveMessage = GMSG_bReceiveUDPSocket(&pMailboxNode->Mailbox.MsgObject.Socket, &pGmsgReceive->MsgInfo);

            if (bReceiveMessage)
            {
               pGmsgReceive->header.Cmd = CMD_ID_GMSG_RECEIVE;
               pGmsgReceive->Mailbox    = pMailboxNode->Mailbox.RtxcMailbox;
               U_GMSG_bAcquireMailboxNode(pMailboxNode, TRUE);
               nMsgCount++;
               //send receive message to application
               KS_send(pMailboxNode->Mailbox.RtxcMailbox, (RTXCMSG*)pGmsgReceive, NORMAL_PRIORITY, (SEMA)0);
            }
            else
            {
               //Something went wrong with receiving on this socket
               free(pGmsgReceive);
               bError = TRUE;
               break;
            }
         }
      }
      else
      {
         //Find the socket node (we already acquired it)
         if (NULL != (pSocketNode = U_GMSG_pFindSocketNodeByFd(&pMailboxNode->Mailbox, pMsgNew->nSocket)))
         {
            while ((nMsgCount < MAX_PROCESS_MAILBOX_COUNT) && GMSG_bMsgAvailableAll(&pSocketNode->MsgSocket.Socket, NULL, pMailboxNode->Mailbox.bUdp))
            {
               MSG_GMSG_RECEIVE *pGmsgReceive = (MSG_GMSG_RECEIVE*)malloc(sizeof(MSG_GMSG_RECEIVE));

               //Receive the Message from the socket
               bReceiveMessage = GMSG_bReceiveMessage(&pMailboxNode->Mailbox, &pSocketNode->MsgSocket.Socket, &pGmsgReceive->MsgInfo);
               if (bReceiveMessage)
               {
                  //update socket address if not alraedy set
                  if (!pSocketNode->bAddressSet)
                  {
                     U_GMSG_bUpdateSocketNodeAddress(&pMailboxNode->Mailbox, pSocketNode, &pGmsgReceive->MsgInfo.pMsgHeader->SrcAddress);
                  }

                  pGmsgReceive->header.Cmd = CMD_ID_GMSG_RECEIVE;
                  pGmsgReceive->Mailbox    = pMailboxNode->Mailbox.RtxcMailbox;
                  U_GMSG_bAcquireMailboxNode(pMailboxNode, TRUE);
                  nMsgCount++;
                  //send receive message to application
                  KS_send(pMailboxNode->Mailbox.RtxcMailbox, (RTXCMSG*)pGmsgReceive, NORMAL_PRIORITY, (SEMA)0);
               }
               else
               {
                  //Something went wrong with receiving on this socket
                  free(pGmsgReceive);
                  bError = TRUE;
                  break;
               }
            }
            //done with socket, release it
            U_GMSG_bReleaseSocketNode(&pMailboxNode->Mailbox, pSocketNode, TRUE);
            if (bError)
            {
               //let's clean up and release resources
               U_GMSG_bDeleteSocketNode(&pMailboxNode->Mailbox, pSocketNode);
               printf("T_GMSG_vHandleNewMessage: Receive Message Failure");
            }
         }
         else
         {
            //This shouldn't happen
            printf("T_GMSG_vHandleNewMessage: Socket Node not found");
         }
      }
      U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
   }
   else
   {
      //This shouldn't happen
      printf("T_GMSG_vHandleNewMessage: Mailbox Node not found");
   }
   U_GMSG_bSignal(pMsgNew->Mailbox);
}
/******************************************************************************/
/**
 *
 *
 * @param pMsgNew
 */
/******************************************************************************/
PRIVATE void T_GMSG_vHandleNewMessageExt(MSG_GMSG_NEW_MESSAGE_EXT *pMsgNew)
{
   GMSG_REG_MAILBOX_NODE  *pMailboxNode= NULL;
   BOOLEAN                 bError = FALSE;

   //Find the Mailbox node (we already acquired it)
   if (NULL != (pMailboxNode = U_GMSG_pFindMailboxNodeById(pMsgNew->Mailbox)))
   {
      MSG_GMSG_RECEIVE *pGmsgReceive = (MSG_GMSG_RECEIVE*)malloc(sizeof(MSG_GMSG_RECEIVE));

      //Receive the Message from the socket
      if (GMSG_bReceiveSocketAdapter(pMsgNew->cpMessageBody, pMsgNew->u32MessageLength, &pGmsgReceive->MsgInfo))
      {
         pGmsgReceive->header.Cmd = CMD_ID_GMSG_RECEIVE;
         pGmsgReceive->Mailbox    = pMailboxNode->Mailbox.RtxcMailbox;
         U_GMSG_bAcquireMailboxNode(pMailboxNode, TRUE);
         //send receive message to application
         KS_send(pMailboxNode->Mailbox.RtxcMailbox, (RTXCMSG*)pGmsgReceive, NORMAL_PRIORITY, (SEMA)0);
      }
      else
      {
         //Something went wrong with receiving on this socket
         free(pGmsgReceive);
         bError = TRUE;
      }
   }
   else
   {
      //This shouldn't happen
      printf("T_GMSG_vHandleNewMessage: Socket Node not found");
   }
   U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
   U_GMSG_bSignal(pMsgNew->Mailbox);
}

/******************************************************************************/
/**
 *
 *
 * @param pMsgSend
 */
/******************************************************************************/
PRIVATE void T_GMSG_vHandleSendMessage(MSG_GMSG_SEND *pMsgSend)
{
   GMSG_REG_MAILBOX_NODE  *pMailboxNode;
   GMSG_SOCKET_NODE       *pSocketNode;
   BOOLEAN                 bError = FALSE;

/*   if (!T_ETHERNET_bIsAddressReady())
   {
      bError = TRUE;
   }
   //Acquire the Mailbox node
   else*/ if (NULL != (pMailboxNode = U_GMSG_pAcquireMailboxNodeById( pMsgSend->Mailbox, FALSE)))
   {
      pMsgSend->MsgInfo.pMsgHeader->SrcAddress = pMailboxNode->Mailbox.MsgAddress;

      if (pMailboxNode->Mailbox.bUdp)
      {
         struct sockaddr_in dstAddr;
         dstAddr.sin_family = AF_INET;
         dstAddr.sin_addr.s_addr = pMsgSend->MsgInfo.pMsgHeader->DstAddress.uAddr.Socket.u32Address;
         dstAddr.sin_port = htons(pMsgSend->MsgInfo.pMsgHeader->DstAddress.uAddr.Socket.u32Port);

         if (!GMSG_bSendSocketUDP(pMailboxNode->Mailbox.MsgObject.Socket, (unsigned char *)pMsgSend->MsgInfo.pStructMsg,
                                  pMsgSend->MsgInfo.u32UdpLength,(struct sockaddr *)&dstAddr))
         {
            bError = TRUE;
            printf("T_GMSG_vHandleSendMessage: Handle UDP Send Failure");
         }
      }
      else
      {
         if (NULL == (pSocketNode = U_GMSG_pAcquireSocketNodeByAddress(&pMailboxNode->Mailbox, &pMsgSend->MsgInfo.pMsgHeader->DstAddress, FALSE)))
         {
            pSocketNode = U_GMSG_pAddNewSocketNode(&pMailboxNode->Mailbox, &pMsgSend->MsgInfo.pMsgHeader->DstAddress, FALSE, TRUE);
         }
         if (pSocketNode != NULL)
         {
            if (!GMSG_bHandleRequests(&pMailboxNode->Mailbox.MsgRequestInfo, &pMsgSend->MsgInfo))
            {
               bError = TRUE;
               printf("T_GMSG_vHandleSendMessage: Handle Request Failure");
            }
            else if (!GMSG_bSendSocket(&pMsgSend->MsgInfo, &pSocketNode->MsgSocket.Socket, FALSE))
            {
               bError = TRUE;
               printf("T_GMSG_vHandleSendMessage: Send Failure");
            }
            if (bError)
            {
               U_GMSG_bDeleteSocketNode(&pMailboxNode->Mailbox, pSocketNode);
            }
            //Release Socket node
            U_GMSG_bReleaseSocketNode(&pMailboxNode->Mailbox, pSocketNode, FALSE);
         }
         else
         {
            bError = TRUE;
            U_GMSG_bReleaseMailboxNode(pMailboxNode, FALSE);
            printf("T_GMSG_vHandleSendMessage: Socket Node not found");
         }

      }
      //Release Mailbox Node
      U_GMSG_bReleaseMailboxNode(pMailboxNode, FALSE);
   }
   else
   {
      //This shouldn't happen
      bError = TRUE;
      printf("T_GMSG_vHandleNewMessage: Mailbox Node not found");
   }
   pMsgSend->bSuccess = !bError;
   //Free Message Info structure
   GMSG_bFree(&pMsgSend->MsgInfo);
}

/******************************************************************************/
/**
 * Handle Timouts
 */
/******************************************************************************/
PRIVATE void T_GMSG_vOnTimer(void)
{
   U_GMSG_vOnTimer();
}

/******************************************************************************/
/**
 * Helper Task to Accept Socket connections
 */
/******************************************************************************/
GLOBAL void T_GMSG_vSocketMain(void)
{
   BOOLEAN bActiveOrNoSockets = TRUE;

   while (1) 
   {
      if (s_bShutdown || bActiveOrNoSockets)
      {
         KS_wait(SEMA_GMSG_SELECT);
         KS_pend(SEMA_GMSG_SELECT);
         bActiveOrNoSockets = FALSE;
      }
      else
      {
         fd_set                  readFds;
         fd_set                  exceptFds;
         int                     nMaxSocket;

         FD_ZERO( &readFds );
         nMaxSocket = 0;
         if (U_GMSG_bGetMailboxFdSet(&readFds, &nMaxSocket))
         {
#ifdef linux
            struct timeval timeout = {0, 7500}; //WAS 250000};
#else
            struct timeval timeout = {0, 100000}; //WAS 250000};
#endif


            FD_COPY( &exceptFds, &readFds );
            if (0 < select(nMaxSocket+1, &readFds, NULL, &exceptFds, &timeout))
            {
               int nSocket;

               for (nSocket = 0; nSocket <= nMaxSocket; nSocket++)
               {
                  if (FD_ISSET( nSocket, &readFds ))
                  {
                     GMSG_REG_MAILBOX_NODE *pMailboxNode = NULL;
                     GMSG_SOCKET_NODE      *pSocketNode = NULL;
                     BOOLEAN                bListenSocket = FALSE;

                     if (NULL == (pMailboxNode = U_GMSG_pAcquireMailboxNodeByFd(nSocket, &bListenSocket, TRUE)))
                     {
                        continue;
                     }
                     if (pMailboxNode->Mailbox.bUdp)
                     {
                        if (NULL == (pSocketNode = U_GMSG_pAcquireSocketNodeByFd(&pMailboxNode->Mailbox, nSocket, TRUE)))
                        {
                           U_GMSG_pAddNewSocketNodeUdp(&pMailboxNode->Mailbox, NULL, TRUE, FALSE);
                           if (!pSocketNode)
                           {
                              U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
                              continue;
                           }
                        }

                        if (GMSG_bMsgAvailableUdp(&pSocketNode->MsgSocket.Socket,NULL))
                        {
                           MSG_GMSG_NEW_MESSAGE *pGmsgNew = (MSG_GMSG_NEW_MESSAGE*)malloc(sizeof(MSG_GMSG_NEW_MESSAGE));
                           pGmsgNew->nSocket          = pSocketNode->MsgSocket.Socket.nSocket;
                           pGmsgNew->Mailbox          = pMailboxNode->Mailbox.RtxcMailbox;
                           pGmsgNew->header.Cmd       = CMD_ID_GMSG_NEW_MESSAGE;
                           KS_send(M_GMSG, (RTXCMSG*)pGmsgNew, NORMAL_PRIORITY, (SEMA)0);
                        }
                        else
                        {
                           U_GMSG_bReleaseSocketNode(&pMailboxNode->Mailbox, pSocketNode, TRUE);
                           U_GMSG_bDeleteSocketNode(&pMailboxNode->Mailbox, pSocketNode);
                           U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
                        }
                     }
                     else if (bListenSocket)
                     {
                        U_GMSG_pAddNewSocketNode(&pMailboxNode->Mailbox, NULL, TRUE, FALSE);
                        U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
                        continue;
                     }
                     else
                     {
                        if (NULL == (pSocketNode = U_GMSG_pAcquireSocketNodeByFd(&pMailboxNode->Mailbox, nSocket, TRUE)))
                        {
                           U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
                           continue;
                        }
                        if (GMSG_bMsgAvailable(&pSocketNode->MsgSocket.Socket, NULL))
                        {
                           MSG_GMSG_NEW_MESSAGE *pGmsgNew = (MSG_GMSG_NEW_MESSAGE*)malloc(sizeof(MSG_GMSG_NEW_MESSAGE));
                           pGmsgNew->nSocket          = pSocketNode->MsgSocket.Socket.nSocket;
                           pGmsgNew->Mailbox          = pMailboxNode->Mailbox.RtxcMailbox;
                           pGmsgNew->header.Cmd       = CMD_ID_GMSG_NEW_MESSAGE;
                           KS_send(M_GMSG, (RTXCMSG*)pGmsgNew, NORMAL_PRIORITY, (SEMA)0);
                        }
                        else
                        {
                           U_GMSG_bReleaseSocketNode(&pMailboxNode->Mailbox, pSocketNode, TRUE);
                           U_GMSG_bDeleteSocketNode(&pMailboxNode->Mailbox, pSocketNode);
                           U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
                        }
                     }
                  }
                  else if (FD_ISSET( nSocket, &exceptFds ))
                  {
                     GMSG_REG_MAILBOX_NODE *pMailboxNode = NULL;
                     GMSG_SOCKET_NODE      *pSocketNode = NULL;
                     BOOLEAN                bListenSocket = FALSE;

                     if (NULL == (pMailboxNode = U_GMSG_pAcquireMailboxNodeByFd(nSocket, &bListenSocket, TRUE)))
                     {
                        continue;
                     }
                     if (!bListenSocket)
                     {
                        if (NULL == (pSocketNode = U_GMSG_pAcquireSocketNodeByFd(&pMailboxNode->Mailbox, nSocket, TRUE)))
                        {
                           U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
                           continue;
                        }
                        U_GMSG_bReleaseSocketNode(&pMailboxNode->Mailbox, pSocketNode, TRUE);
                        U_GMSG_bDeleteSocketNode(&pMailboxNode->Mailbox, pSocketNode);
                        U_GMSG_bReleaseMailboxNode(pMailboxNode, TRUE);
                     }
                  }
               }
            }
         }
         else
         {
            bActiveOrNoSockets = TRUE;
         }
      }
   }
}
