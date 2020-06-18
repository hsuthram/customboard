#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "message.h"
#include "fcid.h"
#include "h_database.h"
#include "u_log.h"
#include "l_timer.h"
#include "l_task.h"
#include "lsvc.h"
#include "rtx2lsvc.h"

/********************              DEFINES                   ******************/
#define SETTINGS_SAVE_PERIOD (60 * 1000)  //every 600 seconds
/********************          LOCAL VARIABLES               ******************/
PRIVATE TIMER_INFO *s_pSaveDBTimer;
/********************              FUNCTIONS                 ******************/
GLOBAL void T_NVRAM_vMain(void)
{
   GENERIC_MESSAGE *pMessage;
   SEMA             s_wait_list[] = {SEMA_MBOX_NVRAM,
	                             SEMA_NVRAM_UPDATE,
	                             0};

   U_PrintLog("I'm alive!");

   //create the database save timer
   s_pSaveDBTimer = KS_alloc_timer();

   if (s_pSaveDBTimer)
   {
      KS_start_timer(s_pSaveDBTimer, SETTINGS_SAVE_PERIOD, SETTINGS_SAVE_PERIOD, SEMA_NVRAM_UPDATE);
   }
   else
   {
      U_PrintLog("timer allocation failed");
   }

   while (1)
   {
      switch (L_SEMA_WaitMultiple(s_wait_list))
      {
         case SEMA_NVRAM_UPDATE:
	    U_PrintLog("got SEMA_NVRAM_UPDATE");
	    H_DATABASE_vSaveToFile();
	    break;
	 case SEMA_MBOX_NVRAM:
	    //we have mail
	    while ((pMessage = (GENERIC_MESSAGE *)L_MBOX_Receive(M_NVRAM)) != NULL)
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
