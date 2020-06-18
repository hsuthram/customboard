/*******************************************************************************
*        Copyright (c) 2009 - 2010 Johnson Outdoors Marine Electronics, Inc.   *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description:

      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      + This software is the proprietary property of:                   +
      +                                                                 +
      +  Johnson Outdoors Marine Electronics, Inc.                      +
      +                                                                 +
      +  1220 Old Alpharetta Rd           1 Humminbird Lane             +
      +  Suite 340                        Eufaula, AL  36027            +
      +  Alpharetta, GA  30005                                          +
      +                                                                 +
      +        Use, duplication, or disclosure of this material,        +
      +          in any form, is forbidden without permission           +
      +         from Johnson Outdoors Marine Electronics, Inc.          +
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

*******************************************************************************/

/********************           COMPILE FLAGS                ******************/

/********************           INCLUDE FILES                ******************/
#include <stdlib.h>
#include <stdio.h>
#include <features.h>
#include <time.h>
#define _TIME_T
#include <pthread.h>

#include "types.h"
#include "lsvc.h"
#include "l_clock.h"
#include "l_task.h"
#include "l_mutex.h"
#include "l_utils.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
typedef struct
{
   TASK              OwningTaskID;     // who owns this mutex
   U32               NestLevel;        // how many lock-nests (monsters?) deep
   pthread_mutex_t   MutexStruct;      // the OS structure
} MUTEX_INFO;
/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/

/********************        FUNCTION PROTOTYPES             ******************/

/********************           LOCAL VARIABLES              ******************/
PRIVATE MUTEX_INFO *ptrMutexArray = NULL;
PRIVATE MUTEX_INFO *endMutexArray;
/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/

/****************************************************************
 * Function    MUTEX_Init
 * Args:       none
 * Return:     none
 * Purpose:    This function will allocate and initialize the
 *             mutex structure array.
 ****************************************************************/
GLOBAL void MUTEX_Init(void)
{
   MUTEX_INFO *miptr;

   ptrMutexArray = calloc(nres, sizeof(MUTEX_INFO));
   if (ptrMutexArray == NULL)             // unlikely, but check anyway
      return;

   endMutexArray = &ptrMutexArray[nres];

   // now, spin through them all initializing the mutexes
   for (miptr = ptrMutexArray; miptr != endMutexArray; miptr++)
      pthread_mutex_init(&miptr->MutexStruct, NULL);
}


/****************************************************************
 * Function    MUTEX_GetPtr
 * Args:       1: MUTEX mutex number
 * Return:     pointer to mutex info structure (or NULL if invalid)
 * Purpose:    This function will check the mutex value.
 ****************************************************************/
PRIVATE MUTEX_INFO *MUTEX_GetPtr(MUTEX sMutex)
{
   // less than one OR too high a number gets an error here
   if (sMutex < 1 || sMutex > nres)
      return(NULL);

   // make sure the array is initialized
   if (ptrMutexArray == NULL)
      return(NULL);

   return(&ptrMutexArray[sMutex - 1]);
}


/****************************************************************
 * Function    MUTEX_DoLock
 * Args:       1: MUTEX Mutex ID 1-n
 *             2: waitFlag: 0 = return immediately if busy, 1 = wait
 * Return:     ERC RC_MUTEX_INVALID if invalid mutex number OR
 *                 RC_MUTEX_BUSY if locked by another TASK already OR
 *                 RC_MUTEX_NESTED if already locked by this task OR
 *                 RC_SUCCESS
 * Purpose:    Lock the mutex; wait if already locked by another task.
 * Note:       use L_MUTEX_Lock() and L_MUTEX_LockWait(), not this
 ****************************************************************/
PRIVATE ERC MUTEX_DoLock(MUTEX sMutexID, U32 waitFlag)
{
   TASK ourTask;
   MUTEX_INFO *miptr;

   miptr = MUTEX_GetPtr(sMutexID);              // get the mutex structure
   if (miptr == NULL)
      return(RC_MUTEX_INVALID);

   ourTask = L_TASK_GetID(0);
   if (miptr->OwningTaskID != ourTask)          // if not ours yet,
   {
      if (waitFlag == 0)                        // if not waiting, just try
      {
         if (pthread_mutex_trylock(&miptr->MutexStruct) != 0)
         {
            return(RC_MUTEX_BUSY);              // could not acquire the lock
         }
      }
      else                                      // wait for acquire lock
      {
         pthread_mutex_lock(&miptr->MutexStruct);
      }

      miptr->OwningTaskID = ourTask;            // we own it now
   }

   miptr->NestLevel++;                          // another nest
   if (miptr->NestLevel != 1)                   // if not the first time,
      return(RC_MUTEX_NESTED);                  // return nested status

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_MUTEX_Lock
 * Args:       1: MUTEX Mutex ID 1-n
 * Return:     ERC RC_MUTEX_INVALID if invalid mutex number OR
 *                 RC_MUTEX_BUSY if locked by another TASK already OR
 *                 RC_MUTEX_NESTED if already locked by this task OR
 *                 RC_SUCCESS
 * Purpose:    Try to lock the mutex.
 ****************************************************************/
GLOBAL ERC L_MUTEX_Lock(MUTEX sMutexID)
{
   return(MUTEX_DoLock(sMutexID, 0));
}


/****************************************************************
 * Function    L_MUTEX_LockWait
 * Args:       1: MUTEX Mutex ID 1-n
 * Return:     ERC RC_MUTEX_INVALID if invalid mutex number OR
 *                 RC_MUTEX_NESTED if already locked by this task OR
 *                 RC_SUCCESS
 * Purpose:    Lock the mutex; wait if already locked by another task.
 ****************************************************************/
GLOBAL ERC L_MUTEX_LockWait(MUTEX sMutexID)
{
   return(MUTEX_DoLock(sMutexID, 1));
}

/****************************************************************
 * Function    L_MUTEX_LockTime
 * Args:       1: MUTEX Mutex ID 1-n 2:wait time ticks
 * Return:     ERC RC_MUTEX_INVALID if invalid mutex number OR
 *                 RC_MUTEX_NESTED if already locked by this task OR
 *                 RC_SUCCESS
 * Purpose:    Lock the mutex; wait a time if already locked by another task.
 ****************************************************************/
GLOBAL ERC L_MUTEX_LockTime(MUTEX sMutexID,TICKS sTicks)
{
   TASK ourTask;
   MUTEX_INFO *miptr;

   struct timespec abs_time;
   L_CLOCK_PrepTimeoutTicks(&abs_time, sTicks);

   miptr = MUTEX_GetPtr(sMutexID);              // get the mutex structure
   if (miptr == NULL)
      return(RC_MUTEX_INVALID);

   ourTask = L_TASK_GetID(0);
   if (miptr->OwningTaskID != ourTask) {        // if not ours yet,
         if(pthread_mutex_timedlock(&miptr->MutexStruct,&abs_time)!=0) // wait for acquire lock
            return(RC_MUTEX_BUSY);
         miptr->OwningTaskID = ourTask;            // we own it now
   }

   miptr->NestLevel++;                          // another nest
   if (miptr->NestLevel != 1)                   // if not the first time,
      return(RC_MUTEX_NESTED);                  // return nested status

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_MUTEX_Unlock
 * Args:       1: MUTEX Mutex ID 1-n
 * Return:     ERC RC_MUTEX_INVALID if invalid mutex number OR
 *                 RC_MUTEX_BUSY if locked by another TASK OR
 *                 RC_MUTEX_NESTED if still locked by this task OR
 *                 RC_SUCCESS
 * Purpose:    Unlocks the mutex.
 * Note:       Unlocking a mutex that is not locked returns RC_BUSY.
 ****************************************************************/
GLOBAL ERC L_MUTEX_Unlock(MUTEX sMutexID)
{
   MUTEX_INFO *miptr;
   miptr = MUTEX_GetPtr(sMutexID);        // get the mutex structure
   if (miptr == NULL)
      return(RC_MUTEX_INVALID);

   if (miptr->OwningTaskID != L_TASK_GetID(0))
      return(RC_MUTEX_BUSY);              // not our mutex

   // it is now safe to manipulate the fields inside the mutex,
   // since only our task will make it into this section of code.

   miptr->NestLevel--;                    // one less time locked
   if (miptr->NestLevel != 0)             // if not fully unnested,
      return(RC_MUTEX_NESTED);            // show we are still nested

   miptr->OwningTaskID = 0;               // its up for grabs again

   // okay, now we can unlock it in the operating system
   pthread_mutex_unlock(&miptr->MutexStruct);

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_MUTEX_GetOwnerTaskID
 * Args:       1: MUTEX Mutex ID 1-n
 * Return:     ERC/TASK 1-n if owned by a task OR
 *                      RC_MUTEX_INVALID if invalid mutex number
 * Purpose:    Returns the owner of a mutex.
 ****************************************************************/
GLOBAL TASK L_MUTEX_GetOwnerTaskID(MUTEX sMutexID)
{
   MUTEX_INFO *miptr;
   miptr = MUTEX_GetPtr(sMutexID);        // get the mutex structure
   if (miptr == NULL)
      return(RC_MUTEX_INVALID);

   return(miptr->OwningTaskID);
}


/****************************************************************
 * Function    L_MUTEX_Debug
 * Args:       none
 * Return:     none
 * Purpose:    This function logs information.
 ****************************************************************/
GLOBAL void L_MUTEX_Debug(void)
{
   int x = 1;
   MUTEX_INFO *siptr;

   log_me("mutex information (%d total)", (int)(endMutexArray - ptrMutexArray));
   for (siptr = ptrMutexArray; siptr != endMutexArray; siptr++, x++)
      log_me("  %02d %s: task #%d (%s) nest %d mutex.lock %d", x,
      get_mutex_name(x), siptr->OwningTaskID, get_task_name(siptr->OwningTaskID),
      siptr->NestLevel, siptr->MutexStruct.__data.__lock);
}


