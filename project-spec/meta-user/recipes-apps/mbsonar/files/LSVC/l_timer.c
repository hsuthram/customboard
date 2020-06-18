/*******************************************************************************
*        Copyright (c) 2010 Johnson Outdoors Marine Electronics, Inc.          *
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
//#define _XOPEN_SOURCE 500

/********************           INCLUDE FILES                ******************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <features.h>
#include <time.h>
#define _TIME_T
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "types.h"
#include "lsvc.h"
#include "l_timer.h"
#include "l_clock.h"
#include "l_utils.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef enum
{
   TMRST_IDLE = 0,
   TMRST_RUNNING,
   TMRST_CYCLING,
   TMRST_COUNT
} TIMER_STATE;
/********************        FUNCTION PROTOTYPES             ******************/

/********************           LOCAL VARIABLES              ******************/
PRIVATE TIMER_INFO *ptrTimerArray = NULL;
PRIVATE TIMER_INFO *endTimerArray = NULL;

PRIVATE pthread_mutex_t TimerMutex = PTHREAD_MUTEX_INITIALIZER;

PRIVATE S32 timer_pipe_handles[2];
#define timer_read_handle  timer_pipe_handles[0]
#define timer_write_handle timer_pipe_handles[1]

pthread_t timer_thread_id = 0;

U32 timer_fired_count = 0;
U32 timer_queue_count = 0;
/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/

/****************************************************************
 * Function    TIMER_Thread
 * Args:       void * ignored
 * Return:     void * NULL
 * Purpose:    A thread for reading the pipe and processing expiry
 * Note:       The u8TimerSignal flag indicates that the timer is
 *             in the pipe awaiting service from this thread.  This
 *             prevents a blocked pipe from an overly-zealous timer.
 *             The signal handler will never get interrupted; but it
 *             might interrupt this function.  So, this function clears
 *             the timer queued flag before firing the semaphore, which
 *             insures that there will never be a missed timer.
 ****************************************************************/
PRIVATE void *TIMER_Thread(void *vptr)
{
   int         xres;
   TIMER_INFO *tiptr;
   struct      sched_param pthreadParam;

   // always make sure the pipe is empty by max'ing our priority
   pthreadParam.sched_priority = 98;
   pthread_setschedparam(pthread_self(), SCHED_FIFO, &pthreadParam);
   pthread_detach(pthread_self());           // detach from main thread

   goto TT_start;

   do
   {
      // do this before processing the semaphore!  (see note above)
      tiptr->u8TimerSignal = 0;              // allow another timer to happen

      // make sure this timer is still active and has an owner
      if (tiptr->u8TimerState != TMRST_IDLE)
      {
         if (tiptr->u8TimerState == TMRST_RUNNING)
         {
            tiptr->u8TimerState = TMRST_IDLE;
         }
         L_SEMA_Signal(tiptr->Semaphore);    // signal the semaphore
      }
TT_start:
      do
      {
         xres = read(timer_read_handle, &tiptr, sizeof(tiptr));
      } while (xres == -1 && errno == 4);
   } while (xres > 0);
   return(NULL);
}

/****************************************************************
 * Function    TIMER_SignalHandler
 * Args:       int signal number
 *             siginfo_t *information about the signal
 *             void * ignored
 * Return:     nothing
 * Purpose:    A thread for reading the pipe and processing expiry
 * Note:       See Note at TIMER_Thread()
 ****************************************************************/
PRIVATE void TIMER_SignalHandler(int signo, siginfo_t *info, void *vptr)
{
   TIMER_INFO *tiptr;

   if (info->si_code == SI_TIMER)         // only if a timer expires
   {
      tiptr = info->si_ptr;               // this timer expired
      timer_fired_count++;
      tiptr->timer_fired_count++;
      if (tiptr->u8TimerSignal++ == 0)    // if the timer is not queued,
      {
         timer_queue_count++;
         tiptr->timer_queue_count++;
         write(timer_write_handle, &tiptr, sizeof(tiptr))
            ;
      }
   }
}

/****************************************************************
 * Function    TIMER_Init
 * Args:       none
 * Return:     -none-
 * Purpose:    This function will allocate and initialize the
 *             timer structure array.
 ****************************************************************/
GLOBAL void TIMER_Init(void)
{
   TIMER_INFO       *tiptr;
   struct sigaction  our_sigusr2;

   ptrTimerArray = calloc(ntmrs, sizeof(TIMER_INFO));
   if (ptrTimerArray != NULL)          // failure unlikely; check anyway
   {
      // create a pipe for the signal handler to trigger the timer thread
      if (pipe(timer_pipe_handles) == 0)
      {
         // create timer thread for processing the timer expiry
         timer_thread_id = L_UTILS_threadCreateSimpleThread( "timer", &TIMER_Thread, NULL );
         if ( timer_thread_id != 0 )
         {
		      // point the timer expiry signal handler at us
		      memset(&our_sigusr2, 0, sizeof(our_sigusr2));
		      our_sigusr2.sa_flags = SA_SIGINFO;
		      our_sigusr2.sa_sigaction = &TIMER_SignalHandler;
		      sigaction(SIGUSR1, &our_sigusr2, NULL);

		      endTimerArray = &ptrTimerArray[ntmrs];

		      // now, spin through them all initializing the structures
		      for (tiptr = ptrTimerArray; tiptr != endTimerArray; tiptr++)
		      {
			      L_TIMER_InitTimer(tiptr, 0);
		      }
            return;
         }
      }
      free(ptrTimerArray);
   }
}

/****************************************************************
 * Function    L_TIMER_InitTimer
 * Args:       1: pointer to TIMER_INFO
 *             2: TASK owner's ID
 * Return:     ERC RC_SUCCESS     if successful
 *                 RC_NO_TIMER    if OS error creating the timer
 * Purpose:    This function will initialize one timer structure.
 * Note:       This exists as a public because timer structures do
 *             not have to be part of the array.
 * Note 2:     Call this only once for each structure or you risk
 *             (actually, you cause) a timer leak in the OS.
 ****************************************************************/
GLOBAL ERC L_TIMER_InitTimer(TIMER_INFO *tiptr, TASK OwnerTaskID)
{
   ERC         tcres;
   sigevent_t  mct_sig;

   // clear it out and save the owner's task number
   memset(tiptr, 0, sizeof(TIMER_INFO));
   tiptr->OwningTaskID = OwnerTaskID;

   // save pointer to timer control block for later access
   memset(&mct_sig, 0, sizeof(mct_sig));
   mct_sig.sigev_value.sival_ptr = tiptr;

   // setup notify type as signal
   mct_sig.sigev_notify = SIGEV_SIGNAL;         // notify via signal
   mct_sig.sigev_signo = SIGUSR1;               // specifically, this signal

   // create the timer now
   tiptr->TimerID = 0;
   tcres = timer_create(CLOCK_REALTIME, &mct_sig, &tiptr->TimerID);
   if (tcres != 0)
   {
      tcres = RC_NO_TIMER;
   }
   return(tcres);
}

/****************************************************************
 * Function    L_TIMER_Allocate
 * Args:       none
 * Return:     TIMER_INFO pointer to the new timer block OR
 *                                NULL if one is not available
 * Purpose:    This function allocates one timer structure out of
 *             the free pool of timers.  The calling task ID is
 *             stored in the timer structure, thereby making it
 *             unusable by other tasks.
 ****************************************************************/
GLOBAL TIMER_INFO *L_TIMER_Allocate(void)
{
   TIMER_INFO *tiptr;

   pthread_mutex_lock(&TimerMutex);       // lock it down

   // note that the following code works even if the timer array init failed

   // find a timer info structure in the array that is unused
   for (tiptr = ptrTimerArray; tiptr != endTimerArray; tiptr++)
   {
      if (tiptr->OwningTaskID == 0)
      {
         break;
      }
   }

   if (tiptr == endTimerArray)            // no available timer blocks?
   {
      tiptr = NULL;                       // use NULL to show "not found"
   }
   else
   {
      // take over this timer block
      tiptr->OwningTaskID = L_TASK_GetID(0);
      tiptr->u8TimerState = TMRST_IDLE;
      tiptr->Semaphore = 0;
   }

   pthread_mutex_unlock(&TimerMutex);     // remove the lock now
   return(tiptr);
}

/****************************************************************
 * Function    L_TIMER_Release
 * Args:       TIMER_INFO pointer to the timer info structure to be released
 * Return:     ERC RC_SUCCESS         if released okay OR
 *                 RC_TIMER_ILLEGAL   if calling task does not own the timer
 *                 RC_TIMER_UNINIT    if initialization not performed
 * Purpose:    This function allocates one timer structure out of
 *             the free pool of timers.  The calling task ID is
 *             stored in the timer structure, thereby making it
 *             unusable by other tasks.
 * Note:       Because of existing Matrix code, NULL is allowed as arg #1.
 ****************************************************************/
GLOBAL ERC L_TIMER_Release(TIMER_INFO *tiptr)
{
   // initialize if needed; split if that fails
   if (ptrTimerArray == NULL)
   {
      return(RC_TIMER_UNINIT);
   }

   if (tiptr == NULL || tiptr->OwningTaskID != L_TASK_GetID(0))
   {
      return(RC_TIMER_ILLEGAL);
   }

   L_TIMER_Stop(tiptr);                   // be sure it is stopped
   // note: removing the lock here, since clearing the owning Task
   // ID should not present a problem, no matter when it is done.
   // (plus, we know the only task to get here is the owning task)
   //pthread_mutex_lock(&TimerMutex);
   tiptr->OwningTaskID = 0;
   //pthread_mutex_unlock(&TimerMutex);
   return(RC_SUCCESS);
}

/****************************************************************
 * Function    L_TIMER_Start
 * Args:       1: TIMER_INFO pointer to the timer to start
 *             2: TICKS number of ticks for initial interval
 *             3: TICKS number of ticks for periodic interval
 *             4: SEMA 1-n semaphore number to associate with this timer
 *                     0 don't associate a semaphore with this timer
 *                     -1 use the semaphore number from the timer block
 * Return:     TIMER_INFO pointer to the new timer block OR
 *                                NULL if one is not available OR
 *                                NULL if timer is owned by another task
 * Purpose:    This function starts a timer ticking, with the option of
 *             it being a one-shot or cyclic, and the additional option
 *             of having a semaphore tied into this timer.
 ****************************************************************/
// little helper function
PRIVATE void TIMER_tsDoit(TIMER_INFO *tiptr, TICKS tStart, TICKS tRecur)
{
   struct itimerspec itset;

   memset(&itset, 0, sizeof(itset));
   L_CLOCK_StoreTimerspec(&itset, tStart, tRecur);
   tiptr->u8TimerState = (tRecur > 0) ? TMRST_CYCLING : TMRST_RUNNING;
   timer_settime(tiptr->TimerID, 0, &itset, NULL);
}

GLOBAL TIMER_INFO *L_TIMER_Start(TIMER_INFO *tiptr, TICKS tStart, TICKS tRecur, SEMA sSemaphore)
{
   if (tiptr == NULL)
   {
      tiptr = L_TIMER_Allocate();
      if (tiptr == NULL)
      {
         return(tiptr);
      }
   }
   if (tiptr->OwningTaskID != L_TASK_GetID(0))
   {
      return(NULL);
   }
   L_TIMER_Stop(tiptr);                   // stop it first
   if (sSemaphore > -1)                   // if the semaphore was defined,
   {
      tiptr->Semaphore = sSemaphore;      // save it too
   }
   if (tiptr->Semaphore > 0)              // if a semaphore, set it pending
   {
      L_SEMA_SetPending(tiptr->Semaphore);
   }
   if (tStart > 0)                        // if an initial timer,
   {
      TIMER_tsDoit(tiptr, tStart, tRecur);// kick-off the timer now
   }
   else                                   // if no start value,
   {
      if (tiptr->Semaphore > 0)           // auto-fire the semaphore
      {
         L_SEMA_Signal(tiptr->Semaphore);
      }
      if (tRecur > 0)                     // if a recurring interval,
      {
         TIMER_tsDoit(tiptr, tRecur, tRecur);
      }
   }
   return(tiptr);
}

/****************************************************************
 * Function    L_TIMER_Stop
 * Args:       1: TIMER_INFO pointer to the timer to start
 * Return:     ERC RC_SUCCESS         if released okay OR
 *                 RC_TIMER_ILLEGAL   if calling task does not own the timer
 *                 RC_TIMER_INACTIVE  if timer was not running
 * Purpose:    Stop the timer.
 * Note:       Because of existing Matrix code, NULL is allowed as arg #1.
 ****************************************************************/
GLOBAL ERC L_TIMER_Stop(TIMER_INFO *tiptr)
{
   if (tiptr == NULL || tiptr->OwningTaskID != L_TASK_GetID(0))
   {
      return(RC_TIMER_ILLEGAL);
   }
   if (tiptr->u8TimerState == TMRST_IDLE)          // is it running?
   {
      return(RC_TIMER_INACTIVE);                   // nope -- error out
   }
   TIMER_tsDoit(tiptr, 0, 0);                      // stop the running timer
   tiptr->u8TimerState = TMRST_IDLE;

   return(RC_SUCCESS);
}

/****************************************************************
 * Function    L_TIMER_GetRemaining
 * Args:       1: TIMER_INFO pointer to the timer to start
 * Return:     TICKS number of ticks until expiration (0 if not running)
 * Purpose:    Check the time remaining on a [running] timer.
 * Note:       Because of existing Matrix code, NULL is allowed as arg #1.
 ****************************************************************/
GLOBAL TICKS L_TIMER_GetRemaining(TIMER_INFO *tiptr)
{
   struct itimerspec itget;

   // make sure this is a running timer first
   if (tiptr == NULL || tiptr->u8TimerState == TMRST_IDLE)
   {
      return(0);
   }
   // get the remaining time on the timer
   timer_gettime(tiptr->TimerID, &itget);

   return(L_CLOCK_ScanTimerspec(&itget));
}

/****************************************************************
 * Function    L_TIMER_GetElapsed
 * Args:       1: pointer to TICKS
 * Return:     1: TICKS number of ticks since last time called
 *             2: value at TICKS pointer updated to current tick time
 * Purpose:    Perform delta timing.
 ****************************************************************/
GLOBAL TICKS L_TIMER_GetElapsed(TICKS *ptrTicks)
{
   TICKS xvalue, xdelta;

   xvalue = L_CLOCK_GetTimeInTicks();
   xdelta = xvalue - *ptrTicks;
   *ptrTicks = xvalue;
   return(xdelta);
}

/****************************************************************
 * Function    L_TIMER_Debug
 * Args:       none
 * Return:     none
 * Purpose:    This function logs information.
 ****************************************************************/
GLOBAL void L_TIMER_Debug(void)
{
   int            x = 1;
   TIMER_INFO    *siptr;
   static char   *strs[] = {"idle", "running", "cycling"};

   log_me("timer information (%d total %d fired %d queued)", (int)(endTimerArray - ptrTimerArray), timer_fired_count, timer_queue_count);
   for (siptr = ptrTimerArray; siptr != endTimerArray; siptr++, x++)
   {
      log_me("  %02d: %s %squeued owner task #%d (%s) semaphore #%d (%s) fired %d queued %d", x,
            strs[siptr->u8TimerState],
            (siptr->u8TimerSignal != 0) ? "" : "not ",
            siptr->OwningTaskID, get_task_name(siptr->OwningTaskID),
            siptr->Semaphore, get_semaphore_name(siptr->Semaphore),
            siptr->timer_fired_count, siptr->timer_queue_count);
   }
}


