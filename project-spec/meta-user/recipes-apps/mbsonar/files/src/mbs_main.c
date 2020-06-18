#include <stdio.h>
#include <stdlib.h>

#include "lsvc.h"
#include "types.h"
#include "l_mbox.h"
#include "l_sema.h"
#include "l_task.h"
#include "l_timer.h"
#include "l_mutex.h"

extern void T_SYSTEM_vMain(void);

GLOBAL int main(void)
{
   MUTEX_Init();
   SEMA_Init();
   TIMER_Init();
   MBOX_Init();
   QUEUE_Init();
   TASK_Init();
   T_SYSTEM_vMain();

   while (1);
   return(0);
}
