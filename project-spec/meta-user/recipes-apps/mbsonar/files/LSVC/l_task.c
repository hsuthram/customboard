/*******************************************************************************
*        Copyright (c) 2009 - 2015 Johnson Outdoors Marine Electronics, Inc.   *
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
#define  USE_SCH_OTHER
// must define this in order to take advantage of pthread's recursive mutexes
#define  _XOPEN_SOURCE 500
/********************           INCLUDE FILES                ******************/
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#define _TIME_T
#include <limits.h>
#include <signal.h>
#include <bits/local_lim.h>

#include "types.h"
#include "lsvc.h"
#include "l_utils.h"

#define IS_L_TASK
//#include "l_timer.h"
#include "l_task.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
// we need to define one extra task for the main thread of our process
#define TASK_COUNT (ntasks + 1)

// #define ENABLE_TASK_LOGS

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
   TASK_PARAM         TaskParam;     // things needed at start-up
   pthread_t          ThreadID;      // thread ID
   pthread_mutex_t    TaskMutex;     // mutex
   pthread_condattr_t TaskCondAttr; // conditional attributes
   pthread_cond_t     TaskCond;      // conditional
   void              *TaskStackPtr;  // pointer to allocated stack memory
   int                active_count;  // activation counter
   struct timeval     activ_time;    // activation timer
   struct timeval     accum_time;    // accumulated running time
} TASK_INFO;
/********************        FUNCTION PROTOTYPES             ******************/
// not interested in figuring out the pthread.h defines ... just declare this
extern int pthread_attr_setstack (pthread_attr_t *__attr, void *__stackaddr,
              size_t __stacksize) __THROW __nonnull ((1));
/********************           LOCAL VARIABLES              ******************/
PRIVATE TASK_INFO *ptrTaskArray = NULL;
PRIVATE TASK_INFO *endTaskArray;

// for signaling that a newly-started task is running at the proper priority
PRIVATE pthread_mutex_t KickOffMutex;
PRIVATE pthread_cond_t KickOffCond;
/********************          GLOBAL VARIABLES              ******************/
//BOOLEAN task_disabled[64] = {0};
BOOLEAN task_disabled[NTASKS] = {0};

extern const TASK_PARAM TaskParams[];
/********************              FUNCTIONS                 ******************/

#if 0
void log_8(char *hptr, void *vptr)
{
   char *cptr = vptr;
   char cbuf[32];
   char *optr = cbuf;
   int x;

   for (x = 0; x < 8; x++)
      optr += sprintf(optr, "%02x", *cptr++);
   log_me("%s: %s", hptr, cbuf);
}
#endif


/****************************************************************
 * Function    TASK_Exit
 * Args:       none
 * Return:     none
 * Purpose:    Exit the thread; executes as a signal handler.
 ****************************************************************/
PRIVATE void TASK_Exit(int signo)
{
   L_TASK_Deactivate();
   pthread_exit(NULL);
}

/******************************************************************************
 *
 *    Function: TASK_u32ConvertPriorityFromRTXC
 *
 *    Args:    RTXC priority
 *
 *    Return:  Linux priority
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE U32 TASK_u32ConvertPriorityFromRTXC( PRIORITY sPriority )
{
#ifdef USE_SCH_OTHER
   S32   s32LinuxPriority;

   s32LinuxPriority = 100 - sPriority;
   if ( s32LinuxPriority <= 0 || sched_get_priority_max(SCHED_OTHER) <= 0)
   {
      return( 0 );
   }
// TAC: BTW, the min and max for OTHER on our target are both 0
   if ( s32LinuxPriority > (sched_get_priority_max(SCHED_OTHER) - 1) )
   {
      return( sched_get_priority_max(SCHED_OTHER) - 1 );
   }
   if ( s32LinuxPriority < sched_get_priority_min(SCHED_OTHER) )
   {
      return( sched_get_priority_min(SCHED_OTHER) );
   }
   return( (U32) s32LinuxPriority );
#else
// TAC: back to the way it was, but with FIFO instead of RR
   if (sPriority > 30)           // if above this,
   {
      sPriority -= 10;           // shorten range from 1-100 to 1-90
   }
   sPriority = sched_get_priority_max(SCHED_FIFO) - sPriority;
   if (sPriority < sched_get_priority_min(SCHED_FIFO))
   {
      sPriority = sched_get_priority_min(SCHED_FIFO);
   }
   return(sPriority);
#endif
}

/******************************************************************************
 *
 *    Function: TASK_u32ConvertPriorityToRTXC
 *
 *    Args:    linux priority
 *
 *    Return:  RTXC like priority
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE PRIORITY TASK_u32ConvertPriorityToRTXC( U32 u32LinuxPriority )
{
#ifdef USE_SCH_OTHER
   return( 100 - u32LinuxPriority );
#else
// TAC: back to the way it was, but with FIFO not RR
   u32LinuxPriority = sched_get_priority_max(SCHED_FIFO) - u32LinuxPriority;
   if (u32LinuxPriority > 20)
   {
      u32LinuxPriority += 10;
   }
   return(u32LinuxPriority);
#endif
}

/****************************************************************
 * Function    TASK_Init
 * Args:       none
 * Return:     none
 * Purpose:    This function will allocate and initialize the
 *             task structure array.
 ****************************************************************/
GLOBAL void TASK_Init(void)
{
// int x;
   TASK_INFO        *tiptr;
   const TASK_PARAM *tpptr;
   struct sigaction  our_sigaction;

   // declaring this static in case the OS tries to access it later
   PRIVATE pthread_mutexattr_t our_mutexattr;

   ptrTaskArray = calloc(TASK_COUNT, sizeof(TASK_INFO));
   printf("[%d]%s: TASK_COUNT %d\n", __LINE__, __FUNCTION__, TASK_COUNT);

   if (ptrTaskArray == NULL)              // unlikely, but check anyway
   {
      return;
   }

   endTaskArray = &ptrTaskArray[TASK_COUNT];
   pthread_mutexattr_init(&our_mutexattr);
   pthread_mutexattr_settype(&our_mutexattr, PTHREAD_MUTEX_RECURSIVE);

   // now, spin through them all initializing the mutexes and conditionals
   for (tiptr = ptrTaskArray, tpptr = TaskParams; tiptr != endTaskArray; tiptr++, tpptr++)
   {
      // copy over params -- but the main thread does not have params
      if (tiptr != endTaskArray - 1)         // if not main, copy info over
      {
         // copy entire parameter structure first
         memcpy(&tiptr->TaskParam, tpptr, sizeof(TASK_PARAM));
         tiptr->TaskParam.TParamPriority = (U16)TASK_u32ConvertPriorityFromRTXC( tiptr->TaskParam.TParamPriority );
      }
      else                                   // main thread
      {
         tiptr->ThreadID = pthread_self();
      }

      pthread_mutex_init(&tiptr->TaskMutex, &our_mutexattr);
      pthread_condattr_init(&tiptr->TaskCondAttr);
      pthread_condattr_setclock(&tiptr->TaskCondAttr, CLOCK_MONOTONIC);
      pthread_cond_init(&tiptr->TaskCond, &tiptr->TaskCondAttr);
   }

   // setup the kickoff mutex and conditional
   pthread_mutex_init(&KickOffMutex, NULL);
   pthread_cond_init(&KickOffCond, NULL);

   // setup the exit procedure as a signal handler for SIGUSR2
   memset(&our_sigaction, 0, sizeof(our_sigaction));
   our_sigaction.sa_handler = &TASK_Exit;
   sigaction(SIGUSR2, &our_sigaction, NULL);
}

/****************************************************************
 * Function    TASK_GetPtr
 * Args:       1: TASK task number
 * Return:     pointer to task info structure (or NULL if invalid)
 * Purpose:    This function will check the task value.
 ****************************************************************/
PRIVATE TASK_INFO *TASK_GetPtr(TASK sTask)
{
   if (sTask == 0)                        // current task?
   {
      sTask = L_TASK_GetID(0);            // get task ID from thread ID
   }

   // less than one OR too high a number gets an error here
   if (sTask < 1 || sTask > TASK_COUNT)
   {
      return(NULL);
   }

   // make sure the array is initialized
   if (ptrTaskArray == NULL)
   {
      return(NULL);
   }

   return(&ptrTaskArray[sTask - 1]);
}

/****************************************************************
 * Function    L_TASK_GetID
 * Args:       1: pthread_t thread ID OR 0 to look-up current thread
 * Return:     TASK/ERC RC_TASK_INVALID       if not found OR
 *                      task number 1-n       if found
 * Purpose:    This function will locate a running thread in the task list.
 ****************************************************************/
GLOBAL TASK L_TASK_GetID(pthread_t ThreadID)
{
   TASK        x;
   TASK_INFO  *txptr;

   // if its "CURRENT THREAD" then get the current thread ID handy
   if (ThreadID == 0)                     // if current thread wanted,
   {
      ThreadID = pthread_self();          // then look up this one
   }
   // make sure the array is initialized
   if (ptrTaskArray == NULL)
   {
      return(TASK_COUNT);                 // return main thread here
   }
   // spin thru them all trying to find our thread ID
   for (x = 0, txptr = ptrTaskArray; x < TASK_COUNT; x++, txptr++)
   {
      if (txptr->ThreadID == ThreadID)
      {
         break;
      }
   }
   x++;                       // task ID is 1-based, not 0-based
   if (x > TASK_COUNT)        // never found the thread ID?
   {
      x = TASK_COUNT;         // oh ... then return the main thread
   }
   return(x);
}

/****************************************************************
 * Function    L_TASK_LockMutex
 * Args:       1: TASK Task ID 1-n
 * Return:     ERC RC_TASK_INVALID if invalid task number OR
 *                 RC_SUCCESS
 * Purpose:    This function locks the task's mutex
 ****************************************************************/
GLOBAL ERC L_TASK_LockMutex(TASK sTaskID)
{
   TASK_INFO  *tiptr;

   tiptr = TASK_GetPtr(sTaskID);             // get the task structure
   if (tiptr == NULL)
   {
      return(RC_TASK_INVALID);
   }
   if ( tiptr->ThreadID != 0 )
   {
      pthread_mutex_lock(&tiptr->TaskMutex);    // lock this thread down
   }
   return(RC_SUCCESS);
}

/****************************************************************
 * Function    L_TASK_UnlockMutex
 * Args:       1: TASK Task ID 1-n
 * Return:     ERC RC_TASK_INVALID if invalid task number OR
 *                 RC_SUCCESS
 * Purpose:    This function unlocks the task's mutex
 ****************************************************************/
GLOBAL ERC L_TASK_UnlockMutex(TASK sTaskID)
{
   TASK_INFO  *tiptr;

   tiptr = TASK_GetPtr(sTaskID);             // get the task structure
   if (tiptr == NULL)
   {
      return(RC_TASK_INVALID);
   }
   if ( tiptr->ThreadID != 0 )
   {
      pthread_mutex_unlock(&tiptr->TaskMutex);  // unlock this thread again
   }
   return(RC_SUCCESS);
}

/****************************************************************
 * Function    L_TASK_WaitConditional
 * Args:       1: TASK Task ID 1-n
 * Return:     ERC RC_TASK_INVALID if invalid task number OR
 *                 RC_SUCCESS
 * Purpose:    To await the task's conditional signal, without timeout
 * Note 1:     This function does NOT lock the task's mutex; but it should
 *             have been done so prior to calling here.
 * Note 2:     Returning from here with success does not always indicate
 *             that the conditional signal was received, due to potential
 *             overrun and race conditions.  This function should be
 *             placed inside a while() loop, as shown here:
 *                 while (condition_is_not_met)
 *                    L_TASK_WaitConditional(task);
 ****************************************************************/
GLOBAL ERC L_TASK_WaitConditional(TASK sTaskID)
{
   TASK_INFO *tiptr = TASK_GetPtr(sTaskID);             // get the task structure

//   printf("[%d]%s(%d): task %d\n", __LINE__, __FUNCTION__, L_TASK_GetID(0), sTaskID);

   if (tiptr == NULL)
   {
      return(RC_TASK_INVALID);
   }

   L_TASK_Deactivate();
//   printf("[%d]%s(%d): deactivate, mutex %d\n", __LINE__, __FUNCTION__, L_TASK_GetID(0), tiptr->TaskMutex);
   pthread_cond_wait(&tiptr->TaskCond, &tiptr->TaskMutex);
//   printf("[%d]%s(%d): activate\n", __LINE__, __FUNCTION__, L_TASK_GetID(0));
   L_TASK_Activate();
   return(RC_SUCCESS);
}

/****************************************************************
 * Function    L_TASK_WaitConditionalTimed
 * Args:       1: TASK Task ID 1-n
 *             2: timespec expiration timer (absolute time)
 * Return:     ERC RC_TASK_INVALID      if invalid task number
 *                 RC_TIMEOUT           if error from OS' wait function
 *                 RC_SUCCESS           if success from OS' wait function
 * Purpose:    To await the task's conditional signal, with timeout
 * Note 1:     This function does NOT lock the task's mutex; but it should
 *             have been done so prior to calling here.
 * Note 2:     Returning from here with success does not always indicate
 *             that the conditional signal was received, due to potential
 *             overrun and race conditions.  This function should be
 *             placed inside a while() loop, as shown here:
 *                 setup_timeout(expiry_ptr);
 *                 while (condition_is_not_met)
 *                    L_TASK_WaitConditionalTimed(task, expiry_ptr);
 ****************************************************************/
GLOBAL ERC L_TASK_WaitConditionalTimed(TASK sTaskID, struct timespec *ExpiryTimer)
{
   // note: we don't lock the mutex; you should have already done it
   int         x;
   TASK_INFO  *tiptr;

   tiptr = TASK_GetPtr(sTaskID);             // get the task structure
   if (tiptr == NULL)                        // not there?
   {
      return(RC_TASK_INVALID);
   }
   L_TASK_Deactivate();
   x = pthread_cond_timedwait(&tiptr->TaskCond, &tiptr->TaskMutex, ExpiryTimer);
   L_TASK_Activate();

   if (x == 0)
   {
      return(RC_SUCCESS);
   }
   return(RC_TIMEOUT);
}

/****************************************************************
 * Function    L_TASK_SignalConditional
 * Args:       1: TASK Task ID 1-n
 * Return:     ERC RC_TASK_INVALID if invalid task number OR
 *                 RC_SUCCESS
 * Purpose:    This function unlocks the task's mutex
 * Note:       The task's mutex is locked for the signal.  If it is already
 *             locked, that's okay, because we define it as recursive.
 ****************************************************************/
GLOBAL ERC L_TASK_SignalConditional(TASK sTaskID)
{
   // note: we lock the mutex for the signal; this is okay, since the
   // mutex is type recursive ("_XOPEN_SOURCE 500" and features.h)
   TASK_INFO  *tiptr;

   tiptr = TASK_GetPtr(sTaskID);             // get the task structure
   if (tiptr == NULL)
   {
      return(RC_TASK_INVALID);
   }
   if ( tiptr->ThreadID != 0 )
   {
      pthread_mutex_lock(&tiptr->TaskMutex);    // lock this thread down
      pthread_cond_signal(&tiptr->TaskCond);
      pthread_mutex_unlock(&tiptr->TaskMutex);  // unlock this thread again
   }
   return(RC_SUCCESS);
}

/****************************************************************
 * Function    L_TASK_Start
 * Args:       1: TASK Task ID 1-n
 * Return:     ERC RC_TASK_INVALID if invalid task number OR
 *                 RC_TASK_PARAMS  invalid stack size or priority
 *                 RC_TASK_MEMORY  could not malloc the stack
 *                 RC_TASK_START   if task fails to start
 *                 RC_SUCCESS      its going
 * Purpose:    Allocate the task's stack space, then start the task.
 *    Note:    Start-up priority in pthreads does not work.  As such,
 *             TASK_KickOff() signals the L_TASK_Start() once the new
 *             task's priority has been reset.
 ****************************************************************/
PRIVATE void *TASK_KickOff(void *vptr)
{
   TASK_INFO  *tiptr = (TASK_INFO *)vptr;

   L_TASK_Activate();                        // starting us up
   pthread_detach(pthread_self());           // detach from main thread

   // empirical evidence shows that the thread attr's priority is not
   // respected, even though the stack pointer and size are ... odd.
   L_TASK_ResetPriority();                   // reset this task's priority

// tmp: warning! the log_me() might cause a task switch!!!
// log_me("kicking off task ID %d", L_TASK_GetID(0));

   pthread_mutex_lock(&KickOffMutex);        // only one start-up at a time
   pthread_cond_signal(&KickOffCond);        // show we're up and running
   pthread_mutex_unlock(&KickOffMutex);

   tiptr->TaskParam.TParamStartFunction();   // call the start function now
   return(NULL);
}

GLOBAL ERC L_TASK_Start(TASK sTaskID)
{
   S32                  x;
   pthread_attr_t       pthreadAttr;
   struct sched_param   pthreadParam;
   TASK_INFO           *tiptr;

   tiptr = TASK_GetPtr(sTaskID);             // get the task structure
   if (tiptr == NULL)
   {
      return(RC_TASK_INVALID);
   }
   if (task_disabled[sTaskID - 1])      // give testers a way to
   {
#ifdef ENABLE_TASK_LOGS
      log_me("task ID %d %s skipped", sTaskID, get_task_name(sTaskID));
#endif
      return(RC_SUCCESS);
   }

//   log_me("[%d]%s: started task ID %d %s\n", __LINE__, __FUNCTION__, sTaskID, get_task_name(sTaskID));
   pthreadParam.sched_priority = tiptr->TaskParam.TParamPriority;

   // allocate the stack memory, if it has never been done before
   if (tiptr->TaskStackPtr == NULL)
   {
      tiptr->TaskStackPtr = malloc( tiptr->TaskParam.TParamStackSize + PTHREAD_STACK_MIN) ;       // we need this much
      if (tiptr->TaskStackPtr == NULL)
      {
         return(RC_TASK_MEMORY);
      }
   }

#ifdef USE_SCH_OTHER
   if ( pthreadParam.sched_priority == 0 )
   {
//log_me( "pmp starting priority 0" );
      if (pthread_attr_init(&pthreadAttr) != 0 ||
          pthread_attr_setschedpolicy(&pthreadAttr, SCHED_OTHER) != 0 ||
          pthread_attr_setschedparam(&pthreadAttr, &pthreadParam) != 0 ||
          pthread_attr_setstack(&pthreadAttr, tiptr->TaskStackPtr, tiptr->TaskParam.TParamStackSize + PTHREAD_STACK_MIN))
      {
//log_me( "pmp L_TASK_Start, priority = %d, error = %d, %s", pthreadParam.sched_priority, errno, strerror( errno ) );
         return(RC_TASK_PARAMS);
      }
   }
   else
#endif
   if (pthread_attr_init(&pthreadAttr) != 0 ||
       pthread_attr_setschedpolicy(&pthreadAttr, SCHED_FIFO) != 0 ||
       pthread_attr_setschedparam(&pthreadAttr, &pthreadParam) != 0 ||
       pthread_attr_setstack(&pthreadAttr, tiptr->TaskStackPtr, tiptr->TaskParam.TParamStackSize + PTHREAD_STACK_MIN))
   {
      return(RC_TASK_PARAMS);
   }

   pthread_mutex_lock(&KickOffMutex);        // only one start-up at a time
   x = pthread_create(&tiptr->ThreadID, &pthreadAttr, &TASK_KickOff, tiptr);
   pthread_setname_np(tiptr->ThreadID, get_task_name(sTaskID));
   pthread_attr_destroy( &pthreadAttr );

   if (x == 0)                               // started?  await a signal
   {
      L_TASK_Deactivate();
      pthread_cond_wait(&KickOffCond, &KickOffMutex);
      L_TASK_Activate();
   }
   pthread_mutex_unlock(&KickOffMutex);

   if (x != 0)                               // never started?
   {
      return(RC_TASK_START);                 // then show the error
   }

//#ifdef ENABLE_TASK_LOGS
   log_me("pthread_create: success task ID %d %s thread ID %d mutex %d",
          sTaskID, get_task_name(sTaskID), tiptr->ThreadID, tiptr->TaskMutex);
//#endif

   return(RC_SUCCESS);
}

/****************************************************************
 * Function    L_TASK_GetPriority
 * Args:       1: TASK Task ID 1-n
 * Return:     PRIORITY/ERC RC_TASK_INVALID if invalid task number OR
 *                          priority 1-n where 1 is higher priority
 ****************************************************************/
GLOBAL ERC L_TASK_GetPriority(TASK sTaskID)
{
   TASK_INFO  *tiptr;

   tiptr = TASK_GetPtr(sTaskID);             // get the task structure
   if (tiptr == NULL)
   {
      return(RC_TASK_INVALID);
   }
   return( TASK_u32ConvertPriorityToRTXC( tiptr->TaskParam.TParamPriority ) );
}

/****************************************************************
 * Function    L_TASK_SetPriority
 * Args:       1: TASK Task ID 1-n
 *             2: PRIORITY priority to set (where 1 is a higher priority)
 * Return:     ERC RC_TASK_INVALID if invalid task number OR
 *                 RC_TASK_PARAMS  invalid priority OR
 *                 RC_SUCCESS      its going
 ****************************************************************/
GLOBAL ERC L_TASK_SetPriority(TASK sTaskID, PRIORITY sPriority)
{
   TASK_INFO  *tiptr;

   tiptr = TASK_GetPtr(sTaskID);             // get the task structure
   if (tiptr == NULL)
   {
      return(RC_TASK_INVALID);
   }

   // 0 means he's resetting the current one; non-zero means set his
   if (sPriority != 0)                       // he's setting one?
   {
      tiptr->TaskParam.TParamPriority = (U16)TASK_u32ConvertPriorityFromRTXC(sPriority);
   }

   // call the OS to set the priority if this task is running already
   if (tiptr->ThreadID != 0)
   {
      struct sched_param   pthreadParam;

      pthreadParam.sched_priority = tiptr->TaskParam.TParamPriority;
#ifdef USE_SCH_OTHER
      if ( tiptr->TaskParam.TParamPriority == 0 )
      {
         if (pthread_setschedparam(tiptr->ThreadID, SCHED_OTHER, &pthreadParam) != 0)
         {
            return(RC_TASK_PARAMS);
         }
      }
      else
#endif
      if (pthread_setschedparam(tiptr->ThreadID, SCHED_FIFO, &pthreadParam) != 0)
      {
         return(RC_TASK_PARAMS);
      }
#ifdef ENABLE_TASK_LOGS
      log_me("new priority for task %s is %d",
             get_task_name((tiptr-ptrTaskArray)+1), tiptr->TaskParam.TParamPriority);
#endif
   }
   return(RC_SUCCESS);
}

/****************************************************************
 * Function    L_TASK_Terminate
 * Args:       1: TASK Task ID 1-n
 * Return:     ERC RC_TASK_INVALID if invalid task number OR
 *                 RC_SUCCESS      terminate issued
 * Note:       If the task being terminated is the current task, then
 *             this function does not return.
 * Note 2:     This function does not kill the main thread.
 ****************************************************************/
GLOBAL ERC L_TASK_Terminate(TASK sTaskID)
{
   TASK_INFO  *tiptr;
   pthread_t   kill_me;

   tiptr = TASK_GetPtr(sTaskID);             // get the task structure
   if (tiptr == NULL)
   {
      return(RC_TASK_INVALID);
   }

   kill_me = tiptr->ThreadID;                // this one is going away
   tiptr->ThreadID = 0;                      // so clear it out
   if (kill_me != 0)                         // never actually kill main
   {
      pthread_kill(kill_me, SIGUSR2);        // see handler TASK_Exit()
   }
   if (kill_me == pthread_self())            // killed ourselves?
   {
      while (1)                              // soon, it should all be over
      {
         sleep(60);
      }
   }
   return(RC_SUCCESS);                       // he's dead, Jim.
}

/****************************************************************
 * Function    L_TASK_Activate
 * Args:       none
 * Return:     none
 * Purpose:    This function starts the accumlated timer.  Call
 *             here when your task is awoken.
 ****************************************************************/
GLOBAL void L_TASK_Activate(void)
{
   TASK_INFO  *tiptr = TASK_GetPtr(0);

   tiptr->active_count++;
   lm_gettime(&tiptr->activ_time);  // what time is it?  (4:30?)
}

/****************************************************************
 * Function    L_TASK_Deactivate
 * Args:       none
 * Return:     none
 * Purpose:    This function stops the accumulated timer.
 ****************************************************************/
PRIVATE void td_doit(TASK_INFO *tiptr, struct timeval *atvptr)
{
   struct timeval    now;

   lm_gettime(&now);                // what time is it?  (4:30?)
   now.tv_usec -= tiptr->activ_time.tv_usec;
   if (now.tv_usec < 0)
   {
      now.tv_usec += 1000000;
      now.tv_sec--;
   }
   now.tv_sec -= tiptr->activ_time.tv_sec;

   atvptr->tv_usec += now.tv_usec;
   if (atvptr->tv_usec >= 1000000)
   {
      atvptr->tv_usec -= 1000000;
      atvptr->tv_sec++;
   }
   atvptr->tv_sec += now.tv_sec;
}

GLOBAL void L_TASK_Deactivate(void)
{
   TASK_INFO  *tiptr = TASK_GetPtr(0);

   td_doit(tiptr, &tiptr->accum_time);
   tiptr->active_count--;
}

/****************************************************************
 * Function    L_TASK_Debug
 * Args:       none
 * Return:     none
 * Purpose:    This function logs information.
 ****************************************************************/
GLOBAL void L_TASK_Debug(void)
{
   int               x;
   TASK_INFO        *siptr;
   int               ac_temp;
   struct timeval    now;
   struct timeval    tv_total;
   struct timeval    tv_stats[TASK_COUNT];
   int               total_millis;

   // first, complete the tallying of run-times
   memset(&tv_total, 0, sizeof(tv_total));
   for (x = 0, siptr = ptrTaskArray; x < TASK_COUNT; x++, siptr++)
   {
      memcpy(&tv_stats[x], &siptr->accum_time, sizeof(struct timeval));
      for (ac_temp = 0; ac_temp < siptr->active_count; ac_temp++)
      {
         td_doit(siptr, &tv_stats[x]);
      }
      tv_total.tv_usec += tv_stats[x].tv_usec;
      if (tv_total.tv_usec >= 1000000)
      {
         tv_total.tv_usec -= 1000000;
         tv_total.tv_sec++;
      }
      tv_total.tv_sec += tv_stats[x].tv_sec;
   }
   lm_gettime(&now);
   log_me("run time %d.%06d accumulated time %d.%06d",
         now.tv_sec, now.tv_usec, tv_total.tv_sec, tv_total.tv_usec);
   total_millis = (tv_total.tv_sec * 1000) + (tv_total.tv_usec / 1000);

   log_me("task information (%d total)", (int)(endTaskArray - ptrTaskArray));
   for (siptr = ptrTaskArray, x = 0; siptr != endTaskArray; siptr++, x++)
   {
      int   local_millis = (tv_stats[x].tv_sec * 1000) + (tv_stats[x].tv_usec / 1000);
      int   local_percent = -1;

      if (total_millis != 0)
      {
         local_percent = (local_millis * 100) / total_millis;
      }
      log_me("  %02d %s: active %d time (%d%) %d.%06d priority %d pthread ID #%d mutex.lock %d", x + 1,
      get_task_name(x + 1), siptr->active_count, local_percent,
      tv_stats[x].tv_sec, tv_stats[x].tv_usec,
      siptr->TaskParam.TParamPriority, siptr->ThreadID, siptr->TaskMutex.__data.__lock);
   }
}


