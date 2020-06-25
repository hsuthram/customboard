#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "types.h"
#include "l_utils.h"
#include "l_task.h"

GLOBAL void U_PrintLog(char *str)
{
   time_t    t = time(NULL);
   struct tm tm = *localtime(&t);

//   printf("%02d:%02d:%02d(%s) - %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, get_task_name(u32TaskId), str);
   printf("%02d:%02d:%02d(%s) - %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, get_task_name(L_TASK_GetID(0)), str);
   fflush(stdout);
   return;
}
