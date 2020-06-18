/*******************************************************************************
*        Copyright (c) 2009 - 2010 Johnson Outdoors Marine Electronics, Inc.   *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description:  Some task based utility routines

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
#include <stdarg.h>
#include <pthread.h>
#include <bits/local_lim.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "types.h"
#include "lsvc.h"
//#include "global.h"
#include "l_task.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define  SIMPLE_THREAD_STACK_LENGTH       (PTHREAD_STACK_MIN+(8*1024))
/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/

/********************        FUNCTION PROTOTYPES             ******************/
extern void L_TASK_Debug(void);
extern void L_TIMER_Debug(void);
extern void L_SEMA_Debug(void);
extern void L_QUEUE_Debug(void);
extern void L_MUTEX_Debug(void);
extern void L_MBOX_Debug(void);
//extern void L_MEM_Debug(void);
extern char model_string[];
/********************           LOCAL VARIABLES              ******************/
PRIVATE int          last_time = 0;
PRIVATE pthread_t    debug_thread_id = 0;
PRIVATE FILE        *output_handle = NULL;
PRIVATE U32          debug_taskno = 0;
PRIVATE const char  *str_invalid = "INVALID";
/********************          GLOBAL VARIABLES              ******************/
GLOBAL U64     quit_seconds = 0;          // quit after running for x seconds
GLOBAL U64     opt_sleeper = 1000;        // throttle
GLOBAL U32     debug_seconds = 0;         // dump debug info after x seconds
GLOBAL U32     opt_fakekey = -1;          // spew this key
GLOBAL BOOLEAN opt_nolog = FALSE;         // suppress log writes
GLOBAL BOOLEAN opt_noadc = FALSE;         // suppress ADC value gathering
GLOBAL BOOLEAN opt_fake_transducer = FALSE;
GLOBAL BOOLEAN opt_fsync = FALSE;
GLOBAL BOOLEAN opt_nodisclaimer = FALSE;
GLOBAL BOOLEAN opt_notone = FALSE;

//extern -BOOLEAN task_disabled[64];
extern BOOLEAN task_disabled[NTASKS];
/********************              FUNCTIONS                 ******************/

/******************************************************************************
 *
 *    Function: lm_gettime
 *
 *    Args:    pointer to a timeval structure or NULL
 *
 *    Return:  time since last call in msec
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void CLOCK_internalGetTimespec(struct timespec *tsptr);

GLOBAL int lm_gettime(struct timeval *tvptr)
{
   struct timespec          tv_now;

   CLOCK_internalGetTimespec(&tv_now);

   if (NULL != tvptr)
   {
      tvptr->tv_sec = tv_now.tv_sec;
      tvptr->tv_usec = tv_now.tv_nsec / 1000;
   }
   return((tv_now.tv_sec * 1000) + (tv_now.tv_nsec / 1000000));
}

/******************************************************************************
 *
 *    Function: debug_doit
 *
 *    Args:    -none-
 *
 *    Return:  -none-
 *
 *    Purpose: Run the sub-system debug code
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void debug_doit(void)
{
   output_handle = stderr;
   debug_taskno = L_TASK_GetID(0);     // get the task ID handy
   L_TASK_Debug();                     // output debug information
   L_TIMER_Debug();
   L_SEMA_Debug();
   L_QUEUE_Debug();
   L_MUTEX_Debug();
   L_MBOX_Debug();
//   L_MEM_Debug();
}

/******************************************************************************
 *
 *    Function: lm_debug
 *
 *    Args:    unused void pointer
 *
 *    Return:  NULL
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void *lm_debug(void *vptr)
{
   int      prev_time;
   int      debugged = 0;
   int      xleft, xstop;
   int      xcount = 0;
   struct   sched_param pthreadParam;

   pthreadParam.sched_priority = 98;
   pthread_setschedparam(pthread_self(), SCHED_FIFO, &pthreadParam);

   pthread_detach(pthread_self());           // detach from main thread

   while(1)
   {
      prev_time = last_time;
      xleft = debug_seconds;
      do
      {
         do
         {
            xstop = sleep(xleft);
         } while (xstop == xleft);
         xleft = xstop;
         fprintf(stderr, "%d: after sleep %d remaining (last=%d prev=%d)\n", xcount++, xleft, last_time, prev_time);
      } while (xleft > 0);
      if (debugged == 0 && prev_time == last_time)
      {
         debug_doit();
         debugged++;
         program_exit(3);
      }
   }
   return(NULL);
}

/******************************************************************************
 *
 *    Function: sigterm_handler
 *
 *    Args:
 *
 *    Return:  -none-
 *
 *    Purpose: termination signal handler
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void sigterm_handler(int signo, siginfo_t *info, void *vptr)
{
   if (signo == SIGINT)
   {
      debug_doit();
   }
   program_exit(2);
}

/******************************************************************************
 *
 *    Function: take_handler
 *
 *    Args:    signal number
 *
 *    Return:  -none-
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void take_handler(int signo)
{
   struct sigaction our_sig;

   memset(&our_sig, 0, sizeof(our_sig));
   our_sig.sa_flags = SA_SIGINFO;
   our_sig.sa_sigaction = &sigterm_handler;
   sigaction(signo, &our_sig, NULL);
}

/******************************************************************************
 *
 *    Function: lm_doit
 *
 *    Args:    string pointer
 *
 *    Return:  -none-
 *
 *    Purpose: timestamp and dump a string
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void lm_doit(char *buffer)
{
   int lmg, dta, taskno;

   lmg = lm_gettime(NULL);    // get time into here
   dta = lmg - last_time;     // get delta time available

   taskno = L_TASK_GetID(0);  // get the task ID handy

   fprintf(output_handle, "%5d.%03d %5d.%03d %2d %s %s\r\n",
      dta / 1000, dta % 1000,
      lmg / 1000, lmg % 1000,
      taskno, str_tasks[taskno], buffer);

   last_time = lmg;           // save this time for next message's delta
   fflush(output_handle);     // gets rid of user buffers

   if (opt_fsync != FALSE)    // if fsync requested
   {
      fsync(STDOUT_FILENO);
   }
}

/******************************************************************************
 *
 *    Function: log_me
 *
 *    Args:    like printf
 *
 *    Return:  -none-
 *
 *    Purpose: dump a log message
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void log_me(char *msg, ...)
{
   if (debug_taskno != 0 &&               // shutting down?
       debug_taskno != L_TASK_GetID(0))   // and not by our task?
   {
      sleep(60);                          // just await the shutdown
   }
   if (output_handle == NULL)
   {
      take_handler(SIGTERM);        // TERM will end without debug info
      take_handler(SIGINT);         // INT will end with debug info
      output_handle = stdout;
   }
   if (opt_nolog == FALSE)
   {
      char        buffer[256];
      va_list     args;

      va_start (args, msg);
      vsprintf (buffer, msg, args);
      va_end (args);
      lm_doit(buffer);              // show this message
   }
   else
   {
      last_time = lm_gettime(NULL);
   }
   if (output_handle != stderr &&      // only once
       quit_seconds != 0 &&            // quitting at a certain time?
       last_time >= quit_seconds)
   {
      debug_doit();
      lm_doit("quit seconds expired");
      program_exit(4);
   }

   if (debug_seconds != 0 && debug_thread_id == 0)
   {
      pthread_create(&debug_thread_id, NULL, &lm_debug, NULL);
   }
}

/******************************************************************************
 *
 *    Function: bail_out
 *
 *    Args:    like printf
 *
 *    Return:  -none-
 *
 *    Purpose: dump a message and exit the application
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void bail_out(char *msg, ...)
{
   char     buffer[256];
   va_list  args;

   va_start (args, msg);
   vsprintf (buffer, msg, args);
   va_end (args);
   lm_doit(buffer);
   program_exit(1);
}

/******************************************************************************
 *
 *    Function: itoa
 *
 *    Args:    value, output pointer, radix
 *
 *    Return:  -none-
 *
 *    Purpose: integer to ASCII conversion routine
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void itoa(int value, char *optr, int radix)
{
   int   x;
   char  xbuf[16];
   char *xptr = &xbuf[15];

   *xptr-- = 0;            // make sure a null ends it

   if (value < 0)          // less than zero?
   {
      *optr++ = '-';       // show its negative
      value = 0 - value;   // but we work with positive
   }
   do
   {
      x = value % radix;   // next digit
      x += '0';            // up to ASCII
      if (x > '9')         // not a digit, per se?
      {
         x += 7;           // bump to A-Z
      }
      *xptr-- = x;         // stuff in this char
      value /= radix;      // slide the value down
   } while (value != 0);   // while still something to report

   do
   {
      *optr++ = *++xptr;
   } while (*xptr != 0);   // until the NULL is moved
}

/******************************************************************************
 *
 *    Function: atoh
 *
 *    Args:    string pointer
 *
 *    Return:  converted value
 *
 *    Purpose: ASCII to hex conversion routine
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL long atoh(char *cptr)
{
   long val = 0;
   char ch;

   while (*cptr != 0)
   {
      ch = *cptr++;                 // get [next] char

      if (ch >= 'a' && ch <= 'z')   // lower case?
      {
         ch -= 0x20;                // make it upper case
      }
      if (ch < '0' || ch > 'Z' || (ch > '9' && ch < 'A'))
      {
         break;
      }
      ch -= '0';
      if (ch > 9)
      {
         ch -= 7;
      }
      val <<= 4;
      val += ch;
   }
   return(val);
}

/******************************************************************************
 *
 *    Function: show_linux_priority
 *
 *    Args:    string pointer
 *
 *    Return:  -none-
 *
 *    Purpose:
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void show_linux_priority(const char *cptr)
{
   int                  pthreadInt;
   struct sched_param   pthreadParam;

   {
      static int     xcount = 0;

      if (xcount++ == 0)
      {
         log_me("sched priorities min %d max %d",
         sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO));
      }
   }
   if (pthread_getschedparam(pthread_self(), &pthreadInt, &pthreadParam) != 0)
   {
      log_me("could not get priority for \"%s\"", cptr);
   }
   else
   {
      log_me("priority for \"%s\" is %d policy %d", cptr, pthreadParam.sched_priority, pthreadInt);
   }
}

/******************************************************************************
 *
 *    Function: rtxcbug
 *
 *    Args:    -none-
 *
 *    Return:  -none-
 *
 *    Purpose: cover an unused function
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void rtxcbug(void)
{
}

/******************************************************************************
 *
 *    Function: lm_exit
 *
 *    Args:
 *
 *    Return:  -none-
 *
 *    Purpose: exit with a message
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void lm_exit(char *fname, int lineno, int status)
{
   log_me("exit from file %s line #%d status %d", fname, lineno, status);
   exit(status);
}

/******************************************************************************
 *
 *    Function: get_task_name
 *
 *    Args:    task numbet
 *
 *    Return:  pointer to task name
 *
 *    Purpose: fetch task name
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL const char *get_task_name(U32 value)
{
   // we allow one extra task for MAINTHREAD
   if (value < 0 || value > (ntasks + 1))
   {
      return(str_invalid);
   }
   return(str_tasks[value]);
}

/******************************************************************************
 *
 *    Function: get_semaphore_name
 *
 *    Args:    semaphore number
 *
 *    Return:  pointer to semaphore name
 *
 *    Purpose: fetch semaphore name
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL const char *get_semaphore_name(U32 value)
{
   if (value < 0 || value > nsemas)
   {
      if ( value == RC_TIMEOUT )
      {
         return( "RC_TIMEOUT" );
      }
      return(str_invalid);
   }
   return(str_semaphores[value]);
}

/******************************************************************************
 *
 *    Function: get_queue_name
 *
 *    Args:    Queue number
 *
 *    Return:  pointer to queue name
 *
 *    Purpose: fetch queue name
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL const char *get_queue_name(U32 value)
{
   if (value < 0 || value > nqueues)
   {
      return(str_invalid);
   }
   return(str_queues[value]);
}

/******************************************************************************
 *
 *    Function: get_mutex_name
 *
 *    Args:    mutex number
 *
 *    Return:  pointer to mutex name
 *
 *    Purpose: fetch mutex name
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL const char *get_mutex_name(U32 value)
{
   if (value < 0 || value > nres)
   {
      return(str_invalid);
   }
   return(str_mutexes[value]);
}

/******************************************************************************
 *
 *    Function: get_mailbox_name
 *
 *    Args:    mailbox number
 *
 *    Return:  pointer to mailbox name
 *
 *    Purpose: fetch mailbox name
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL const char *get_mailbox_name(U32 value)
{
   if (value < 0 || value > nmboxes)
   {
      return(str_invalid);
   }
   return(str_mailboxes[value]);
}

/******************************************************************************
 *
 *    Function: get_memory_name
 *
 *    Args:    memory partition number
 *
 *    Return:  pointer to memory partition name
 *
 *    Purpose: fetch memory partition name
 *
 *    Notes:
 *
 ******************************************************************************/
/*GLOBAL const char *get_memory_name(U32 value)
{
   if (value < 0 || value > nmaps)
   {
      return(str_invalid);
   }
   return(str_memblocks[value]);
}*/

#define anotherArg (++argi < argc)
#define requireArg  \
   if (!anotherArg) \
      bail_out("specifier required for argument %s", cptr);

/******************************************************************************
 *
 *    Function: handle_task_addrem
 *
 *    Args:    list of task numbers, on/off task
 *
 *    Return:  -none-
 *
 *    Purpose: enable / disable a list of tasks
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void handle_task_addrem(char *cptr, BOOLEAN value)
{
   for (cptr = strtok(cptr, ","); cptr != NULL; cptr = strtok(NULL, ","))
   {
      task_disabled[atoi(cptr) - 1] = value;
   }
}

/******************************************************************************
 *
 *    Function: main
 *
 *    Args:
 *
 *    Return:
 *
 *    Purpose: application main
 *
 *    Notes:
 *
 ******************************************************************************/
/*extern int Main(void);
GLOBAL int main(int argc, char **argv)
{
   int argi = 0;

   signal(SIGPIPE, SIG_IGN);

// L_TASK_Activate();

   log_me("%s start", model_string);      // first log_me() sets up important stuff

   // quick-and-dirty command line argument processing
   while (anotherArg)
   {
      char *cptr = argv[argi];

      if (strcmp(cptr, "task_flush") == 0)
      {
         memset(task_disabled, TRUE, sizeof(task_disabled));
      }
      else if (strcmp(cptr, "task_add") == 0)
      {
         requireArg
         handle_task_addrem(argv[argi], 0);
         //task_disabled[atoi(argv[argi]) - 1] = 0;
      }
      else if (strcmp(cptr, "task_rem") == 0)
      {
         requireArg
         handle_task_addrem(argv[argi], 1);
         //task_disabled[atoi(argv[argi]) - 1] = 1;
      }
      else if (strcmp(cptr, "quit_sec") == 0)
      {
         requireArg
         quit_seconds = atoi(argv[argi]) * 1000;
      }
      else if (strcmp(cptr, "fake_key") == 0)
      {
         requireArg
         opt_fakekey = atoi(argv[argi]);
      }
      else if (strcmp(cptr, "debug_sec") == 0)
      {
         requireArg
         debug_seconds = atoi(argv[argi]);
      }
      else if (strcmp(cptr, "sleeper") == 0)
      {
         requireArg
         opt_sleeper = atoi(argv[argi]) * 1000;
      }
      else if (strcmp(cptr, "fake_transducer") == 0)
      {
         opt_fake_transducer = TRUE;
      }
      else if (strcmp(cptr, "no_log") == 0)
      {
         opt_nolog = TRUE;
      }
      else if (strcmp(cptr, "no_adc") == 0)
      {
         opt_noadc = TRUE;
      }
      else if (strcmp(cptr, "fsync") == 0)
      {
         opt_fsync = TRUE;
      }
      else if (strcmp(cptr, "nodisclaimer") == 0)
      {
         opt_nodisclaimer = TRUE;
      }
      else if (strcmp(cptr, "notone") == 0)
      {
         opt_notone = TRUE;
      }
      else
      {
         bail_out("invalid argument %s", cptr);
      }
   }
   // Let's do some initialization
   MUTEX_Init();
   SEMA_Init();
   MEM_Init();
   TIMER_Init();
   MBOX_Init();
   QUEUE_Init();
   TASK_Init();

   Main();
   log_me("main exited");
   return(0);
}*/

/******************************************************************************
 *
 *    Function: L_UTILS_threadCreateSimpleThread
 *
 *    Args:    pointer to thread name, pointer to the thread start routine,
 *             pointer to the argument list
 *
 *    Return:  thread ID
 *
 *    Purpose: Start a helper thread with a minimum stack
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL pthread_t L_UTILS_threadCreateSimpleThread( char *pThreadName, void *(*start_routine)(void*), void *arg )
{
   pthread_attr_t    tAttr;
   int               res;
   pthread_t         threadID = 0;
   void             *pvStack = malloc( SIMPLE_THREAD_STACK_LENGTH );

   pthread_attr_init( &tAttr );
   if ( (res = pthread_attr_setstack( &tAttr, pvStack, SIMPLE_THREAD_STACK_LENGTH )) == 0 )
   {
      if ( (res = pthread_create( &threadID, &tAttr, start_routine, arg )) == 0 )
      {
         pthread_attr_destroy( &tAttr );
         log_me( "pthread_create (%s): %d", pThreadName, threadID );
         return( threadID );
      }
      else
      {
         log_me( "pthread_create result = %d, %d, %s", res, errno, strerror( errno ) );
      }
   }
   else
   {
      log_me( "pthread_attr_setstack result = %d, %d, %s", res, errno, strerror( errno ) );
   }
   free( pvStack );
   pthread_attr_destroy( &tAttr );
   return( 0 );
}

/******************************************************************************
 *
 *    Function: L_UTILS_vSetSysDate
 *
 *    Args:    number of seconds since...
 *
 *    Return:  -none-
 *
 *    Purpose: Set system time/date
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void L_UTILS_vSetSysDate( U32 u32DateTime )
{
   static time_t     time_tLastTime = 0;

   if ( abs( u32DateTime - time_tLastTime ) >= (60*60) )
   {
      struct tm      GMTime;
      char           buffer[80];

      time_tLastTime = u32DateTime;
      gmtime_r( (const time_t *)&time_tLastTime, &GMTime );
      sprintf( buffer, "date -u -s %4d.%02d.%02d-%02d:%02d:%02d > /dev/null",
               GMTime.tm_year+1900, GMTime.tm_mon+1, GMTime.tm_mday,
               GMTime.tm_hour, GMTime.tm_min, GMTime.tm_sec );
      system( buffer );
   }
}

/******************************************************************************
 *
 *    Function: L_UTILS_cpGetPathnamePtr
 *
 *    Args:    -none-
 *
 *    Return:  pointer to default path
 *
 *    Purpose: help build the appropriate filename
 *
 ******************************************************************************/
GLOBAL char *L_UTILS_cpGetPathnamePtr(void)
{
   static char *pathnamePtr = NULL;
   static char  matrix_pathname[] = "/usr/matrix";

   if (pathnamePtr == NULL)
   {                                            // only the first time
      pathnamePtr = getenv("MATRIX_CWD");       // try for an env arg
      if (pathnamePtr == NULL)                  // not available
      {
         pathnamePtr = matrix_pathname;
      }
   }
   return(pathnamePtr);
}


