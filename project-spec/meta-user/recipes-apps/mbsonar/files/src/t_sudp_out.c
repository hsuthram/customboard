#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "lsvc.h"
#include "types.h"
#include "message.h"
#include "t_gmsg.h"
#include "t_sudp_out.h"
#include "u_log.h"
#include "rtx2lsvc.h"
/********************              DEFINES                   ******************/
#define PACKET_PAYLOAD_SIZE (1024)
//#define PACKET_PAYLOAD_SIZE 4096
#define PACKET_HEADER_SIZE (4 * sizeof(unsigned))
/********************          LOCAL VARIABLES               ******************/
PRIVATE S32 s_s32Socket = 0;
PRIVATE U32 s_u32LocalAddress = 0;
PRIVATE char s_str[64];
/********************          FUNCTIONS                     ******************/
/******************************************************************************
 *
 *    Function:   T_SUDP_OUT_pucPacketBufferPut
 *
 *    Args:
 *
 *    Return:
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE unsigned char *T_SUDP_OUT_pucPacketBufferPut(unsigned char *pb, unsigned char  *ub, int len)
{
   while (len--)
   {
      *pb++ = *ub++;
   }

   return pb;
}

/******************************************************************************
 *
 *    Function:   T_SUDP_OUT_s32SudpSend
 *
 *    Args:
 *
 *    Return:
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE S32 T_SUDP_OUT_s32SudpSend(unsigned char *message, int len, unsigned messageSequenceNumber, int udpSocket, struct sockaddr *destAddr)
{
   unsigned char packetBuffer[PACKET_PAYLOAD_SIZE + PACKET_HEADER_SIZE];
   unsigned      messagePartSequenceNumber = 0;
   unsigned      messageTotalPartsCount = (unsigned) ceil((double) len / (double) PACKET_PAYLOAD_SIZE);
   unsigned      messageTotalSize = len;
   unsigned      packetCount;
   ssize_t       addrlength;
   #ifdef DEBUG_SIGNATURE
   MD5_CTX c;
   int n;
   unsigned char out[MD5_DIGEST_LENGTH];

   MD5_Init(&c);
   MD5_Update(&c, message, len);
   MD5_Final(out, &c);

   for(n=0; n < MD5_DIGEST_LENGTH; n++)
       PRINTF("%02x", out[n]);
   PRINTF("\n");
   #endif

#if 0
   {
      int i;

      for (i = 0; i < len; i++)
      {
         printf("%02X ", message[i]);

	 if (((i + 1) % 32) == 0)
	 {
            printf("\n");
	 }
      }

      printf("\n");
   }
#endif

//   sprintf(s_str, "messageTotalPartsCount %d", messageTotalPartsCount);
//   U_PrintLog(s_str);

   for (packetCount = 0; packetCount < messageTotalPartsCount; packetCount++)
   {
      unsigned char *packetBufferPtr = packetBuffer;
      int            packetPayloadLen = len > PACKET_PAYLOAD_SIZE ? PACKET_PAYLOAD_SIZE : len;
      int            iSent = 0;

      packetBufferPtr = T_SUDP_OUT_pucPacketBufferPut(packetBufferPtr, (unsigned char *)&messageSequenceNumber, sizeof(unsigned));
      packetBufferPtr = T_SUDP_OUT_pucPacketBufferPut(packetBufferPtr, (unsigned char *)&messagePartSequenceNumber, sizeof(unsigned));
      packetBufferPtr = T_SUDP_OUT_pucPacketBufferPut(packetBufferPtr, (unsigned char *)&messageTotalPartsCount, sizeof(unsigned));
      packetBufferPtr = T_SUDP_OUT_pucPacketBufferPut(packetBufferPtr, (unsigned char *)&messageTotalSize, sizeof(unsigned));
      packetBufferPtr = T_SUDP_OUT_pucPacketBufferPut(packetBufferPtr, message, packetPayloadLen);
      message += packetPayloadLen;
      len -= packetPayloadLen;
      ++messagePartSequenceNumber;
      addrlength = sizeof(struct sockaddr_in);

      //we have delay because the receive side is slower than the send side
      //and we will get packet mismatches if we don't
      KS_delay(SELFTASK, 1);

#if 0
      {
         int i;

         for (i = 0; i < PACKET_PAYLOAD_SIZE + PACKET_HEADER_SIZE; i++)
         {
            printf("%02X ", packetBuffer[i]);

   	    if (((i + 1) % 32) == 0)
            {
               printf("\n");
	    }
         }

         printf("\n");
      }
#endif

      iSent = sendto(udpSocket, packetBuffer, PACKET_PAYLOAD_SIZE + PACKET_HEADER_SIZE, 0, destAddr, addrlength);
//      printf("[%d]%s: %d bytes sent to socket %d\n", __LINE__, __FUNCTION__, iSent, udpSocket);
   }

   return 0;
}

/******************************************************************************
 *
 *    Function:   T_SUDP_OUT_s32RawSend
 *
 *    Args:
 *
 *    Return:
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE S32 T_SUDP_OUT_s32RawSend(unsigned char *message, int len, int udpSocket, struct sockaddr *destAddr)
{
   ssize_t addrlength = sizeof(struct sockaddr_in);

   sendto(udpSocket, message, len, 0, destAddr, addrlength);
   return 0;
}

/******************************************************************************
 *
 *    Function:   T_SUDP_OUT_vHandleSend
 *
 *    Args:
 *
 *    Return:
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void T_SUDP_OUT_vHandleSend(MSG_SUDP_OUT_SEND *pMsg)
{
   static unsigned    messageSequenceNumber = 0;
   struct sockaddr_in destAddress;

   destAddress.sin_family = AF_INET;
   destAddress.sin_port = htons(pMsg->u32Port);
   destAddress.sin_addr.s_addr = pMsg->u32Address;
#if 0
   sprintf(s_str, "size of message %d, send to port %d address 0x%x", pMsg->u32MessageLength, pMsg->u32Port, destAddress.sin_addr.s_addr);
   U_PrintLog(s_str);

   {
      int i;

      for (i = 0; i < pMsg->u32MessageLength; i++)
      {
         printf("%02X ", pMsg->ucpMessage[i]);
      }

      printf("\n");
   }
#endif

   if (pMsg->u32MultiPacket)
   {
      T_SUDP_OUT_s32SudpSend(pMsg->ucpMessage, pMsg->u32MessageLength, messageSequenceNumber++, s_s32Socket, (struct sockaddr *)&destAddress);
   }
   else
   {
      T_SUDP_OUT_s32RawSend(pMsg->ucpMessage, pMsg->u32MessageLength, s_s32Socket, (struct sockaddr *)&destAddress);
   }

//   printf("[%d]%s: free 0x%x\n", __LINE__, __FUNCTION__, pMsg->ucpMessage);
   free(pMsg->ucpMessage);
}

/******************************************************************************
 *
 *    Function:   T_SUDP_OUT_vMain
 *
 *    Args:
 *
 *    Return:
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void T_SUDP_OUT_vMain(void)
{
   GENERIC_MESSAGE *pMsg;
   SEMA             s_wait_list[] = {SEMA_MBOX_SUDPOUT,
                                     0};
   char             loopch = 0;
   struct in_addr   localInterface;

   U_PrintLog("I'm alive!");
   s_s32Socket = socket(AF_INET, SOCK_CLOEXEC|SOCK_DGRAM, 0);
   GSOCKET_bGetLocalAddress(&s_u32LocalAddress, "eth0");  //on target
   printf("IP address is %d.%d.%d.%d, socket %d\n", (U8)s_u32LocalAddress, (U8)(s_u32LocalAddress >> 8), (U8)(s_u32LocalAddress >> 16), (U8)(s_u32LocalAddress >> 24), s_s32Socket);

   if (s_u32LocalAddress > 0)
   {
      if (setsockopt(s_s32Socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loopch, sizeof(loopch)) < 0)
      {
         U_PrintLog("Error IP_MULTICAST_LOOP");
      }

      localInterface.s_addr = s_u32LocalAddress;

      if (setsockopt(s_s32Socket, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
      {
         U_PrintLog("Error IP_MULTICAST_IF");
      }
   }
   else
   {
      U_PrintLog("We had a problem.");
      return;
   }

   while (1)
   {
      switch (KS_waitm(s_wait_list))
      {
         case SEMA_MBOX_SUDPOUT:
            //we have mail
            while ((pMsg = (GENERIC_MESSAGE *)L_MBOX_Receive(M_SUDPOUT)) != NULL)
            {
               switch (pMsg->Cmd)
               {
                  case CMD_ID_SUDP_OUT_SEND:
//                     U_PrintLog("got CMD_ID_SUDP_OUT_SEND");
                     T_SUDP_OUT_vHandleSend((MSG_SUDP_OUT_SEND *)pMsg);
                     break;
                  default:
                     break;
               }

               free(pMsg);
            }

            break;
         default:
            printf("[%d]%s: got message id %d\n", __LINE__, __FUNCTION__, pMsg->Cmd);
	    break;
      }
   }
}



