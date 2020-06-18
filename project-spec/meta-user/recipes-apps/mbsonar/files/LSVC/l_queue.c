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
// must define this in order to take advantage of pthread's recursive mutexes
#define _XOPEN_SOURCE 500

/********************           INCLUDE FILES                ******************/
#include <stdlib.h>
#include <stdio.h>
#include <features.h>
#include <string.h>
#include <time.h>
#define _TIME_T
#include <pthread.h>

#include "types.h"
#include "lsvc.h"
#include "l_clock.h"
#include "l_sema.h"
#include "l_queue.h"
#include "l_utils.h"

/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define qiWidth QueueParam.QParamWidth
#define qiDepth QueueParam.QParamDepth
/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
   QUEUE_PARAM       QueueParam;       // width, depth
   U8               *ptrQueueBase;     // pointer to first entry
   U8               *ptrQueueEnd;      // ptr to first byte after last entry

   U32               QueueCount;       // current number of entries
   U8               *ptrQueuePut;      // where to put next entry
   U8               *ptrQueueGet;      // where to get next entry

   U8               *ptrQueueCreated;  // if auto-created, this is the ptr
   U32               sizQueueCreated;  // if auto-created, this is the size

   // one semaphore for each of the queue conditions in linuxsvc.h
   SEMA              QueueSemas[QCOND_COUNT];

   pthread_mutex_t   QueueMutex;
   pthread_condattr_t QueueCondAttr;
   pthread_cond_t    QueueCond;
} QUEUE_INFO;
/********************        FUNCTION PROTOTYPES             ******************/

/********************           LOCAL VARIABLES              ******************/
PRIVATE QUEUE_INFO *ptrQueueArray = NULL;
PRIVATE QUEUE_INFO *endQueueArray;

/********************          GLOBAL VARIABLES              ******************/
extern const QUEUE_PARAM QueueParams[];
/********************              FUNCTIONS                 ******************/

/****************************************************************
 * Function    QUEUE_Init
 * Args:       none
 * Return:     none
 * Purpose:    This function will allocate and initialize the
 *             queue structure array.
 ****************************************************************/
GLOBAL void QUEUE_Init(void)
{
   QUEUE_INFO *qiptr;
   const QUEUE_PARAM *qpptr;

   // declaring this static in case the OS tries to access it later
   PRIVATE pthread_mutexattr_t our_mutexattr;

   ptrQueueArray = calloc(nqueues, sizeof(QUEUE_INFO));
   if (ptrQueueArray == NULL)             // unlikely, but check anyway
      return;

   endQueueArray = &ptrQueueArray[nqueues];

   pthread_mutexattr_init(&our_mutexattr);
   pthread_mutexattr_settype(&our_mutexattr, PTHREAD_MUTEX_RECURSIVE);

   // now, spin through them all init'ing the control structure
   for (qiptr = ptrQueueArray, qpptr = QueueParams; qiptr != endQueueArray; qiptr++, qpptr++) {
      memcpy(&qiptr->QueueParam, qpptr, sizeof(QUEUE_PARAM));
      pthread_mutex_init(&qiptr->QueueMutex, &our_mutexattr);
      pthread_condattr_init(&qiptr->QueueCondAttr);
      pthread_condattr_setclock(&qiptr->QueueCondAttr, CLOCK_MONOTONIC);
      pthread_cond_init(&qiptr->QueueCond, &qiptr->QueueCondAttr);
   }
}


/****************************************************************
 * Function    QUEUE_GetPtr
 * Args:       1: QUEUE queue number
 * Return:     pointer to queue info structure (or NULL if invalid)
 * Purpose:    This function will check the queue value.
 ****************************************************************/
PRIVATE QUEUE_INFO *QUEUE_GetPtr(QUEUE sQueue)
{
   // less than one OR too high a number gets an error here
   if (sQueue < 1 || sQueue > nqueues)
      return(NULL);

   // make sure the array is initialized
   if (ptrQueueArray == NULL)
      return(NULL);

   return(&ptrQueueArray[sQueue - 1]);
}


/****************************************************************
 * Function    L_QUEUE_Define
 * Args:       1: QUEUE queue number
 *             2: U32 size of elements
 *             3: U32 depth of queue (maximum number of elements)
 *             4: buffer pointer (pass NULL for a newly malloc'ed buffer)
 *             5: U32 current count of elements (if arg #4 is non-null)
 * Return:     ERC RC_SUCCESS           if no problems
 *                 RC_QUEUE_INVALID     if queue number is invalid
 *                 RC_QUEUE_NO_MEMORY   if NULL buff ptr and malloc failed
 * Purpose:    This function will malloc memory for the queue buffer and
 *             initialize the control structure.
 ****************************************************************/

// helper function to remove the buffer
PRIVATE void QUEUE_qdFree(QUEUE_INFO *qiptr)
{
   if (qiptr->ptrQueueCreated != NULL)    // if previously created,
      free(qiptr->ptrQueueCreated);       // let it go
   qiptr->ptrQueueCreated = NULL;         // clear out these
   qiptr->sizQueueCreated = 0;
}

GLOBAL ERC L_QUEUE_Define(QUEUE queue, U32 QueueWidth, U32 QueueDepth, U8 *QueueBuffer, U32 QueueCount)
{
   U32 newBufferSize;
   int result = RC_SUCCESS;            // ah, the optimism

   QUEUE_INFO *qiptr = QUEUE_GetPtr(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   pthread_mutex_lock(&qiptr->QueueMutex);

   qiptr->qiWidth = QueueWidth;        // width stored
   qiptr->qiDepth = QueueDepth;        // depth stored

   if (QueueBuffer != NULL)            // providing us with a buffer?
      QUEUE_qdFree(qiptr);             // free one that we created
   else {                              // must create our own buffer
      newBufferSize = QueueWidth * QueueDepth;
      if (newBufferSize == 0 ||        // if not done, and needs more room,
         newBufferSize > qiptr->sizQueueCreated)
         QUEUE_qdFree(qiptr);          // get rid of the [smaller] one

      QueueBuffer = malloc(newBufferSize);
      if (QueueBuffer != NULL) {       // created a new buffer?
         qiptr->ptrQueueCreated = QueueBuffer;
         qiptr->sizQueueCreated = newBufferSize;
      } else
         result = RC_QUEUE_NO_MEMORY;

      QueueCount = 0;                  // new buffer means zero count
   }

   // initialize all buffer pointers to the base first
   qiptr->ptrQueueBase =
   qiptr->ptrQueueEnd =
   qiptr->ptrQueueGet =
   qiptr->ptrQueuePut = QueueBuffer;   // initialize all pointers
   qiptr->QueueCount = QueueCount;     // and current count

   if (QueueBuffer != NULL) {          // if memory actually exists,
      qiptr->ptrQueuePut += (QueueWidth * QueueCount);
      qiptr->ptrQueueEnd += (QueueWidth * QueueDepth);
   }

   pthread_mutex_unlock(&qiptr->QueueMutex);

   return(result);
}


/****************************************************************
 * Function    QUEUE_AutoDefine
 * Args:       1: QUEUE queue number
 * Return:     QUEUE_INFO pointer to queue control structure OR
 *                        NULL if invalid queue or some init failure
 * Purpose:    This function will check the queue value.
 ****************************************************************/
PRIVATE QUEUE_INFO *QUEUE_AutoDefine(QUEUE queue)
{
   QUEUE_INFO *qiptr;

   qiptr = QUEUE_GetPtr(queue);
   if (qiptr != NULL &&                // if a control structure exists,
      qiptr->ptrQueueBase == NULL &&   // and there is no buffer mem yet,
      L_QUEUE_Define(queue, qiptr->qiWidth, qiptr->qiDepth, NULL, 0) != RC_SUCCESS)
      qiptr = NULL;                    // could not create buffer memory

   return(qiptr);
}


/****************************************************************
 * Function    L_QUEUE_Put
 * Args:       1: QUEUE queue number
 *             2: pointer to record
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error OR
 *                 RC_QUEUE_FULL    if unable to enqueue another record
 * Purpose:    Place an entry into the queue, if there is room.
 ****************************************************************/
GLOBAL ERC L_QUEUE_Put(QUEUE queue, void *ptrElement)
{
   int result;
   QUEUE_INFO *qiptr;

   qiptr = QUEUE_AutoDefine(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   pthread_mutex_lock(&qiptr->QueueMutex);

   //FIXME? jcole note: if the queue is already full, the semaphore will not
   //resignal
   result = RC_QUEUE_FULL;
   if (qiptr->QueueCount < qiptr->qiDepth) {
      result = RC_SUCCESS;

      memcpy(qiptr->ptrQueuePut, ptrElement, qiptr->qiWidth);

      // update pointer and wrap around if needed
      qiptr->ptrQueuePut += qiptr->qiWidth;
      if (qiptr->ptrQueuePut == qiptr->ptrQueueEnd)
         qiptr->ptrQueuePut = qiptr->ptrQueueBase;

      // increment the count and signal appropriate semaphores
      qiptr->QueueCount++;

      // commenting out this line makes the SONARSIM work; but that's not how the RTXC code reads
      //if (qiptr->QueueCount == 1)
         L_SEMA_Signal(qiptr->QueueSemas[QNE]);
      if (qiptr->QueueCount == qiptr->qiDepth)
         L_SEMA_Signal(qiptr->QueueSemas[QF]);

      // whenever count is updated, signal the conditional
      pthread_cond_signal(&qiptr->QueueCond);
   }

   pthread_mutex_unlock(&qiptr->QueueMutex);
   return(result);
}


/****************************************************************
 * Function    L_QUEUE_PutWait
 * Args:       1: QUEUE queue number
 *             2: pointer to record
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error
 * Purpose:    Place an entry into the queue; wait if there is no room.
 ****************************************************************/
GLOBAL ERC L_QUEUE_PutWait(QUEUE queue, void *ptrElement)
{
   QUEUE_INFO *qiptr;

   qiptr = QUEUE_AutoDefine(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   pthread_mutex_lock(&qiptr->QueueMutex);
   while (L_QUEUE_Put(queue, ptrElement) == RC_QUEUE_FULL)
      pthread_cond_wait(&qiptr->QueueCond, &qiptr->QueueMutex);
   pthread_mutex_unlock(&qiptr->QueueMutex);

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    QUEUE_PutWaitTimed
 * Args:       1: QUEUE queue number
 *             2: pointer to record
 *             3: structure timeout pointer
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error OR
 *                 RC_QUEUE_FULL    if timeout awaiting buffer not full
 * Purpose:    Put an entry into the queue; wait timed if there is none.
 ****************************************************************/
PRIVATE ERC QUEUE_PutWaitTimed(QUEUE queue, void *ptrElement, struct timespec *tsptr)
{
   QUEUE_INFO *qiptr;
   ERC result = RC_SUCCESS;

   qiptr = QUEUE_AutoDefine(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   pthread_mutex_lock(&qiptr->QueueMutex);
   while (result == RC_SUCCESS && L_QUEUE_Put(queue, ptrElement) == RC_QUEUE_FULL)
      if (pthread_cond_timedwait(&qiptr->QueueCond, &qiptr->QueueMutex, tsptr) != 0)
         result = RC_QUEUE_FULL;
   pthread_mutex_unlock(&qiptr->QueueMutex);
   return(result);
}


/****************************************************************
 * Function    L_QUEUE_PutWaitTicks
 * Args:       1: QUEUE queue number
 *             2: pointer where to place the record
 *             3: TICKS number of ticks before timeout
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error OR
 *                 RC_QUEUE_FULL    if timeout awaiting buffer not full
 * Purpose:    Put an entry into the queue; wait timed if full.
 ****************************************************************/
GLOBAL ERC L_QUEUE_PutWaitTicks(QUEUE queue, void *ptrElement, TICKS sTicks)
{
   // prepare the timeout structure (ABSTIME)
   struct timespec ts_timeout;
   L_CLOCK_PrepTimeoutTicks(&ts_timeout, sTicks);
   return(QUEUE_PutWaitTimed(queue, ptrElement, &ts_timeout));
}


/****************************************************************
 * Function    L_QUEUE_PutWaitSeconds
 * Args:       1: QUEUE queue number
 *             2: pointer where to place the record
 *             3: U32 number of seconds til timeout
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error OR
 *                 RC_QUEUE_FULL    if timeout awaiting buffer not full
 * Purpose:    Put an entry into the queue; wait timed if full.
 ****************************************************************/
GLOBAL ERC L_QUEUE_PutWaitSeconds(QUEUE queue, void *ptrElement, U32 sSeconds)
{
   struct timespec ts_timeout;
   // prepare the timeout structure (ABSTIME)
   L_CLOCK_PrepTimeoutSeconds(&ts_timeout, sSeconds);
   return(QUEUE_PutWaitTimed(queue, ptrElement, &ts_timeout));
}


/****************************************************************
 * Function    L_QUEUE_Get
 * Args:       1: QUEUE queue number
 *             2: pointer where to place the record
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error OR
 *                 RC_QUEUE_EMPTY   if no entries are available in the queue
 * Purpose:    Get an entry from the queue.
 ****************************************************************/
GLOBAL ERC L_QUEUE_Get(QUEUE queue, void *ptrElement)
{
   int result;
   QUEUE_INFO *qiptr;

   qiptr = QUEUE_AutoDefine(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   pthread_mutex_lock(&qiptr->QueueMutex);

   result = RC_QUEUE_EMPTY;
   if (qiptr->QueueCount > 0) {
      result = RC_SUCCESS;

      memcpy(ptrElement, qiptr->ptrQueueGet, qiptr->qiWidth);

      // update pointer and wrap around if needed
      qiptr->ptrQueueGet += qiptr->qiWidth;
      if (qiptr->ptrQueueGet == qiptr->ptrQueueEnd)
         qiptr->ptrQueueGet = qiptr->ptrQueueBase;

      // decrement the count and signal appropriate semaphores
      qiptr->QueueCount--;
      if (qiptr->QueueCount == (qiptr->qiDepth - 1))
         L_SEMA_Signal(qiptr->QueueSemas[QNF]);
      if (qiptr->QueueCount == 0)
         L_SEMA_Signal(qiptr->QueueSemas[QE]);

      // whenever count is updated, signal the conditional
      pthread_cond_signal(&qiptr->QueueCond);
   }

   pthread_mutex_unlock(&qiptr->QueueMutex);
   return(result);
}


/****************************************************************
 * Function    L_QUEUE_GetWait
 * Args:       1: QUEUE queue number
 *             2: pointer where to place the record
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error
 * Purpose:    Get an entry from the queue; wait if there is none.
 ****************************************************************/
GLOBAL ERC L_QUEUE_GetWait(QUEUE queue, void *ptrElement)
{
   QUEUE_INFO *qiptr;

   qiptr = QUEUE_AutoDefine(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   pthread_mutex_lock(&qiptr->QueueMutex);
   while (L_QUEUE_Get(queue, ptrElement) == RC_QUEUE_EMPTY)
      pthread_cond_wait(&qiptr->QueueCond, &qiptr->QueueMutex);
   pthread_mutex_unlock(&qiptr->QueueMutex);

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    QUEUE_GetWaitTimed
 * Args:       1: QUEUE queue number
 *             2: pointer where to place the record
 *             3: structure timeout pointer
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error OR
 *                 RC_QUEUE_EMPTY   if timeout awaiting buffer not empty
 * Purpose:    Get an entry from the queue; wait timed if there is none.
 ****************************************************************/
PRIVATE ERC QUEUE_GetWaitTimed(QUEUE queue, void *ptrElement, struct timespec *tsptr)
{
   QUEUE_INFO *qiptr;
   ERC result = RC_SUCCESS;

   qiptr = QUEUE_AutoDefine(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   pthread_mutex_lock(&qiptr->QueueMutex);
   while (result == RC_SUCCESS && L_QUEUE_Get(queue, ptrElement) == RC_QUEUE_EMPTY)
      if (pthread_cond_timedwait(&qiptr->QueueCond, &qiptr->QueueMutex, tsptr) != 0)
         result = RC_QUEUE_EMPTY;
   pthread_mutex_unlock(&qiptr->QueueMutex);
   return(result);
}


/****************************************************************
 * Function    L_QUEUE_GetWaitTicks
 * Args:       1: QUEUE queue number
 *             2: pointer where to place the record
 *             3: TICKS number of ticks before timeout
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error OR
 *                 RC_QUEUE_EMPTY   if timeout awaiting buffer not empty
 * Purpose:    Get an entry from the queue; wait timed if there is none.
 ****************************************************************/
GLOBAL ERC L_QUEUE_GetWaitTicks(QUEUE queue, void *ptrElement, TICKS sTicks)
{
   // prepare the timeout structure (ABSTIME)
   struct timespec ts_timeout;
   L_CLOCK_PrepTimeoutTicks(&ts_timeout, sTicks);
   return(QUEUE_GetWaitTimed(queue, ptrElement, &ts_timeout));
}


/****************************************************************
 * Function    L_QUEUE_GetWaitSeconds
 * Args:       1: QUEUE queue number
 *             2: pointer where to place the record
 *             3: U32 number of seconds til timeout
 * Return:     ERC RC_SUCCESS       if no problems OR
 *                 RC_QUEUE_INVALID if invalid queue or init error OR
 *                 RC_QUEUE_EMPTY   if timeout awaiting buffer not empty
 * Purpose:    Get an entry from the queue; wait timed if there is none.
 ****************************************************************/
GLOBAL ERC L_QUEUE_GetWaitSeconds(QUEUE queue, void *ptrElement, U32 sSeconds)
{
   struct timespec ts_timeout;
   // prepare the timeout structure (ABSTIME)
   L_CLOCK_PrepTimeoutSeconds(&ts_timeout, sSeconds);
   return(QUEUE_GetWaitTimed(queue, ptrElement, &ts_timeout));
}


/****************************************************************
 * Function    L_QUEUE_Purge
 * Args:       1: QUEUE queue number
 * Return:     ERC RC_SUCCESS          if no problems OR
 *                 RC_QUEUE_INVALID    if invalid queue or init error
 * Purpose:    Simulate draining a queue (potentially firing semaphores).
 ****************************************************************/
GLOBAL ERC L_QUEUE_Purge(QUEUE queue)
{
   U32 TempCount;
   QUEUE_INFO *qiptr;

   // using GetPtr here instead of AutoDefine, since an un-init'ed queue
   // will return with the compare count == 0 (no need to init yet)
   qiptr = QUEUE_GetPtr(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   if (qiptr->QueueCount == 0)            // nothing to purge
      return(RC_SUCCESS);

   pthread_mutex_lock(&qiptr->QueueMutex);

   TempCount = qiptr->QueueCount;         // keep this for later

   qiptr->QueueCount = 0;                 // clear count and reset pointer
   qiptr->ptrQueueGet = qiptr->ptrQueuePut;

   if (TempCount == qiptr->qiDepth)       // if previously full, signal QNF
      L_SEMA_Signal(qiptr->QueueSemas[QNF]);

   L_SEMA_Signal(qiptr->QueueSemas[QE]);  // always try to signal QE

   pthread_cond_signal(&qiptr->QueueCond);
   pthread_mutex_unlock(&qiptr->QueueMutex);

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_QUEUE_DefineSema
 * Args:       1: QUEUE queue number
 *             2: SEMA semaphore number
 *             3: QUEUE_CONDITION
 * Return:     ERC RC_SUCCESS          if no problems OR
 *                 RC_QUEUE_INVALID    if invalid queue or init error
 *                 RC_QCOND_INVALID    if invalid queue condition
 *                 RC_SEMA_INVALID     if invalid semaphore condition
 * Purpose:    Associate a semaphore with a queue.
 ****************************************************************/
GLOBAL ERC L_QUEUE_DefineSema(QUEUE queue, SEMA sema, QUEUE_CONDITION qcond)
{
   QUEUE_INFO *qiptr;

   // using GetPtr here instead of AutoDefine, since an un-init'ed queue
   // is able to handle definition of semaphores (no need to init yet)
   qiptr = QUEUE_GetPtr(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   if (qcond < 0 || qcond >= QCOND_COUNT)
      return(RC_QCOND_INVALID);

   // allow 0-n, not 1-n, since 0 is the way to disassociate the semaphore
   if (sema < 0 || sema > nsemas)
      return(RC_SEMA_INVALID);

   qiptr->QueueSemas[qcond] = sema;
   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_QUEUE_Inquire
 * Args:       1: QUEUE queue number
 * Return:     ERC RC_QUEUE_INVALID    if invalid queue or init error OR
 *                 >= 0 count of entries in the queue
 * Purpose:    Returns the number of entries in the queue
 ****************************************************************/
GLOBAL ERC L_QUEUE_Inquire(QUEUE queue)
{
   QUEUE_INFO *qiptr;

   // using GetPtr here instead of AutoDefine, since an un-init'ed queue
   // is able to return the entry count as zero (no need to init yet)
   qiptr = QUEUE_GetPtr(queue);
   if (qiptr == NULL)
      return(RC_QUEUE_INVALID);

   return(qiptr->QueueCount);
}


/****************************************************************
 * Function    L_QUEUE_Debug
 * Args:       none
 * Return:     none
 * Purpose:    This function logs information.
 ****************************************************************/
GLOBAL void L_QUEUE_Debug(void)
{
   int x = 1;
   QUEUE_INFO *siptr;

   log_me("queue information (%d total)", (int)(endQueueArray - ptrQueueArray));
   for (siptr = ptrQueueArray; siptr != endQueueArray; siptr++, x++)
      log_me("  %02d %s: width %d depth %d count %d QNE %d QNF %d QF %d QE %d mutex.lock %d", x,
      get_queue_name(x), siptr->QueueParam.QParamWidth, siptr->QueueParam.QParamWidth,
      siptr->QueueCount, siptr->QueueSemas[QNE], siptr->QueueSemas[QNF],
      siptr->QueueSemas[QF], siptr->QueueSemas[QE], siptr->QueueMutex.__data.__lock);
}


