#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fcid.h"
#include "lsvc.h"
#include "h_database.h"
#include "types.h"
#include "l_mbox.h"
#include "l_task.h"
#include "version.h"

PRIVATE void T_SYSTEM_vSetupMailBoxes(void)
{
   L_MBOX_DefineSema(M_PING, SEMA_MBOX_PING);
   L_MBOX_DefineSema(M_GMSG, SEMA_MBOX_GMSG);
   L_MBOX_DefineSema(M_SLPD, SEMA_MBOX_SLPD);
   L_MBOX_DefineSema(M_SUDPOUT, SEMA_MBOX_SUDPOUT);
   L_MBOX_DefineSema(M_NVRAM, SEMA_MBOX_NVRAM);
   L_MBOX_DefineSema(M_INLINETEST, SEMA_MBOX_INLINETEST);
   L_MBOX_DefineSema(M_SW_UPDATE, SEMA_MBOX_SW_UPDATE);
}

GLOBAL int T_SYSTEM_vMain(void)
{
   H_DATABASE_vInit();

   if (!H_DATABASE_bValidDBAtStartup())
   {
      H_DATABASE_bUpdateValueU32(SMDL, 119);     //model number - matches NET_NAME_ENUM_MULTIBEAM in t_ethernet.h in Pandora/Main
      H_DATABASE_bUpdateValueU32(SEDU, 190122);  //burn in date code
      H_DATABASE_bUpdateValueU32(SEIU, 1);       //post bunn-in line number
      H_DATABASE_bUpdateValueU32(SENU, 2468);    //sequential unt ID
   }

   H_DATABASE_bUpdateValueU32(SVER, VERSION_NUMBER_TIMES_1000);  //version number * 1000
//   printf("[%d]%s: SVER updated to %s\n", VERSION_NUMBER_TIMES_1000);
   T_SYSTEM_vSetupMailBoxes();
   L_TASK_Start((TASK)NVRAMTASK);
   L_TASK_Start((TASK)GMSGTASK);
   L_TASK_Start((TASK)GMSGSOCKETTASK);
   L_TASK_Start((TASK)SLPDTASK);
   L_TASK_Start((TASK)SUDPOUTTASK);
   L_TASK_Start((TASK)PINGTASK);
   L_TASK_Start((TASK)INLINETESTTASK);
   L_TASK_Start((TASK)SWUPDATETASK);
   return 0;
}
