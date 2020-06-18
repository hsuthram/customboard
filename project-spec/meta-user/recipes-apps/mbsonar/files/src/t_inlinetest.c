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
#define PORT    50002
#define MAXLINE 1024
#define MY_IP   "10.111.222.32"
/********************          LOCAL VARIABLES               ******************/
PRIVATE S32  s_s32Socket = 0;
PRIVATE U32  s_u32LocalAddress = 0;
PRIVATE char s_cMyLocalAddress[16] = "10.111.222.30";
/********************          FUNCTIONS                     ******************/
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
GLOBAL void T_INLINETEST_vMain(void)
{
   GENERIC_MESSAGE *pMsg;
   SEMA             s_wait_list[] = {SEMA_MBOX_INLINETEST,
                                     0};

   U_PrintLog("I'm alive!");
   s_s32Socket = socket(AF_INET, SOCK_CLOEXEC|SOCK_DGRAM, 0);
   GSOCKET_bGetLocalAddress(&s_u32LocalAddress, "eth0");  //on target
   sprintf(s_cMyLocalAddress, "%d.%d.%d.%d", (U8)s_u32LocalAddress, (U8)(s_u32LocalAddress >> 8), (U8)(s_u32LocalAddress >> 16), (U8)(s_u32LocalAddress >> 24));
   printf("IP address is %s, socket %d\n", s_cMyLocalAddress, s_s32Socket);

   if ((s_u32LocalAddress > 0) && (s_s32Socket >= 0))
   {
      struct sockaddr_in servAddr, cliAddr;
      S32                s32Len = sizeof(cliAddr);
      char               cBuffer[MAXLINE];
      U32                u32Count = 0;

      memset(&servAddr, 0, sizeof(servAddr));
      memset(&cliAddr, 0, sizeof(cliAddr));

      //my info
      servAddr.sin_family = AF_INET;
      servAddr.sin_port = htons(PORT);
      servAddr.sin_addr.s_addr = INADDR_ANY;

      if (bind(s_s32Socket, (const struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
      {
         U_PrintLog("bind problem.");
         return;
      }

      for (;;)
      {
         recvfrom(s_s32Socket, (char *)cBuffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliAddr, &s32Len);
	 printf("received %s\n", cBuffer);

	 //do test based on buffer and get result
	 if ((u32Count % 2) == 0)
	 {
            sprintf(cBuffer, "FAIL");
	 }
	 else
	 {
            sprintf(cBuffer, "PASS");
	 }

	 sendto(s_s32Socket, (char *)cBuffer, strlen(cBuffer), MSG_CONFIRM, (const struct sockaddr *)&cliAddr, s32Len);
	 u32Count++;
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
         case SEMA_MBOX_INLINETEST:
            //we have mail
            while ((pMsg = (GENERIC_MESSAGE *)L_MBOX_Receive(M_INLINETEST)) != NULL)
            {
               switch (pMsg->Cmd)
               {
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
