#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "types.h"
#include "message.h"
#include "fcid.h"
#include "h_database.h"
#include "u_log.h"
#include "l_timer.h"
#include "l_task.h"
#include "lsvc.h"
#include "rtx2lsvc.h"
#include "service.h"

/********************              DEFINES                   ******************/
#define MEGA_LIVE_ROOT_PATH  "/usr/app/"
#define MEGA_LIVE_UPDATE_DIR "mbupdate"
#define MEGA_LIVE_FILE       "mbupdate.mbz"
#define SW_UPDATE_PORT       SERVICE_PORT_SOCKET_SERVER_BASE  //50304
#define BUFF_LEN             1024
/********************         TYPE DEFINITIONS               ******************/
typedef struct file_info
{
   char cFileName[32];
   char cCheckSum[128];
   U32  u32FileSize;
} XFER_FILE_INFO;
/********************          LOCAL VARIABLES               ******************/
PRIVATE S32 s_s32SWUpdateState = 0;
PRIVATE S32 s_s32BytesReceived = 0;
PRIVATE S32 s_s32ServerSocket = -1;
PRIVATE S32 s_s32ConnectSocket = -1;
/********************              FUNCTIONS                 ******************/
/*******************************************************************************
 *
 *    Function: T_SW_UPDATE_vRecvUpdate
 *
 *    Args:     nothing
 *
 *    Return:   nothing
 *
 *    Purpose:  start server to receive packet
 *
 *    Notes:    transition is as follows:
 *              -2 = error
 *              -1 = server error 
 *               0 = no update
 *               1 = software ready on control head side
 *               2 = server has started, ready to receive
 *               4 = control head transfer done (after this we transition back to 0)
 *
 *******************************************************************************/
PRIVATE void T_SW_UPDATE_vRecvUpdate(S32 s32Socket)
{
   char  buff[BUFF_LEN];
   S32   s32Bytes = read(s32Socket, buff, BUFF_LEN);

   printf("[%d] socket %d, read %d bytes\n", __LINE__, s32Socket, s32Bytes);

   if (s32Bytes > 0)
   {
      XFER_FILE_INFO fileInfo;
      char           cFileName[64];
      FILE          *fp = NULL;

      memcpy(&fileInfo, buff, s32Bytes);
      printf("filename: %s\n", fileInfo.cFileName);
      printf("filesize: %d\n", fileInfo.u32FileSize);
      printf("checksum: %s\n", fileInfo.cCheckSum);
      sprintf(cFileName, "%s%s", MEGA_LIVE_ROOT_PATH, fileInfo.cFileName);
      printf("full filename: %s\n", cFileName);
      fp = fopen(cFileName, "w+");
      s_s32BytesReceived = 0;

      while (s_s32BytesReceived < fileInfo.u32FileSize)
      {
         s32Bytes = read(s32Socket, buff, BUFF_LEN);
//	 printf("Total received so far %d, this packet %d\n", s_s32BytesReceived, s32Bytes);

	 if (s32Bytes)
	 {
            if (fp)
	    {
               fwrite(buff, sizeof(char), s32Bytes, fp);	
	       s_s32BytesReceived += s32Bytes;
	    }
	 }
      }

      fclose(fp);

      //see if sha1sum matches
      {
         char cCmd[64];
	 char cCkSum[128];

         sprintf(cCmd, "/usr/bin/sha1sum %s", cFileName);
         fp = popen(cCmd, "re");

         if (fp)
         {
            fscanf(fp, "%[^ ]", cCkSum);

            if (strcmp(fileInfo.cCheckSum, cCkSum) == 0)
            {
               printf("[%d] sha1sum matches\n", __LINE__);
            }
            else
            {
               printf("[%d] sha1sum does not match\n", __LINE__);
            }
         }
         else
         {
	    printf("[%d] local sha1sum failed\n", __LINE__);
         }
      }
   }
   else
   {
      printf("[%d]%s; update YSWR to -2\n", __LINE__, __FUNCTION__);
//      H_DATABASE_bUpdateValueS32(MSWR, -2);
      H_DATABASE_bUpdateValueS32(YSWR, -2);
   }
}

/*******************************************************************************
 *
 *    Function: T_SW_UPDATE_vStopServer
 *
 *    Args:     nothing
 *
 *    Return:   nothing
 *
 *    Purpose:  terminate the server
 *
 *    Notes:    This will terminate both the server and the client sockets.
 *
 *******************************************************************************/
PRIVATE void T_SW_UPDATE_vStopServer(void)
{
   close(s_s32ConnectSocket);
   s_s32ConnectSocket = -1;
   close(s_s32ServerSocket);
   s_s32ServerSocket = -1;
   printf("[%d]%s; update YSWR to 0\n", __LINE__, __FUNCTION__);
   H_DATABASE_bUpdateValueS32(YSWR, 0);
//   H_DATABASE_bUpdateValueS32(MSWR, 0);
}

/*******************************************************************************
 *
 *    Function: T_SW_UPDATE_vStartServer
 *
 *    Args:     nothing
 *
 *    Return:   nothing
 *
 *    Purpose:  start the server
 *
 *    Notes:    
 *
 *******************************************************************************/
PRIVATE void T_SW_UPDATE_vStartServer(void)
{
   struct sockaddr_in servAddr;
   S32                slen = sizeof(servAddr);

   //create a socket
   if ((s_s32ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
   {
      U_PrintLog("Cannot create socket");
      printf("[%d]%s; update YSWR to -1\n", __LINE__, __FUNCTION__);
//      H_DATABASE_bUpdateValueS32(MSWR, -1);
      H_DATABASE_bUpdateValueS32(YSWR, -1);
   }
   else
   {
      printf("[%d] socket %d\n", __LINE__, s_s32ServerSocket);
      memset((char *)&servAddr, 0, sizeof(servAddr));
      servAddr.sin_family = AF_INET;
      servAddr.sin_port = htons(SW_UPDATE_PORT);
      servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

      if (bind(s_s32ServerSocket, &servAddr, sizeof(servAddr)) == -1)
      {
         U_PrintLog("Cannot bind socket");
         printf("[%d]%s; update YSWR to -1\n", __LINE__, __FUNCTION__);
//         H_DATABASE_bUpdateValueS32(MSWR, -1);
         H_DATABASE_bUpdateValueS32(YSWR, -1);
      }
      else
      {
         if (listen(s_s32ServerSocket, 5) < 0)
         {
            U_PrintLog("Cannot listen on socket");
            printf("[%d]%s; update YSWR to -1\n", __LINE__, __FUNCTION__);
//            H_DATABASE_bUpdateValueS32(MSWR, -1);
            H_DATABASE_bUpdateValueS32(YSWR, -1);
         }
      }
   }

   if (s_s32SWUpdateState > 0)
   {
      printf("[%d] listen socket %d\n", __LINE__, s_s32ServerSocket);
      printf("[%d]%s; update YSWR to 2\n", __LINE__, __FUNCTION__);
      H_DATABASE_bUpdateValueS32(YSWR, 2);
//      H_DATABASE_bUpdateValueS32(MSWR, 2);

      while (1)
      {
         s_s32ConnectSocket = accept(s_s32ServerSocket, (struct sockaddr*)&servAddr, (socklen_t *)&slen);
         printf("[%d]%s: s32ConnectSocket %d\n", __LINE__, __FUNCTION__, s_s32ConnectSocket);

         if (s_s32ConnectSocket > 0) 
         {
            T_SW_UPDATE_vRecvUpdate(s_s32ConnectSocket);
	    break;
         }
      
         sleep(1);
      }
   }
}

PRIVATE void T_SW_UPDATE_vVerifyAndDeploy(void)
{
   char cFileName[32];
   FILE *fp;

   sprintf(cFileName, "%s%s", MEGA_LIVE_ROOT_PATH, MEGA_LIVE_FILE);
   fp = fopen(cFileName, "r");

   if (fp)
   {
      char cCmd[64];

      printf("received %s\n", cFileName);
      fclose(fp);
      sprintf(cCmd, "tar xvf %s", cFileName);
      printf("execute %s\n", cCmd);
      system(cCmd);
      sprintf(cCmd, "%s/%s/PreInstall.sh", MEGA_LIVE_ROOT_PATH, MEGA_LIVE_UPDATE_DIR);
      printf("execute %s\n", cCmd);
      system(cCmd);
      printf("[%d]%s; update YSWR to 0\n", __LINE__, __FUNCTION__);
      H_DATABASE_bUpdateValueS32(YSWR, 0);
   }
}

PRIVATE void T_SW_UPDATE_vHandleNewData(MESSAGE_NEW_DATA *pMsg)
{
   printf("[%d] fcid = %c%c%c%c\n", __LINE__, 
          (U8)pMsg->FourCharId, (U8)(pMsg->FourCharId >> 8), (U8)(pMsg->FourCharId >> 16), (U8)(pMsg->FourCharId >> 24));

   switch (pMsg->FourCharId)
   {
      case MSWR:
         printf("[%d] current %d, new %d\n", __LINE__, s_s32SWUpdateState, pMsg->dbValue.s32Val);

	 //expecting data but we got an error notification
	 if (((s_s32SWUpdateState > 0) && (pMsg->dbValue.s32Val < 0)) ||
            (pMsg->dbValue.s32Val == 4))
	 {
            s_s32SWUpdateState  = pMsg->dbValue.s32Val;
            T_SW_UPDATE_vStopServer();
	 }
	 else if ((s_s32SWUpdateState == 0) && (pMsg->dbValue.s32Val == 1))  //new update available
	 {
            s_s32SWUpdateState  = pMsg->dbValue.s32Val;
//            T_SW_UPDATE_vStartServer();
            T_SW_UPDATE_vVerifyAndDeploy();
	 }
	 else
	 {
            s_s32SWUpdateState  = pMsg->dbValue.s32Val;
	 }

//         printf("[%d]%s: s_s32SWUpdateState = %d\n", __LINE__, __FUNCTION__, s_s32SWUpdateState);
         break;
      default:
	 break;
   }
}

PRIVATE void T_SW_UPDATE_vRegisterDatabaseItems(void)
{
   H_DATABASE_bRegisterForData(MSWR, M_SW_UPDATE);
}

GLOBAL void T_SW_UPDATE_vMain(void)
{
   GENERIC_MESSAGE *pMessage;
   SEMA             s_wait_list[] = {SEMA_MBOX_SW_UPDATE,
	                             0};

   U_PrintLog("I'm alive!");
   T_SW_UPDATE_vRegisterDatabaseItems();

   while (1)
   {
      switch (L_SEMA_WaitMultiple(s_wait_list))
      {
	 case SEMA_MBOX_SW_UPDATE:
	    //we have mail
	    while ((pMessage = (GENERIC_MESSAGE *)L_MBOX_Receive(M_SW_UPDATE)) != NULL)
	    {
               switch (pMessage->Cmd)
	       {
		  case CMD_ID_NEW_DATA:
                     U_PrintLog("got CMD_ID_NEW_DATA");
		     T_SW_UPDATE_vHandleNewData((MESSAGE_NEW_DATA *)pMessage);
		     break;
                  default:
		     break;
	       }
	    }

	    free(pMessage);
	    break;
	 default: 
	    printf("[%d]%s: got message id %d\n", __LINE__, __FUNCTION__, pMessage->Cmd);
	    break;
      }
   }

   return;
}
