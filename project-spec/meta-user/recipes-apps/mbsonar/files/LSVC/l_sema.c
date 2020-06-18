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
#include <time.h>
#define _TIME_T
#include <pthread.h>

#include "types.h"
#include "lsvc.h"
#include "l_clock.h"
#include "l_sema.h"
#include "l_utils.h"
/********************               ENUMS                    ******************/
// chasing something ... requires DBG_WHEEL also!
//#define DBG_SEMA
#define SEMA_WAIT_LOGGING 1

/********************              DEFINES                   ******************/
// internal states of the semaphore (actually, these are bit values)
#define SEMST_PENDING 0x00
#define SEMST_WAITING 0x01
#define SEMST_DONE    0x02
#define SEMST_RESET   (SEMST_WAITING | SEMST_DONE)
/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
   U8              u8SemaState;      // current state (see bit values above)
   TASK            WaitingTaskID;    // the thread awaiting this signal
   U64             u64FireTimestamp; // when [most recently] fired
   pthread_mutex_t SemaMutex;        // mutex for this semaphore
} SEMA_INFO;
/********************        FUNCTION PROTOTYPES             ******************/

/********************           LOCAL VARIABLES              ******************/
PRIVATE SEMA_INFO *ptrSemaphoreArray = NULL;
PRIVATE SEMA_INFO *endSemaphoreArray;
/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/

/****************************************************************
 * Function    SEMA_Init
 * Args:       none
 * Return:     none
 * Purpose:    This function will allocate and initialize the
 *             semaphore structure array.
 ****************************************************************/
GLOBAL void SEMA_Init(void)
{
   SEMA_INFO *siptr;

   ptrSemaphoreArray = calloc(nsemas, sizeof(SEMA_INFO));
   if (ptrSemaphoreArray == NULL)         // unlikely, but check anyway
      return;

   endSemaphoreArray = &ptrSemaphoreArray[nsemas];

   // now, spin through them all initializing the mutexes and conditionals
   for (siptr = ptrSemaphoreArray; siptr != endSemaphoreArray; siptr++)
      pthread_mutex_init(&siptr->SemaMutex, NULL);
}


/****************************************************************
 * Function    SEMA_GetPtr
 * Args:       1: SEMA semaphore number
 * Return:     pointer to semaphore info structure (or NULL if invalid)
 * Purpose:    This function will check the semaphore value.
 ****************************************************************/
PRIVATE SEMA_INFO *SEMA_GetPtr(SEMA sSemaphore)
{
   // less than one OR too high a number gets an error here
   if (sSemaphore < 1 || sSemaphore > nsemas)
      return(NULL);

   // make sure the array is initialized
   if (ptrSemaphoreArray == NULL)
      return(NULL);

   return(&ptrSemaphoreArray[sSemaphore - 1]);
}


/****************************************************************
 * Function    SEMA_CheckList
 * Args:       1: SEMA pointer to semaphore number array (0-terminated)
 * Return:     ERC RC_SEMA_INVALID if any one is invalid OR
 *                 RC_SUCCESS      if all are valid
 * Purpose:    This function will check OR count the semaphore values.
 * Note:       See the NO_PARTIAL_LISTS define.
 ****************************************************************/
PRIVATE ERC SEMA_CheckList(SEMA *spSemaphores)
{
#ifdef NO_PARTIAL_LISTS
   // spin through them all checking for an invalid value
   for (; spSemaphores != NULL && *spSemaphores != 0; spSemaphores++)
      if (*spSemaphores < 1 || *spSemaphores > nsemas)
         return(RC_SEMA_INVALID);
#else
   S32 result = 0;

   // spin through them all counting the valid values
   for (; spSemaphores != NULL && *spSemaphores != 0; spSemaphores++)
      if (*spSemaphores >= 1 && *spSemaphores <= nsemas)
         result++;

   if (result < 1)
      return(RC_SEMA_INVALID);
#endif
   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_SEMA_GetWaitingTaskID
 * Args:       1: SEMA semaphore number
 * Return:     TASK/ERC RC_SEMA_INVALID if invalid semaphore argument OR
 *                      RC_SEMA_PENDING if semaphore is pending OR
 *                      RC_SEMA_DONE    if semaphore is signaled OR
 *                      TASK ID 1-n     if semaphore is waiting
 * Purpose:    This function will return the status of a semaphore.
 ****************************************************************/
GLOBAL TASK L_SEMA_GetWaitingTaskID(SEMA sSemaphore)
{
   SEMA_INFO *siptr;
   TASK state;

   siptr = SEMA_GetPtr(sSemaphore);       // get ptr to sema info
   if (siptr == NULL)                     // not a valid input arg?
      return(RC_SEMA_INVALID);            // correct, return error

// tmp
// log_me("inquire %s", get_semaphore_name(sSemaphore));

   pthread_mutex_lock(&siptr->SemaMutex);

   // translate the state to the appropriate return value
   state = RC_SEMA_PENDING;               // return code for pending
   if ((siptr->u8SemaState & SEMST_DONE) != 0)
      state = RC_SEMA_DONE;               // return code for done
   if ((siptr->u8SemaState & SEMST_WAITING) != 0)
      state = siptr->WaitingTaskID;       // return task ID that's waiting

#ifdef DBG_SEMA
   if (sSemaphore == 3)
      dbg_add(0, siptr->u8SemaState);
#endif

   pthread_mutex_unlock(&siptr->SemaMutex);

   return(state);
}


/****************************************************************
 * Function    L_SEMA_SetPending
 * Args:       1: SEMA semaphore number
 * Return:     ERC RC_SEMA_INVALID if invalid semaphore number
 *                 RC_SUCCESS      if valid semaphore number
 * Purpose:    This function sets a semaphore's state = PENDING
 ****************************************************************/
GLOBAL ERC L_SEMA_SetPending(SEMA sSemaphore)
{
   SEMA_INFO *siptr;

   siptr = SEMA_GetPtr(sSemaphore);       // get ptr to sema info
   if (siptr == NULL)                     // not a valid input arg?
      return(RC_SEMA_INVALID);

   pthread_mutex_lock(&siptr->SemaMutex); // lock down

#ifdef DBG_SEMA
   if (sSemaphore == 3)
      dbg_add(1, siptr->u8SemaState);
#endif

   // if there is no task waiting, set it to pending
   if ((siptr->u8SemaState & SEMST_WAITING) == 0)
      siptr->u8SemaState = SEMST_PENDING;

#ifdef DBG_SEMA
   if (sSemaphore == 3)
      dbg_add(1, siptr->u8SemaState);
#endif

   pthread_mutex_unlock(&siptr->SemaMutex);

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_SEMA_SetPendingMultiple
 * Args:       1: SEMA pointer to a null-terminated semaphore list
 * Return:     ERC RC_SEMA_INVALID if invalid semaphore number
 *                 RC_SUCCESS      if valid semaphore number
 * Purpose:    This function sets the semaphores' states = PENDING
 ****************************************************************/
GLOBAL ERC L_SEMA_SetPendingMultiple(SEMA *spSemaphores)
{
   if (SEMA_CheckList(spSemaphores) != RC_SUCCESS)
      return(RC_SEMA_INVALID);

   for (; *spSemaphores != 0; spSemaphores++)
      L_SEMA_SetPending(*spSemaphores);

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_SEMA_Signal
 * Args:       1: SEMA semaphore number
 * Return:     ERC RC_SUCCESS      if successful OR
 *                 RC_SEMA_INVALID if invalid input argument OR
 *                 RC_SEMA_OVERRUN if semaphore already signaled
 * Purpose:    This function sets the DONE flag in the semaphore
 *             state and signals a waiting task, if appropriate.
 ****************************************************************/
GLOBAL ERC L_SEMA_Signal(SEMA sSemaphore)
{
   SEMA_INFO *siptr;
   ERC result;

   siptr = SEMA_GetPtr(sSemaphore);       // get ptr to sema info
   if (siptr == NULL)                     // not a valid input arg?
      return(RC_SEMA_INVALID);            // correct, return error

   pthread_mutex_lock(&siptr->SemaMutex); // lock semaphore access

   // we are going to fire the semaphore only if it is not already fired
   result = RC_SEMA_OVERRUN;              // assume it already fired
   if ((siptr->u8SemaState & SEMST_DONE) == 0) {
      result = RC_SUCCESS;                // things are good

      siptr->u64FireTimestamp = L_CLOCK_qwGetTimeInNans();
      siptr->u8SemaState |= SEMST_DONE;   // show it is done now

#ifdef DBG_SEMA
      if (sSemaphore == 3)
         dbg_add(2, siptr->u8SemaState);
#endif

      // if this semaphore is also waiting, signal the waiting thread
      if (siptr->u8SemaState == SEMST_RESET)
         L_TASK_SignalConditional(siptr->WaitingTaskID);
   }

   pthread_mutex_unlock(&siptr->SemaMutex);

   return(result);
}


/****************************************************************
 * Function    L_SEMA_SignalMultiple
 * Args:       1: SEMA pointer to a null-terminated semaphore list
 * Return:     ERC RC_SEMA_INVALID if invalid semaphore number
 *                 RC_SUCCESS      if valid semaphore number
 * Purpose:    This calls L_SEMA_Signal with each of the semaphores
 *             in the list, the returns from which are ignored.
 ****************************************************************/
GLOBAL ERC L_SEMA_SignalMultiple(SEMA *spSemaphores)
{
   if (SEMA_CheckList(spSemaphores) != RC_SUCCESS)
      return(RC_SEMA_INVALID);

   for (; *spSemaphores != 0; spSemaphores++)
      L_SEMA_Signal(*spSemaphores);

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    SEMA_WaitInternal with helper functions
 * Args:       1: SEMA ptr to semaphore list (with at least one valid entry)
 *             2: struct timeout *
 * Return:     SEMA/ERC RC_SEMA_CONFLICT    if a wait conflict OR
 *                      RC_SEMA_TIMEOUT     if a timeout occurred OR
 *                      semaphore ID 1-n    if a semaphore fired
 * Purpose:    This function combines Wait, WaitMultiple, and WaitTimed
 ****************************************************************/

// these helper functions are used by SEMA_WaitInternal
PRIVATE void SEMA_wiLock(SEMA *pSema)
{
   SEMA_INFO *sxptr;          // used as temp ptr during spins

   // lock the semaphores' mutexes
   for (; *pSema != 0; pSema++) {
      sxptr = SEMA_GetPtr(*pSema);        // get the next one
#ifdef DBG_SEMA
      if (*pSema == 3)
         dbg_add(7, sxptr->u8SemaState);
#endif
      if (sxptr != NULL)
         pthread_mutex_lock(&sxptr->SemaMutex);
#ifdef DBG_SEMA
      if (*pSema == 3)
         dbg_add(8, sxptr->u8SemaState);
#endif
   }
}

PRIVATE void SEMA_wiUnlock(SEMA *pSema)
{
   SEMA_INFO *sxptr;          // used as temp ptr during spins

   // unlock the semaphores' mutexes
   for (; *pSema != 0; pSema++) {
      sxptr = SEMA_GetPtr(*pSema);        // get the next one
#ifdef DBG_SEMA
      if (*pSema == 3)
         dbg_add(9, sxptr->u8SemaState);
#endif
      if (sxptr != NULL)
         pthread_mutex_unlock(&sxptr->SemaMutex);
   }
}

PRIVATE BOOLEAN SEMA_wiCheckFired(SEMA *pSema)
{
   SEMA_INFO *sxptr;          // used as temp ptr during spins

   // returns true if any of the list has fired
   for (; *pSema != 0; pSema++) {
      sxptr = SEMA_GetPtr(*pSema);
      if (sxptr != NULL &&
         (sxptr->u8SemaState & SEMST_DONE) != 0)
         return(TRUE);
   }
   return(FALSE);
}

PRIVATE BOOLEAN SEMA_wiCheckOlder(SEMA_INFO *srptr, SEMA_INFO *sxptr)
{
   // returns false if srptr is older and true if sxptr is older

   // check 1: return true if no srptr (sxptr is older)
   return (srptr == NULL ||

   // check 2: return true if srptr greater (sxptr is older)
           srptr->u64FireTimestamp > sxptr->u64FireTimestamp);
}

PRIVATE SEMA SEMA_WaitInternal(SEMA *spSemaphores, struct timespec *toptr)
{
#if SEMA_WAIT_LOGGING
   int millis = lm_gettime(NULL);
#endif
   SEMA result;
   TASK our_task;
   SEMA *pSema;               // used to spin thru the arg list
   SEMA_INFO *sxptr;          // used as temp ptr during spins
   SEMA_INFO *srptr;          // used to hold the oldest-fired semaphore

   our_task = L_TASK_GetID(0);         // get task ID for our own thread

   if (our_task < 1)                      // not a valid task found?
   {
      return(our_task);                   // correct -- return this ERC
   }

   SEMA_wiLock(spSemaphores);             // lock all semaphore mutexes

   // check for the existence of a wait conflict
   result = RC_SUCCESS;                   // so far, so good

   for (pSema = spSemaphores; *pSema != 0; pSema++) 
   {
      sxptr = SEMA_GetPtr(*pSema);        // get the next one

      if (sxptr != NULL && sxptr->WaitingTaskID != 0) 
      {
         result = RC_SEMA_WAIT_CONFLICT;  // show the error here
         break;                           // quit early
      }
   }

   // if there is no wait conflict, continue processing
   if (result == RC_SUCCESS) 
   {
      // now set the waiting state for all the semaphores in question
      for (pSema = spSemaphores; *pSema != 0; pSema++) 
      {
         sxptr = SEMA_GetPtr(*pSema);

         if (sxptr != NULL) {
#ifdef DBG_SEMA
            if (*pSema == 3)
               dbg_add(6, sxptr->u8SemaState);
#endif
            sxptr->u8SemaState |= SEMST_WAITING;
#ifdef DBG_SEMA
            if (*pSema == 3)
               dbg_add(3, sxptr->u8SemaState);
#endif
            sxptr->WaitingTaskID = our_task;
         }
      }

      // switch our control from "all semas" lock to "task" lock
      L_TASK_LockMutex(our_task);         // lock our task's mutex
      SEMA_wiUnlock(spSemaphores);        // unlock the sema mutexes

      while (result == RC_SUCCESS &&      // while not timeout
             SEMA_wiCheckFired(spSemaphores) == 0) 
      {
         if (toptr == NULL)
	 {
            L_TASK_WaitConditional(our_task);
         }
         else
	 {
            result = L_TASK_WaitConditionalTimed(our_task, toptr);
	 }
      }

      // now switch back to "all semas" lock; if interrupted in the
      // middle of this, it will only be to signal another semaphore,
      // which is handled by the "find oldest timestamp" logic
      L_TASK_UnlockMutex(our_task);
      SEMA_wiLock(spSemaphores);          // back to a locked state

      srptr = NULL;
      if (result == RC_SUCCESS) {
         // no timeout, so loop to find the oldest fired semaphore
         for (pSema = spSemaphores; *pSema != 0; pSema++) {
            sxptr = SEMA_GetPtr(*pSema);
            if (sxptr != NULL &&
               (sxptr->u8SemaState & SEMST_DONE) != 0 &&
               SEMA_wiCheckOlder(srptr, sxptr)) {
                  srptr = sxptr;
                  result = *pSema;
            }
         }
      }

      if (srptr != NULL) {                // remove the fired state
         srptr->u8SemaState &= ~SEMST_DONE;
#ifdef DBG_SEMA
         if (result == 3)
            dbg_add(4, sxptr->u8SemaState);
#endif
      }

      // now set the waiting state for all the semaphores in question
      for (pSema = spSemaphores; *pSema != 0; pSema++) {
         sxptr = SEMA_GetPtr(*pSema);
         if (sxptr != NULL) {
            sxptr->u8SemaState &= ~SEMST_WAITING;
#ifdef DBG_SEMA
            if (*pSema == 3)
               dbg_add(5, sxptr->u8SemaState);
#endif
            sxptr->WaitingTaskID = 0;
         }
      }
   }

   SEMA_wiUnlock(spSemaphores);
   return(result);
}

/****************************************************************
 * Function    L_SEMA_Wait
 * Args:       1: SEMA semaphore number
 * Return:     ERC RC_SUCCESS            if successful OR
 *                 RC_SEMA_INVALID       if invalid input argument OR
 *                 RC_SEMA_WAIT_CONFLICT if a wait conflict
 * Purpose:    This function awaits a semaphore signal.
 ****************************************************************/
GLOBAL ERC L_SEMA_Wait(SEMA sSemaphore)
{
   SEMA result;
   SEMA_INFO *siptr;
   SEMA semalist[2];

//   printf("[%d]%s: sSemaphore %d\n", __LINE__, __FUNCTION__, sSemaphore);
   siptr = SEMA_GetPtr(sSemaphore);       // get ptr to sema info

   if (siptr == NULL)                     // not a valid input arg?
   {
      return(RC_SEMA_INVALID);            // correct, return error
   }

   // call the internal routine for this
   semalist[0] = sSemaphore;
   semalist[1] = 0;
   printf("[%d]%s: call SEMA_WaitInternal\n", __LINE__, __FUNCTION__);
   result = SEMA_WaitInternal(semalist, NULL);
   printf("[%d]%s: result %d\n", __LINE__, __FUNCTION__, result);

   if (result > RC_SUCCESS)               // if it comes back as an ID,
   {
      result = RC_SUCCESS;                // we return success, not the ID
   }

   return(result);
}


/****************************************************************
 * Function    L_SEMA_WaitMultiple
 * Args:       1: SEMA ptr to semaphore list (with at least one valid entry)
 * Return:     SEMA/ERC semaphore number      if successful OR
 *                      RC_SEMA_INVALID       if invalid input argument OR
 *                      RC_SEMA_WAIT_CONFLICT if a wait conflict
 * Purpose:    This function awaits one of many semaphores' signal.
 ****************************************************************/
GLOBAL SEMA L_SEMA_WaitMultiple(SEMA *spSemaphores)
{
   if (SEMA_CheckList(spSemaphores) != RC_SUCCESS)
      return(RC_SEMA_INVALID);

   return(SEMA_WaitInternal(spSemaphores, NULL));
}


/****************************************************************
 * Function    L_SEMA_WaitTicks
 * Args:       1: SEMA ptr to semaphore list (with at least one valid entry)
 *             2: TICKS number of ticks before timeout returned
 * Return:     ERC RC_SUCCESS            if successful OR
 *                 RC_SEMA_INVALID       if invalid input argument OR
 *                 RC_SEMA_WAIT_CONFLICT if a wait conflict OR
 *                 RC_TIMEOUT            if a timeout occurred
 * Purpose:    This function awaits a semaphore signal with timeout.
 ****************************************************************/
GLOBAL ERC L_SEMA_WaitTicks(SEMA sSemaphore, TICKS sTicks)
{
   SEMA result;
   SEMA_INFO *siptr;
   SEMA semalist[2];
   struct timespec ts_timeout;

   siptr = SEMA_GetPtr(sSemaphore);       // get ptr to sema info
   if (siptr == NULL)                     // not a valid input arg?
      return(RC_SEMA_INVALID);            // correct, return error

   // prepare the timeout structure (ABSTIME)
   L_CLOCK_PrepTimeoutTicks(&ts_timeout, sTicks);

   // call the internal routine for this
   semalist[0] = sSemaphore;
   semalist[1] = 0;
   result = SEMA_WaitInternal(semalist, &ts_timeout);

   if (result > RC_SUCCESS)               // if it comes back as an ID,
      result = RC_SUCCESS;                // we return success, not the ID

   return(result);
}


/****************************************************************
 * Function    L_SEMA_WaitSeconds
 * Args:       1: SEMA ptr to semaphore list (with at least one valid entry)
 *             2: S32 number of seconds before timeout returned
 * Return:     ERC RC_SUCCESS            if successful OR
 *                 RC_SEMA_INVALID       if invalid input argument OR
 *                 RC_SEMA_WAIT_CONFLICT if a wait conflict OR
 *                 RC_TIMEOUT            if a timeout occurred
 * Purpose:    This function awaits a semaphore signal with timeout.
 ****************************************************************/
GLOBAL ERC L_SEMA_WaitSeconds(SEMA sSemaphore, S32 sSeconds)
{
   SEMA result;
   SEMA_INFO *siptr;
   SEMA semalist[2];
   struct timespec ts_timeout;

   siptr = SEMA_GetPtr(sSemaphore);       // get ptr to sema info
   if (siptr == NULL)                     // not a valid input arg?
      return(RC_SEMA_INVALID);            // correct, return error

   // prepare the timeout structure (ABSTIME)
   L_CLOCK_PrepTimeoutSeconds(&ts_timeout, sSeconds);

   // call the internal routine for this
   semalist[0] = sSemaphore;
   semalist[1] = 0;
   result = SEMA_WaitInternal(semalist, &ts_timeout);

   if (result > RC_SUCCESS)               // if it comes back as an ID,
      result = RC_SUCCESS;                // we return success, not the ID

   return(result);
}


/****************************************************************
 * Function    L_SEMA_Debug
 * Args:       none
 * Return:     none
 * Purpose:    This function logs information.
 ****************************************************************/
GLOBAL void L_SEMA_Debug(void)
{
   int x = 1;
   SEMA_INFO *siptr;

   static char *strs[] = {"pending", "waiting", "done", "reset"};

   log_me("semaphore information (%d total)", (int)(endSemaphoreArray - ptrSemaphoreArray));
   for (siptr = ptrSemaphoreArray; siptr != endSemaphoreArray; siptr++, x++)
      log_me("  %02d %s: %s task #%d (%s) fired at %lld mutex.lock %d", x,
      get_semaphore_name(x), strs[siptr->u8SemaState],
      siptr->WaitingTaskID, get_task_name(siptr->WaitingTaskID),
      siptr->u64FireTimestamp, siptr->SemaMutex.__data.__lock);
}


