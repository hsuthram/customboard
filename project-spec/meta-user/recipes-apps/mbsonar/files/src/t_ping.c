#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gmsg.h"
#include "gmsg_info.h"
#include "datatype.h"
#include "types.h"
#include "message.h"
#include "lsvc.h"
#include "fcid.h"
#include "h_database.h"
#include "h_ping.h"
#include "rpmsg_utility.h"
#include "u_log.h"
#include "u_ping.h"
#include "u_gmsg.h"
#include "l_timer.h"
#include "l_task.h"

/********************              DEFINES                   ******************/
#define RPMSG_GET_KFIFO_SIZE       1
#define RPMSG_GET_AVAIL_DATA_SIZE  2
#define RPMSG_GET_FREE_SPACE       3
#define RPMSG_HEADER_LEN           16
#define MAX_RPMSG_BUFF_SIZE       (512 - RPMSG_HEADER_LEN)
#define PAYLOAD_MIN_SIZE	   1
#define PAYLOAD_MAX_SIZE	  (MAX_RPMSG_BUFF_SIZE - 24)
#define NUM_PAYLOADS		  (PAYLOAD_MAX_SIZE/PAYLOAD_MIN_SIZE)
#define RPMSG_BUS_SYS1            "/sys/bus/rpmsg"
/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
    U32 registerFlag;
    U32 pingArrayLimit;
    U32 u32SonarSource;
} sonarRequestDataType;

typedef struct
{
   FOURCHARID fcid;
   U32        value;
   U32        advise;
} controlChangeType;

typedef struct
{
   FOURCHARID fcidUIControl;
   FOURCHARID fcidSavedSetting;
} FCID_TRANSLATE_TYPE;

#pragma pack(1)
typedef struct
{
   U32   u32LowerFrequency;
   U32   u32UpperFrequency;
   float fFwdPingRange;
   float fDwnPingRange;
   U32   u32PingIndex;
   U32   u32PingOnOff;
} core2Send;

typedef struct
{
   core2Send headerPacket;
   U32       u32NumberOfSamples;
   U8        cData[100];
} core2Recv;

typedef struct
{
   U32 u32Num;
   U32 u32Size;
   U8  u8Data[];
} _payload;
#pragma pack()

PRIVATE FCID_TRANSLATE_TYPE s_fcidTranslateTable[] =
{
   {YBIW, MBIW},     // 1.  in/out water for x seconds
   {YBRD, SBRD},     // 2.  actual down ping range when in auto
   {YBRF, SBRF},     // 3.  actual forward ping range when in auto
   {YBMR, SBMR},     // 4.  actual mode as reported by gyroscope when in auto
   {YM1D, SM1D},     // 5.  mode 1 (full) depth range
   {YM1F, SM1F},     // 6.  mode 1 (full) forward range
   {YM2D, SM2D},     // 7.  mode 2 (down) depth range
   {YM2L, SM2L},     // 8.  chirp freq start
   {YM2U, SM2U},     // 9.  chirp freq end
   {YM3D, SM3D},     //10.  mode 3 (forward) depth range
   {YM3F, SM3F},     //11.  mode 3 (forward) forward range
   {YM4F, SM4F},     //12.  mode 4 (outlook) forward range
   {YMBN, SMBN},     //13.  ping enable on/off
   {YMLM, SMLM},     //14.  megalive user mode (auto/full/down/forward/outlook)
   {YSWR, MSWR},     //15.  software update state
};
/********************        FUNCTION PROTOTYPES             ******************/
PRIVATE void T_PING_vPrintCore2Header(void);
/********************          LOCAL VARIABLES               ******************/
PRIVATE TIMER_INFO *s_pWaterTimer;
PRIVATE U32         s_u32Count = 0;
PRIVATE U32         s_u32MulticastIpAddress = 0;
PRIVATE BOOLEAN     s_bInWater = FALSE;
PRIVATE BOOLEAN     s_bWetSwitchActive = FALSE;
PRIVATE BOOLEAN     s_bPingOn = TRUE;
PRIVATE float       s_fFwdPingRange = 3.0f;
PRIVATE float       s_fDwnPingRange = 3.0f;
PRIVATE U32         s_u32CurrentMode = 0;  //auto/full/down/forward/outlook
PRIVATE U32         s_u32LowerFrequency = 1;
PRIVATE U32         s_u32UpperFrequency = 2;
PRIVATE struct sonarRegisteredClientNode *sonarSourcesArray[TOTAL_SONAR_SOURCES];
PRIVATE S32         s_fd = -1;
PRIVATE BOOLEAN     s_bRPMSGInitialized = FALSE;
PRIVATE core2Send  *s_pPingHeaderToCore2;
PRIVATE _payload   *snd_Payload, *rcv_Payload;
/********************          GLOBAL VARIABLES              ******************/
DT_STRUCT_TYPE(GLOBAL, DT_SONAR_REQUEST_DATA, 1, NULL, NULL, sizeof(sonarRequestDataType), NULL,
               DT_UNSIGNED32,
               DT_UNSIGNED32,
               DT_UNSIGNED32);

DT_STRUCT_TYPE(GLOBAL, DT_CONTROL_CHANGE, 1, NULL, NULL, sizeof(controlChangeType), NULL,
               DT_UNSIGNED32,
               DT_UNSIGNED32,
               DT_UNSIGNED32);

DT_BASIC_TYPE(GLOBAL, DT_BEAM, G_DF_UNSIGNED8, G_DU_NONE);

DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_RAW_SONAR_DATA, DT_UNSIGNED8);

DT_EXCLUDED_TYPE(GLOBAL, DT_EXCLUDE_TVGD, sizeof(U8*), sizeof(U8*));

DT_STRUCT_TYPE(GLOBAL, DT_MB_PING_DESCRIPTOR, 1, NULL, NULL, sizeof(MB_PING_DESCRIPTOR), NULL,
	       DT_BEAM,
	       DT_RAW_SONAR_DATA);

PRIVATE BOOLEAN T_PING_bHandleNewDataRequest(MBOX Mailbox, GMSG_INFO *pMsgInfo, void *pvParam1, void *pvParam2);
PRIVATE BOOLEAN T_PING_bHandleControlChangeRecv(MBOX Mailbox, GMSG_INFO *pMsgInfo, void *pvParam1, void *pvParam2);

BEGIN_MESSAGE_MAP(s_PING_MsgMapInfo)
ON_MESSAGE(GMSG_ID_SONAR_REQUEST_DATA, T_PING_bHandleNewDataRequest, DT_SONAR_REQUEST_DATA, NULL, NULL) //ERZE: Handled by networked unit using local sonar
ON_MESSAGE(GMSG_ID_SONAR_CONTROL_CHANGE, T_PING_bHandleControlChangeRecv, DT_CONTROL_CHANGE, NULL, NULL) //ERZE: Handled by networked unit using network sonar
END_MESSAGE_MAP()
/********************              FUNCTIONS                 ******************/
//PRIVATE void T_DEVELOPMENT_vSendMsg(void)
//{
//   GENERIC_MESSAGE *pMsg;

//   pMsg = (GENERIC_MESSAGE *)malloc(sizeof(GENERIC_MESSAGE));
//   pMsg->Cmd = CMD_ID_RUN_SYSTEM;
//   L_MBOX_Send(M_GMSG, (MSGHEADER *)pMsg, (PRIORITY)NORMAL_PRIORITY, 0);
//   L_MBOX_Send(M_PROCESS, (MSGHEADER *)pMsg, (PRIORITY)NORMAL_PRIORITY, 0);
//}

PRIVATE void T_PING_vQueueInit(void)
{
   static MB_PING_DESCRIPTOR *QueueOfAvailableDescriptorPointers[QUEUE_ENTRIES];
   static MB_PING_DESCRIPTOR  PingDescriptor[QUEUE_ENTRIES];
   MB_PING_DESCRIPTOR        *pTemp;
   static U8                  u8RawBuffers[QUEUE_ENTRIES][COLS_8_dyn/2 * ROWS_8_dyn/2 + MAX_HFBYTES_T + MAX_HFBYTES_S];
   U32                        u32Q;

   //initialize the ping queue
   if (L_QUEUE_Define(Q_PING, sizeof(MB_PING_DESCRIPTOR *), QUEUE_ENTRIES, (U8 *)QueueOfAvailableDescriptorPointers, 0) != RC_SUCCESS)
   {
      U_PrintLog("could not define queue");
   }

   L_QUEUE_DefineSema(Q_PING, SEMA_PING_QNE, QNE);

   for (u32Q = 0; u32Q < QUEUE_ENTRIES; u32Q++)
   {
      PingDescriptor[u32Q].pu8RawSonarData = u8RawBuffers[u32Q]; 
      pTemp = &PingDescriptor[u32Q];

      if (L_QUEUE_Put(Q_PING, &pTemp) != RC_SUCCESS)
      {
         U_PrintLog("queue init failure");
         return;
      }
   }
}

PRIVATE void T_PING_vRegisterDatabaseItems(void)
{
   //pinging on/off not on last save
   H_DATABASE_bFetchBoolean(SMBN, &s_bPingOn);
   H_DATABASE_bRegisterForData(SMBN, M_PING);

   //last saved mode
   H_DATABASE_bFetchU32(SMLM, &s_u32CurrentMode);
   H_DATABASE_bRegisterForData(SMLM, M_PING);  //user selected mode
   H_DATABASE_bRegisterForData(YBMR, M_PING);  //real mode report by gyro when in auto

   if (s_u32CurrentMode == 0)  //auto
   {
      H_DATABASE_bFetchFloat(SBRF, &s_fFwdPingRange);
      H_DATABASE_bFetchFloat(SBRD, &s_fDwnPingRange);
   }
   else
   {
      //ping ranges on last shut down
      switch (s_u32CurrentMode)
      {
         case 1:  //full
            H_DATABASE_bFetchFloat(SM1F, &s_fFwdPingRange);
            H_DATABASE_bFetchFloat(SM1D, &s_fDwnPingRange);
	    break;
         default:
         case 2:  //down
            H_DATABASE_bFetchFloat(SM2D, &s_fFwdPingRange);
	    break;
         case 3:  //forward
            H_DATABASE_bFetchFloat(SM3F, &s_fFwdPingRange);
            H_DATABASE_bFetchFloat(SM3D, &s_fDwnPingRange);
	    break;
         case 4:  //outloook
            H_DATABASE_bFetchFloat(SM4F, &s_fFwdPingRange);
            break;
      }

      if (s_fFwdPingRange == 0.0f)  //mode is user selected but range is auto
      {
         H_DATABASE_bFetchFloat(SBRF, &s_fFwdPingRange);
      }

      if (s_fDwnPingRange == 0.0f)  //mode is user selected but range is auto
      {
         H_DATABASE_bFetchFloat(SBRD, &s_fDwnPingRange);
      }
   }

   H_DATABASE_bRegisterForData(SM1D, M_PING);  
   H_DATABASE_bRegisterForData(SM1F, M_PING);
   H_DATABASE_bRegisterForData(SM2D, M_PING);
   H_DATABASE_bRegisterForData(SM3D, M_PING);
   H_DATABASE_bRegisterForData(SM3F, M_PING);
   H_DATABASE_bRegisterForData(SM4F, M_PING);
   H_DATABASE_bRegisterForData(YBRD, M_PING);
   H_DATABASE_bRegisterForData(YBRF, M_PING);

   //fetch the upper and lower frequency
   //this is a const for now, cannot be changed by control head
   {
      float fFreq;

      //upper limit
      H_DATABASE_bFetchFloat(SM2L, &fFreq);
      s_u32LowerFrequency = (U32)(fFreq * 1000);

      //lower limit
      H_DATABASE_bFetchFloat(SM2U, &fFreq);
      s_u32UpperFrequency = (U32)(fFreq * 1000);
   }

   //wet switch active
   H_DATABASE_bRegisterForData(MBSW, M_PING);  

   //wet switch state change (after 3 seconds)
   H_DATABASE_bRegisterForData(YBIW, M_PING);  

   //software update state
//   H_DATABASE_bRegisterForData(MSWR, M_PING);  
   H_DATABASE_bRegisterForData(YSWR, M_PING);  
}

PRIVATE void T_PING_vSetMulticastIPAddress(void)
{
   U32 u32MyIPAddress;
   U8  u8Ip3, u8Ip4;

   GSOCKET_bGetLocalAddress(&u32MyIPAddress, "eth0");  //on target
   //printf("my IP address is (0x%x) %d.%d.%d.%d\n", u32MyIPAddress, (U8)u32MyIPAddress, (U8)(u32MyIPAddress >> 8), (U8)(u32MyIPAddress >> 16), (U8)(u32MyIPAddress >> 24));
   u8Ip3 = (U8)(u32MyIPAddress >> 16);
   u8Ip4 = (U8)(u32MyIPAddress >> 24);
   s_u32MulticastIpAddress = ((u8Ip4 << 24) & 0xFF000000) | ((u8Ip3 << 16) & 0x00FF0000) | ((SONAR_MB_MULTICAST_GROUP_OCTET << 8) & 0x0000FF00) | 239;
   printf("my multicast address is (0x%x) %d.%d.%d.%d\n", s_u32MulticastIpAddress, (U8)s_u32MulticastIpAddress, (U8)(s_u32MulticastIpAddress >> 8), (U8)(s_u32MulticastIpAddress >> 16), (U8)(s_u32MulticastIpAddress >> 24));
}

PRIVATE void T_PING_vSetupNetworkInterface(void)
{
   PRIVATE BOOLEAN bRegistered = 0;

   U_PrintLog("call U_GMSG_bRegisterMailbox");

   if (!bRegistered)
   {
      U_GMSG_bRegisterMailbox(M_PING, RES_PING, SEMA_GMSG_PING, s_PING_MsgMapInfo, SERVICE_PORT_HB_SONAR_BASE);
   }

   T_PING_vSetMulticastIPAddress();
}

PRIVATE void T_PING_vInitDatabaseItems(BOOLEAN bRestore)
{
   if (bRestore)
   {
      H_DATABASE_bUpdateValueFloat(SM2L, 1.200);  //lower limit
      H_DATABASE_bUpdateValueFloat(SM2U, 1.400);  //upper limit
      H_DATABASE_bUpdateValueU32(SMBN, 1);        //ping enabled
      H_DATABASE_bUpdateValueU32(SMLM, 0);        //default mode = auto
      H_DATABASE_bUpdateValueFloat(SM1D, 0.0f);   //default range of 0 means "auto" (full)
      H_DATABASE_bUpdateValueFloat(SM1F, 0.0f);   //default range of 0 means "auto" (full)
      H_DATABASE_bUpdateValueFloat(SM2D, 0.0f);   //default range of 0 means "auto" (down)
      H_DATABASE_bUpdateValueFloat(SM3D, 0.0f);   //default range of 0 means "auto" (forward)
      H_DATABASE_bUpdateValueFloat(SM3F, 0.0f);   //default range of 0 means "auto" (forward)
      H_DATABASE_bUpdateValueFloat(SM4F, 0.0f);   //default range of 0 means "auto" (outlook)
      KS_signal(SEMA_NVRAM_UPDATE);
   }

   H_DATABASE_bUpdateValueU32(MBSW, 0);        //not in water
   H_DATABASE_bUpdateValueU32(MBIW, 0);        //not in water for x seconds
   H_DATABASE_bUpdateValueFloat(SBRD, 35.0f);  //default down range of 35 meters
   H_DATABASE_bUpdateValueFloat(SBRF, 40.0f);  //default forward range of 40 meters
   H_DATABASE_bUpdateValueU32(SBMR, 1);        //default "real" mode is down
   H_DATABASE_bUpdateValueS32(MSWR, 0);        //no software update
   s_bInWater = FALSE;
   s_bWetSwitchActive = FALSE;
}

PRIVATE BOOLEAN T_PING_bInitRPMSG(void)
{
   struct rpmsg_endpoint_info eptinfo;
   char                       ept_dev_name[16];
   char                       ept_dev_path[32];
   S32                        s32Ret = 0;
   char                      *rpmsg_dev="virtio0.rpmsg-openamp-demo-channel.-1.0";
   char                       cPath[256];
   char                       rpmsg_char_name[16];
   int                        charfd = -1;

   /* Load rpmsg_char driver */
   U_PrintLog("load rpmsg_char");
   s32Ret = system("modprobe rpmsg_char");

   if (s32Ret < 0)
   {
      U_PrintLog("load rpmsg_char driver failed");
      return FALSE;
   }

   printf("open rpmsg dev %s\n", rpmsg_dev);
   sprintf(cPath, "%s/devices/%s", RPMSG_BUS_SYS1, rpmsg_dev);

   if (access(cPath, F_OK) != 0) 
   {
      printf("not able to access rpmsg device %s, %s\n", cPath, strerror(errno));
      return FALSE;
   }

   printf("bind to rpmsg dev %s\n", rpmsg_dev);
   s32Ret = bind_rpmsg_chrdev(rpmsg_dev);

   if (s32Ret < 0)
   {
      printf("unable to bind to rpmsg device %s\n", rpmsg_dev);
      return FALSE;
   }

   charfd = get_rpmsg_chrdev_fd(rpmsg_dev, rpmsg_char_name);

   if (charfd < 0)
   {
      printf("charfd = %d\n", charfd);
      return FALSE;
   }

   /* Create endpoint from rpmsg char driver */
   strcpy(eptinfo.name, "rpmsg-openamp-demo-channel");
   eptinfo.src = 0;
   eptinfo.dst = 0xFFFFFFFF;
   s32Ret = rpmsg_create_ept(charfd, &eptinfo);

   if (s32Ret != 0)
   {
      U_PrintLog("failed to create RPMSG endpoint");
      return FALSE;
   }

   if (!get_rpmsg_ept_dev_name(rpmsg_char_name, eptinfo.name, ept_dev_name))
   {
      U_PrintLog("failed to get RPMSG device name");
      return FALSE;
   }

   sprintf(ept_dev_path, "/dev/%s", ept_dev_name);
   s_fd = open(ept_dev_path, O_RDWR | O_NONBLOCK);

   if (s_fd < 0)
   {
      U_PrintLog("failed to open rpmsg device");
      close(charfd);
      return FALSE;
   }

   printf("[%d]%s: fd to rpmsg dev %d\n", __LINE__, __FUNCTION__, s_fd);
   return TRUE;
}

PRIVATE void T_PING_vInit(void)
{
   if (H_DATABASE_bValidDBAtStartup())
   {
      T_PING_vInitDatabaseItems(0);
   }
   else
   {
      T_PING_vInitDatabaseItems(1);
   }

   T_PING_vRegisterDatabaseItems();

   //populate the ping header that we will be sending to core2
   s_pPingHeaderToCore2 = (core2Send *)malloc(sizeof(core2Send));
   s_pPingHeaderToCore2->u32LowerFrequency = s_u32LowerFrequency;
   s_pPingHeaderToCore2->u32UpperFrequency = s_u32UpperFrequency;
   s_pPingHeaderToCore2->u32PingOnOff = (U32)s_bPingOn;
   s_pPingHeaderToCore2->fFwdPingRange = s_fFwdPingRange;
   s_pPingHeaderToCore2->fDwnPingRange = s_fDwnPingRange;
   s_pPingHeaderToCore2->u32PingOnOff = (U32)s_bPingOn;
   s_pPingHeaderToCore2->u32PingIndex = s_u32Count;  
//   T_PING_vPrintCore2Header();

   //intialize rpmsg
   s_bRPMSGInitialized = T_PING_bInitRPMSG();

   //initialize the ping queue
   T_PING_vQueueInit();
   
   //allocate the structures to and from core2
   snd_Payload = (_payload *)malloc(2 * (sizeof(U32) + 128));
   rcv_Payload = (_payload *)malloc(2 * (sizeof(U32) + 128));

   //allocate the timer for the water detection switch
   s_pWaterTimer = L_TIMER_Allocate();

   if (!s_pWaterTimer)
   {
      U_PrintLog("water switch timer allocation failed");
   }

   sleep(1);
   T_PING_vSetupNetworkInterface();
}

PRIVATE S32 T_PING_myPDSerializer(MB_PING_DESCRIPTOR *pd, void **ppvSerialized, int *pnSize)
{
   unsigned char *pusc = NULL;

   *pnSize = (int)(sizeof(MB_PING_DESCRIPTOR) + pd->u32NumberOfSamples);
   pusc = (unsigned char *)malloc((U32)(*pnSize));  //gets freed by SUDPOUTTASK
//   printf("[%d]%s: allocated %d bytes @ 0x%x\n", __LINE__, __FUNCTION__, *pnSize, pusc);
   *ppvSerialized = pusc;

   if (pusc == NULL)
   {
      printf("T_N_PING_myPDSerializer malloc failed!! %d bytes\n", *pnSize);
      return -1;
   }

   memcpy(pusc, pd, sizeof(MB_PING_DESCRIPTOR));
   memcpy(pusc + sizeof(MB_PING_DESCRIPTOR), pd->pu8RawSonarData, pd->u32NumberOfSamples);
   return 0;
}

PRIVATE BOOLEAN T_PING_bSendToNetworkDestinations(MB_PING_DESCRIPTOR *pPing)
{
   GMSG_ALT_TRANSPORT altTransport;
   void              *pvSerialized;
   int                nSize;
   GMSG_ADDRESS       multicastAddress;

   if (T_PING_myPDSerializer(pPing, &pvSerialized, &nSize) == -1)
   {
      return FALSE;
   }

//   printf("[%d]%s: serialized %d bytes\n", __LINE__, __FUNCTION__, nSize);
   altTransport.bRaw = TRUE;
   altTransport.u32MultiPacket = TRUE;
   altTransport.u32DataLength = (U32)nSize;
   multicastAddress.uAddr.Socket.u32Address = s_u32MulticastIpAddress;
   multicastAddress.uAddr.Socket.u32Port = SONAR_MB_MULTICAST_PORT;
//   printf("[%d]%s: port %d, address 0x%x\n", __LINE__, __FUNCTION__, multicastAddress.uAddr.Socket.u32Port, multicastAddress.uAddr.Socket.u32Address);

   if (U_GMSG_bSendMsgNoReplyAlt(M_PING, &multicastAddress, GMSG_ID_SONAR_RECEIVE_DATA, DT_MB_PING_DESCRIPTOR, (void *)pvSerialized, 0, &altTransport))
   {
//      U_PrintLog("send ping descriptor to M_PING");

      if (L_QUEUE_Put(Q_PING, &pPing) != RC_SUCCESS)
      {
         printf("[%d]%s: cannot go back to queue\n", __LINE__, __FUNCTION__);
      }

      return TRUE;
   }
   else
   {
      U_PrintLog("send ping descriptor to M_PING failed");
      printf("[%d]%s: call free\n", __LINE__, __FUNCTION__);
      free(pvSerialized);
      return FALSE;
   }
}

PRIVATE void T_PING_vPrintCore2Header(void)
{
   printf("Lower Frequency %d\n", s_pPingHeaderToCore2->u32LowerFrequency);
   printf("Upper Frequency %d\n", s_pPingHeaderToCore2->u32UpperFrequency);
   printf("Pinging         %s\n", s_pPingHeaderToCore2->u32PingOnOff ? "ON" : "OFF");
   printf("Forward Ping Range %f\n", s_pPingHeaderToCore2->fFwdPingRange);
   printf("Down Ping Range    %f\n", s_pPingHeaderToCore2->fDwnPingRange);
   printf("Ping Index      %d\n", s_pPingHeaderToCore2->u32PingIndex);
}

PRIVATE void T_PING_vPerformPing(void)
{
   MB_PING_DESCRIPTOR *apReturn[1];
   BOOLEAN             bPing = FALSE;

   apReturn[0] = NULL;
   L_QUEUE_GetWait(Q_PING, &apReturn[0]);
   apReturn[0]->Beam = 5;
   apReturn[0]->u32PingNumber = s_u32Count;
   KS_delay(SELFTASK, 100);

   if (sonarSourcesArray[4] && s_bPingOn)
   {
#if 1
      char       c_FileName[64];
      FILE      *fp;
      static U32 u32PingNum = 0;

      if (u32PingNum > 23)
      {
         U32 u32CurrentMode;

         u32PingNum = 0;
         H_DATABASE_bFetchU32(SBMR, &u32CurrentMode);
	 u32CurrentMode++;

	 if (u32CurrentMode > 4)
	 {
            u32CurrentMode = 1;
	 }

	 //this is for testing menus
//         H_DATABASE_bUpdateValueU32(YBMR, u32CurrentMode);
//	 printf("[%d]%s: SBMR to %d\n", __LINE__, __FUNCTION__, u32CurrentMode);
      }

      //read the header file
      sprintf(c_FileName, "/usr/app/simdata/ba0h%d.bin", u32PingNum);
//      printf("[%d]%s: %s\n", __LINE__, __FUNCTION__, c_FileName);
      fp = fopen(c_FileName, "rb");

      if (fp)
      {
	 S32 s32Data;

         fread(&s32Data, sizeof(S32), 1, fp);
         apReturn[0]->s32nhf_t = s32Data;
         fread(&s32Data, sizeof(S32), 1, fp);
         apReturn[0]->s32nhf_s = s32Data;
         fread(&s32Data, sizeof(S32), 1, fp);
         apReturn[0]->u32Seed = (U32)s32Data;
	 fclose(fp);
//	 printf("%d %d %d\n", apReturn[0]->s32nhf_t, apReturn[0]->s32nhf_s, apReturn[0]->u32Seed);

	 //now read the ping data file
         sprintf(c_FileName, "/usr/app/simdata/ba0d%d.bin", u32PingNum);
//         printf("[%d]%s: %s\n", __LINE__, __FUNCTION__, c_FileName);
         fp = fopen(c_FileName, "rb");

	 if (fp)
         {
            struct stat filestat;
            int         iElements;

            stat(c_FileName, &filestat);
//	    memset(apReturn[0]->pu8RawSonarData, 0, COLS_8_dyn/2 * ROWS_8_dyn/2 + MAX_HFBYTES_T + MAX_HFBYTES_S);
            iElements = (int)fread(apReturn[0]->pu8RawSonarData, sizeof(U8), filestat.st_size, fp);
            apReturn[0]->u32NumberOfSamples = iElements;
	    fclose(fp);
	    //printf("file size %ld bytes, read %d elements\n", filestat.st_size, iElements);
//	    printf("ping #%d, %d elements\n", u32PingNum, iElements);
	    bPing = TRUE;
	 }
	 else
	 {
            apReturn[0]->u32NumberOfSamples = 0;
	 }

	 s_u32Count = u32PingNum;
	 u32PingNum++;
      }
#else
      //perform real ping here
      U8 *pusc = snd_Payload->u8Data;
      S32 s32Bytes;

      T_PING_vPrintCore2Header();
      memcpy(pusc, s_pPingHeaderToCore2, 24);

 #if DEBUG
      for (int i = 0; i < 24; i++)
      {
         printf("%02X ", snd_Payload->u8Data[i]); 
      }
 
      printf("\n");
 #endif
      snd_Payload->u32Num = 1;
      snd_Payload->u32Size = 112;

      //send to fabric
      s32Bytes = write(s_fd, snd_Payload, 2 * sizeof(U32) + snd_Payload->u32Size);
      printf("[%d]%s: sent %d bytes\n", __LINE__, __FUNCTION__, s32Bytes);

      //recv data back
      rcv_Payload->u32Num = 0;
      s32Bytes = read(s_fd, rcv_Payload, 128);

      while (s32Bytes <= 0)
      {
         usleep(10000);
         s32Bytes = read(s_fd, rcv_Payload, 2 * sizeof(U32) + PAYLOAD_MAX_SIZE);
      }

      printf("[%d]%s: received payload %d %d bytes\n", __LINE__, __FUNCTION__, rcv_Payload->u32Num, s32Bytes);

 #if DEBUG
      for (int i = 0; i < s32Bytes; i++)
      {
         printf("%02X ", rcv_Payload->u8Data[i]);

	 if (((i + 1) % 32) == 0)
	 {
            printf("\n");
	 }
      }
 #endif

      printf("\n");
      s32Bytes = read(s_fd, rcv_Payload, 2 * sizeof(U32) + PAYLOAD_MAX_SIZE);
      s_u32Count++;
      s_pPingHeaderToCore2->u32PingIndex = s_u32Count;  
      bPing = TRUE;
#endif

      if (bPing)  //we managed to do a real ping 
      {
         if (!T_PING_bSendToNetworkDestinations(apReturn[0]))
         {
            bPing = FALSE;  //but sending the ping to the network failed;
         }
      }
   }

   //put the descriptor back into the queue when:
   //1) we can't do a real ping
   //2) if the ping that we tried to send to the network fails
   if (!bPing)
   {
      sleep(1);

      if (L_QUEUE_Put(Q_PING, &apReturn[0]) != RC_SUCCESS)
      {
         printf("[%d]%s: cannot go back to queue\n", __LINE__, __FUNCTION__);
      }
   }
}

/*******************************************************************************
 *
 *    Function: T_PING_fcidTranslateRemoteFcid
 *
 *    Args:     args
 *
 *    Return:   -none-
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE FOURCHARID T_PING_fcidTranslateRemoteFcid(FOURCHARID fcidRemote)
{
   int i;

   for (i = 0; i < (int)ARRAY_LENGTH(s_fcidTranslateTable); i++)
   {
       if (s_fcidTranslateTable[i].fcidUIControl == fcidRemote)
       {
          return s_fcidTranslateTable[i].fcidSavedSetting;
       }
       else if (s_fcidTranslateTable[i].fcidSavedSetting == fcidRemote)
       {
          return s_fcidTranslateTable[i].fcidUIControl;
       }
   }

   return fcidRemote;
}

PRIVATE void T_PING_vAdjustRange(void)
{
   U32 u32Mode;

   H_DATABASE_bFetchU32(SMLM, &s_u32CurrentMode);

   if (s_u32CurrentMode == 0)
   {
      H_DATABASE_bFetchU32(SBMR, &u32Mode);
   }
   else
   {
      u32Mode = s_u32CurrentMode;
   }

   switch (u32Mode)
   {
      case 1:  //full
         H_DATABASE_bFetchFloat(SM1F, &s_fFwdPingRange);
         H_DATABASE_bFetchFloat(SM1D, &s_fDwnPingRange);
         break;
      case 2:  //down
         H_DATABASE_bFetchFloat(SM2D, &s_fDwnPingRange);
         break;
      case 3:  //forward
         H_DATABASE_bFetchFloat(SM3F, &s_fFwdPingRange);
         H_DATABASE_bFetchFloat(SM3D, &s_fDwnPingRange);
	 break;
      case 4:  //outloook
         H_DATABASE_bFetchFloat(SM4F, &s_fFwdPingRange);
	 break;
      default:
	 break;
   }

   if (s_fFwdPingRange == 0.0f)  //range is auto
   {
      //we should let the unit decide range for for now we'll fix it at a number
      s_fFwdPingRange = 35.0;
   }

   if (s_fDwnPingRange == 0.0f)  //range is auto
   {
      //we should let the unit decide range for for now we'll fix it at a number
      s_fDwnPingRange = 40.0f;
   }

   printf("[%d]%s: mode %d, forward range %f, down range %f\n", __LINE__, __FUNCTION__, s_u32CurrentMode, s_fFwdPingRange, s_fDwnPingRange);
   H_DATABASE_bUpdateValueFloat(YBRD, s_fDwnPingRange);  
   H_DATABASE_bUpdateValueFloat(YBRF, s_fFwdPingRange);  
}

PRIVATE void T_PING_vHandleNewData(MESSAGE_NEW_DATA *pMsg)
{
   switch (pMsg->FourCharId)
   {
      case SMBN:
	 s_bPingOn = (BOOLEAN)pMsg->dbValue.u32Val;
	 printf("[%d]%s: SMBN = %d\n", __LINE__, __FUNCTION__, s_bPingOn);
	 break;
      case SMLM:
      case SM1D:
      case SM1F:
      case SM2D:
      case SM3D:
      case SM3F:
      case SM4F:
	 T_PING_vAdjustRange();
	 break;
      case YBIW:
	 s_bInWater = (BOOLEAN)pMsg->dbValue.u32Val;
	 printf("[%d]%s: YBIW = %d\n", __LINE__, __FUNCTION__, s_bInWater);
	 break;
      case MBSW:
	 s_bWetSwitchActive = (BOOLEAN)pMsg->dbValue.u32Val;
	 printf("[%d]%s: s_bWetSwitchActive %d\n", __LINE__, __FUNCTION__, s_bWetSwitchActive);

	 if (s_bWetSwitchActive == s_bInWater)
	 {
            U_PrintLog("stop wet switch timer");
	    KS_stop_timer(s_pWaterTimer);
	 }
	 else
	 {
            if (KS_inqtimer(s_pWaterTimer) > 0)
	    {
               U_PrintLog("restart wet switch timer");
	       KS_restart_timer(s_pWaterTimer, 3000, 0);
	    }
	    else
	    {
               U_PrintLog("start wet switch timer");
	       KS_start_timer(s_pWaterTimer, 3000, 0, SEMA_TIMER_MB_WET_STATE);
	    }
	 }
	 break;
      default:
	 break;
   }

   //save the value to the "other" database item
   {
      FOURCHARID fcidOther = T_PING_fcidTranslateRemoteFcid(pMsg->FourCharId);

      if (fcidOther != pMsg->FourCharId)
      {
         DB_DATA_TYPE dbData;

         dbData.u32Val = pMsg->dbValue.u32Val;
         H_DATABASE_bUpdateValue(fcidOther, dbData, 0);
      }
   }
}

PRIVATE void T_PING_bSynchronizeNewClientControls(GMSG_ADDRESS *address)
{
   int i;

   for (i = 0; i < (int)ARRAY_LENGTH(s_fcidTranslateTable) ; i++)
   {
      FOURCHARID fcid = s_fcidTranslateTable[i].fcidSavedSetting;
      U32        u32Value;

      printf("%x(%c%c%c%c)\n", fcid, (U8)fcid, (U8)(fcid >> 8), (U8)(fcid >> 16), (U8)(fcid >> 24));

      if (H_DATABASE_bFetchU32(fcid, &u32Value))
      {
         controlChangeType controlChange;
	  
#if DEBUG 
//	 if ((fcid == SMRF) || (fcid == SMRD) || (fcid == SM2U) || (fcid == SM2L))
	 if ((fcid == SM2U) || 
             (fcid == SM2L) || 
	     (fcid == SM0D) ||
	     (fcid == SM0F) ||
	     (fcid == SM1D) ||
	     (fcid == SM1F) ||
	     (fcid == SM2D) ||
	     (fcid == SM3D) ||
	     (fcid == SM3F) ||
	     (fcid == SM4F))
	 {
            float fVal;

	    memcpy(&fVal, &u32Value, sizeof(float));
            printf("[%d]%s: value 0x%x (%f)\n", __LINE__, __FUNCTION__, u32Value, fVal);
	 }
	 else
	 {
            printf("[%d]%s: value 0x%x (%d)\n", __LINE__, __FUNCTION__, u32Value, u32Value);
	 }
#endif

	 controlChange.fcid = fcid;
         controlChange.value = u32Value;
	 controlChange.advise = 1;
	 U_GMSG_bSendMsgNoWait(M_PING, address, GMSG_ID_SONAR_CONTROL_CHANGE, DT_CONTROL_CHANGE, (void *)&controlChange, 0, SONAR_MSG_TIMEOUT);
      }
   }

#if 1
   //turn water switch on when we get the first client - this is just for testing
   H_DATABASE_bUpdateValueU32(MBSW, 1);    //not in water
#endif
}

PRIVATE BOOLEAN T_PING_bHandleNewDataRequest(MBOX Mailbox, GMSG_INFO *pMsgInfo, void *pvParam1, void *pvParam2)
{
   sonarRequestDataType *request = GMSG_pGetMessageBody(sonarRequestDataType, pMsgInfo);

   printf("[%d]%s: u32MsgId 0x%x\n", __LINE__, __FUNCTION__, pMsgInfo->pMsgHeader->u32MsgId);
   printf("[%d]%s: Src Address 0x%x\n", __LINE__, __FUNCTION__, pMsgInfo->pMsgHeader->SrcAddress.uAddr.Socket.u32Address);  
   printf("[%d]%s: request->registerFlag  %d\n", __LINE__, __FUNCTION__, request->registerFlag);
   printf("[%d]%s: request->pingArrayLimit 0x%x\n", __LINE__, __FUNCTION__, request->pingArrayLimit);
   printf("[%d]%s: request->0xu32SonarSource 0x%x\n", __LINE__, __FUNCTION__, request->u32SonarSource);

   if (request->registerFlag)
   {
      U_PrintLog("want to register for sonar data");
      T_PING_bSynchronizeNewClientControls(&pMsgInfo->pMsgHeader->SrcAddress);
      U_PING_vAddClientNode(&(sonarSourcesArray[request->u32SonarSource]), pMsgInfo->pMsgHeader->SrcAddress.uAddr.Socket.u32Address);
   }
   else
   {
      U_PrintLog("want to un-register for sonar data");

      if (sonarSourcesArray[request->u32SonarSource])
      {
         U_PING_vDeleteClientNode(&(sonarSourcesArray[request->u32SonarSource]), pMsgInfo->pMsgHeader->SrcAddress.uAddr.Socket.u32Address);
         printf("[%d]%s: delete source %d node\n", __LINE__, __FUNCTION__, request->u32SonarSource);
//         T_N_PING_vHandleClients(request->u32SonarSource);
      }
   }
}

PRIVATE BOOLEAN T_PING_bIsOurClient(GMSG_ADDRESS *pAddress)
{
   if (sonarSourcesArray[4])
   {
      struct sonarRegisteredClientNode *tmp = sonarSourcesArray[4];

      while (tmp)
      {
         if (tmp->u32ClientAddress == pAddress->uAddr.Socket.u32Address)
         {
            return TRUE;
          }
          else
          {
             tmp = tmp->next;
          }
      }
   }

   return FALSE;
}

PRIVATE void T_PING_vUpdateClients(FOURCHARID fcid, GMSG_ADDRESS *pOriginAddress)
{
   printf("fcid %x(%c%c%c%c)\n", fcid, (U8)fcid, (U8)(fcid >> 8), (U8)(fcid >> 16), (U8)(fcid >> 24));

   if (T_PING_fcidTranslateRemoteFcid(fcid) == fcid)
   {
      printf("[%d]%s: this does not need to get passed to clients\n", __LINE__, __FUNCTION__);
      return;
   }

   if (sonarSourcesArray[4])  //this means we have clients
   {
      U32 u32Value;

      if (H_DATABASE_bFetchU32(fcid, &u32Value))
      {
         struct sonarRegisteredClientNode *tmp = sonarSourcesArray[4];
         controlChangeType                 controlChange;

         controlChange.fcid = fcid;
         controlChange.value = u32Value;
         controlChange.advise = 1;

         while (tmp)
	 {
            if ((pOriginAddress == NULL) || //update everyone
                (pOriginAddress->uAddr.Socket.u32Address != tmp->u32ClientAddress))  //don't update where it came from
            {
               GMSG_ADDRESS destAddr;

	       GMSG_vMakeSocketAddress(&destAddr, tmp->u32ClientAddress, SERVICE_PORT_HB_SONAR_BASE);
               U_GMSG_bSendMsgNoWait(M_PING, &destAddr, GMSG_ID_SONAR_CONTROL_CHANGE, DT_CONTROL_CHANGE, (void *)&controlChange, 0, SONAR_MSG_TIMEOUT);
	    }

            tmp = tmp->next;
         }
      }
   }
}

PRIVATE void T_PING_vUpdateDatabaseItem(controlChangeType *controlChange)
{
   FOURCHARID  fcidLocal = T_PING_fcidTranslateRemoteFcid(controlChange->fcid);
   DB_DATA_TYPE dbData;
   MESSAGE_NEW_DATA NewDataMsg;

   printf("[%d]: received fcid (%c%c%c%c)\n", __LINE__, (U8)controlChange->fcid, (U8)(controlChange->fcid >> 8), (U8)(controlChange->fcid >> 16), (U8)(controlChange->fcid >> 24));
   printf("[%d]: local fcid (%c%c%c%c)\n", __LINE__, (U8)fcidLocal, (U8)(fcidLocal >> 8), (U8)(fcidLocal >> 16), (U8)(fcidLocal >> 24));
   dbData.u32Val = controlChange->value;
//   H_DATABASE_bUpdateValue(fcidLocal, dbData, 0);
   H_DATABASE_bUpdateValueU32Except(fcidLocal, dbData.u32Val, M_PING);
   
   //handle local update
   memset(&NewDataMsg, 0, sizeof(NewDataMsg));
   NewDataMsg.FourCharId = fcidLocal;
   NewDataMsg.dbValue.u32Val = controlChange->value;
   T_PING_vHandleNewData(&NewDataMsg);
}

PRIVATE FOURCHARID T_PING_fcidTranslateRemoteFCID(FOURCHARID fcidRemote)
{
   int i;

   for (i = 0; i < (int)ARRAY_LENGTH(s_fcidTranslateTable); i++)
   {
      if (s_fcidTranslateTable[i].fcidUIControl == fcidRemote)
      {
         return s_fcidTranslateTable[i].fcidSavedSetting;
      }
      else if (s_fcidTranslateTable[i].fcidSavedSetting == fcidRemote)
      {
         return s_fcidTranslateTable[i].fcidUIControl;
      }
   }

   return fcidRemote;
}

#if 0
PRIVATE BOOLEAN T_PING_bControlChangeSend(FOURCHARID fcid, U32 u32Value, U32 advise, GMSG_ADDRESS *pOriginAddress)
{
   controlChangeType controlChange; 
//   MESSAGE_NEW_DATA  newDataMsg;
   FOURCHARID        fcidOther;
   
   KS_lockw(RES_SONAR_SHARE); //I dont' know if we really need this
   fcidOther = T_PING_fcidTranslateRemoteFCID(fcid);
   printf("fcidOther %x(%c%c%c%c)\n", fcidOther, (U8)fcidOther, (U8)(fcidOther >> 8), (U8)(fcidOther >> 16), (U8)(fcidOther >> 24));

   //we may need to save the db item and notify other tasks but we'll skip that right now

   //if this is set we don't forward it to any clients
   //I don't really know what this is used for
   if (advise)
   {
      KS_unlock(RES_SONAR_SHARE);
      return;
   }

   //we need to send it to other clients
   if (sonarSourcesArray[4])
   {
      struct sonarRegisteredClientNode *tmp = sonarSourcesArray[4];

      while (tmp)
      {
	 //if we received this from a client, don't forward it to the one that send it to us
         if (tmp->u32ClientAddress != pOriginAddress->uAddr.Socket.u32Address)
         {
            controlChange.fcid = fcid;
            controlChange.value = u32Value;
	    controlChange.advise = 1;
	    U_GMSG_bSendMsgNoWait(M_PING, address, GMSG_ID_SONAR_CONTROL_CHANGE, DT_CONTROL_CHANGE, (void *)&controlChange, 0, SONAR_MSG_TIMEOUT);
	 }

         tmp = tmp->next;
     }
  }

   KS_unlock(RES_SONAR_SHARE);
}
#endif

PRIVATE BOOLEAN T_PING_bHandleControlChangeRecv(MBOX Mailbox, GMSG_INFO *pMsgInfo, void *pvParam1, void *pvParam2)
{
   controlChangeType *controlChange = GMSG_pGetMessageBody(controlChangeType, pMsgInfo);

   printf("[%d]%s: u32MsgId 0x%x\n", __LINE__, __FUNCTION__, pMsgInfo->pMsgHeader->u32MsgId);
   printf("[%d]%s: Src Address 0x%x\n", __LINE__, __FUNCTION__, pMsgInfo->pMsgHeader->SrcAddress.uAddr.Socket.u32Address);  
   printf("received %x(%c%c%c%c)\n", controlChange->fcid, (U8)controlChange->fcid, (U8)(controlChange->fcid >> 8), (U8)(controlChange->fcid >> 16), (U8)(controlChange->fcid >> 24));

   if (T_PING_bIsOurClient(&pMsgInfo->pMsgHeader->SrcAddress))
   {
      //save the local
      T_PING_vUpdateDatabaseItem(controlChange);

      //and update all other subscribers
      T_PING_vUpdateClients(T_PING_fcidTranslateRemoteFcid(controlChange->fcid), &pMsgInfo->pMsgHeader->SrcAddress);
   }
}

PRIVATE void T_PING_vHandlePeerDisconnect(MSG_GMSG_DISCONNECT *pMsg)
{
   if (sonarSourcesArray[4])
   {
      U_PING_vDeleteClientNode(&sonarSourcesArray[4], pMsg->Address.uAddr.Socket.u32Address);
   }
}

GLOBAL void T_PING_vMain(void)
{
   GENERIC_MESSAGE *pMessage;
   SEMA             s_wait_list[] = {SEMA_MBOX_PING,
                                     SEMA_TIMER_MB_WET_STATE,
				     SEMA_PING_QNE,
	                             0};

   U_PrintLog("I'm alive!");
   T_PING_vInit();

   while (1)
   {
      switch (L_SEMA_WaitMultiple(s_wait_list))
      {
         case SEMA_PING_QNE:
//            U_PrintLog("got SEMA_PING_QNE");
	    T_PING_vPerformPing();
	    break;
         case SEMA_TIMER_MB_WET_STATE:
            U_PrintLog("got SEMA_TIMER_MB_WET_STATE");
            H_DATABASE_bUpdateValueU32(YBIW, s_bWetSwitchActive);
	    break;
	 case SEMA_MBOX_PING:
            //we have mail
            while ((pMessage = (GENERIC_MESSAGE *)L_MBOX_Receive(M_PING)) != NULL)
            {
               switch (pMessage->Cmd)
               {
                  case CMD_ID_RUN_SYSTEM:
                     U_PrintLog("got CMD_ID_RUN_SYSTEM");
                     break;
		  case CMD_ID_GMSG_RECEIVE:
                     U_PrintLog("got CMD_ID_GMSG_RECEIVE");
                     U_GMSG_bHandleReceive((MSG_GMSG_RECEIVE *)pMessage);
		     break;
		  case CMD_ID_NETWORK_REGISTER_4DATA:
                     U_PrintLog("got CMD_ID_NETWORK_REGISTER_4DATA");
                     break;
                  case CMD_ID_NEW_DATA:
                     U_PrintLog("got CMD_ID_NEW_DATA");
                     T_PING_vHandleNewData((MESSAGE_NEW_DATA *)pMessage);
                     T_PING_vUpdateClients(T_PING_fcidTranslateRemoteFcid(((MESSAGE_NEW_DATA *)pMessage)->FourCharId), 0);
                     break;
		  case CMD_ID_GMSG_DISCONNECT:
		     T_PING_vHandlePeerDisconnect((MSG_GMSG_DISCONNECT *)pMessage);
		     break;
                  default:
		     printf("[%d]%s: got message id %d\n", __LINE__, __FUNCTION__, pMessage->Cmd);
                     break;
               }
            }

            free(pMessage);
            break;
	 default:
	    break;
      }
   }
}
