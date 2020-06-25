#include <stdio.h>
#include <stdlib.h>

#include "gsocket.h"
#include "gmsg.h"
#include "service.h"
#include "types.h"
#include "message.h"
#include "lsvc.h"
#include "fcid.h"
#include "h_database.h"
#include "h_ping.h"
#include "u_log.h"
#include "s_sonar.h"
#include "l_timer.h"
#include "l_task.h"

/********************          LOCAL VARIABLES               ******************/
PRIVATE SERVICE_HANDLE s_hServiceHandle = SERVICE_HANDLE_INVALID;
/********************              FUNCTIONS                 ******************/
PRIVATE void T_SLPD_vRegisterDatabaseItems(void)
{
}

PRIVATE void T_SLPD_vRegisterService(void)
{
   if (s_hServiceHandle == SERVICE_HANDLE_INVALID)
   {
      s_hServiceHandle = SERVICE_hOpenServiceHandle();
   }

   if (s_hServiceHandle != SERVICE_HANDLE_INVALID)
   {
      U32           u32MyIPAddress;
      U64           u64MyMacAddress;
      char          cAttrStrClient[256];
      char          cAttributeString[1024];
      char          cSerial[16];
      U8            acMac[6];
      unsigned char cMyMacAddress[30];
      GMSG_ADDRESS  gmsgAddress;
      BOOLEAN       bReturn = 0;
      U32           u32Model, u32Version, u32Date, u32Line, u32Unit;

      GSOCKET_bGetMacAddress(acMac, "eth0");
//      sprintf(cMyMacAddress, "%02x:%02x:%02x:%02x:%02x:%02x", acMac[0], acMac[1], acMac[2], acMac[3], acMac[4], acMac[5]);
      sprintf(cMyMacAddress, "00:1a:29:%02x:%02x:%02x", acMac[3], acMac[4], acMac[5]);
      printf("[%d]: my MAC address is %s\n", __LINE__, cMyMacAddress);
//   GSOCKET_bGetLocalAddress(&u32MyIPAddress, "ens33");   //on vm
      GSOCKET_bGetLocalAddress(&u32MyIPAddress, "eth0");  //on target
      printf("my IP address is %d.%d.%d.%d\n", (U8)u32MyIPAddress, (U8)(u32MyIPAddress >> 8), (U8)(u32MyIPAddress >> 16), (U8)(u32MyIPAddress >> 24));
      GMSG_vMakeSocketAddress(&gmsgAddress, u32MyIPAddress, SERVICE_PORT_DEFAULT_BASE);  //client service
      
      //get model number
      H_DATABASE_bFetchU32(SMDL, &u32Model);

      //get version number
      H_DATABASE_bFetchU32(SVER, &u32Version);
//      printf("[%d]%s: SVER %d\n", __LINE__, __FUNCTION__, u32Version);

      //make the serial string
      H_DATABASE_bFetchU32(SEDU, &u32Date);
      H_DATABASE_bFetchU32(SEIU, &u32Line);
      H_DATABASE_bFetchU32(SENU, &u32Unit);
      sprintf(cSerial, "%05d%02d%04d", u32Date, u32Line, u32Unit);

      //create the client attribute list and register the client service
      sprintf(cAttrStrClient, "(unitname=zynq),(modelstring=MEGA Live Sonar),(unitmodel=70),(m2=%d),(macaddress=%s),(version=%d),(serialnum=%s)", u32Model, cMyMacAddress, u32Version, cSerial);
      bReturn = SERVICE_bRegister(s_hServiceHandle, "client", "base", &gmsgAddress, cAttrStrClient);
      printf("[%d]: client register returns %d\n", __LINE__, bReturn);

      //create the sonar attribute list and register the sonar service
      GMSG_vMakeSocketAddress(&gmsgAddress, u32MyIPAddress, SERVICE_PORT_HB_SONAR_BASE);  //sonar service
      sprintf(cAttributeString, "%s,(transducer=39),(transducer-beams=524288),(supported-beams=524288),(multicastaddress=239.%d.%d.%d)", 
              cAttrStrClient, SONAR_MB_MULTICAST_GROUP_OCTET, (U8)(u32MyIPAddress >> 16), (U8)(u32MyIPAddress >> 24));
      bReturn = SERVICE_bRegister(s_hServiceHandle, "sonarMBStream", "base", &gmsgAddress, cAttributeString);
      printf("[%d]: sonar register returns %d\n", __LINE__, bReturn);
   }
   else
   {
      printf("[%d]: slpd exit\n", __LINE__);
   }
}

PRIVATE void T_SLPD_vInit(void)
{
//   T_SLPD_vRegisterDatabaseItems();
   T_SLPD_vRegisterService();
}

GLOBAL void T_SLPD_vMain(void)
{
   GENERIC_MESSAGE *pMessage;
   SEMA             s_wait_list[] = {SEMA_MBOX_SLPD,
	                             0};

   U_PrintLog("I'm alive!");
   T_SLPD_vInit();

   while (1)
   {
      switch (L_SEMA_WaitMultiple(s_wait_list))
      {
	 case SEMA_MBOX_SLPD:
	    //we have mail
	    while ((pMessage = (GENERIC_MESSAGE *)L_MBOX_Receive(M_SLPD)) != NULL)
	    {
               switch (pMessage->Cmd)
	       {
		  case CMD_ID_NEW_DATA:
                     U_PrintLog("got CMD_ID_NEW_DATA");
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
