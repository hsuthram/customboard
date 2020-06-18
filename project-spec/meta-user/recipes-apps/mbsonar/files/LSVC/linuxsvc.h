/************************************************************************
*    Copyright (c) 2009  Techsonic Industries, Inc.                     *
*  - Contains CONFIDENTIAL and TRADE SECRET information -               *
*************************************************************************

$Workfile: linuxsvc.h $
$Revision: $
Last $Modtime: 3/02/09 9:00a $
$Author: Thomas Christie $
Description: Enhanced 900 Series header

The Enhanced 900 Series is expected to run on embedded Linux.  Hence,
the name for this is Linux SerViCes.

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+ This software is the proprietary property of:                   +
+    Techsonic Industries, Inc.                                   +
+    1220 Old Alpharetta Rd.        1 Humminbird Lane             +
+    Alpharetta, GA  30005          Eufaula, Alabama  36027       +
+ Use, duplication, or disclosure of this material, in any form,  +
+ is forbidden without permission from Techsonic Industries, Inc. +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

*******************************************************************************/


/********************           COMPILE FLAGS                ******************/
#ifndef LINUXSVC_H
   #define LINUXSVC_H

/********************           INCLUDE FILES                ******************/
//#include "dd_arm_neon.h"
/********************               ENUMS                    ******************/

// queue condition (see l_queue.h and l_queue.c)
typedef enum {
QNE = 0,                   // queue not empty
QNF,                       // queue not full
QF,                        // queue full
QE,                        // queue empty
QCOND_COUNT
} QUEUE_CONDITION;


typedef enum LINUX_SVC_RETURN_CODES {
// generic stuff
lsSUCCESS = 0,
#define RC_SUCCESS             (0 - lsSUCCESS)
lsTIMEOUT,
#define RC_TIMEOUT             (0 - lsTIMEOUT)

// semaphore logic
lsSEMA_INVALID,
#define RC_SEMA_INVALID        (0 - lsSEMA_INVALID)
lsSEMA_OVERRUN,
#define RC_SEMA_OVERRUN        (0 - lsSEMA_OVERRUN)
lsSEMA_PENDING,
#define RC_SEMA_PENDING        (0 - lsSEMA_PENDING)
lsSEMA_DONE,
#define RC_SEMA_DONE           (0 - lsSEMA_DONE)
lsSEMA_WAIT_CONFLICT,
#define RC_SEMA_WAIT_CONFLICT  (0 - lsSEMA_WAIT_CONFLICT)

// task logic
lsTASK_INVALID,
#define RC_TASK_INVALID        (0 - lsTASK_INVALID)
lsTASK_PARAMS,
#define RC_TASK_PARAMS         (0 - lsTASK_PARAMS)
lsTASK_MEMORY,
#define RC_TASK_MEMORY         (0 - lsTASK_MEMORY)
lsTASK_START,
#define RC_TASK_START          (0 - lsTASK_START)

// timer stuff
lsNO_TIMER,
#define RC_NO_TIMER            (0 - lsNO_TIMER)
lsTIMER_ILLEGAL,
#define RC_TIMER_ILLEGAL       (0 - lsTIMER_ILLEGAL)
lsTIMER_UNINIT,
#define RC_TIMER_UNINIT        (0 - lsTIMER_UNINIT)
lsTIMER_INACTIVE,
#define RC_TIMER_INACTIVE      (0 - lsTIMER_INACTIVE)

// clock stuff
lsNO_CLOCK,
#define RC_NO_CLOCK            (0 - lsNO_CLOCK)

// queue stuff
lsQUEUE_INVALID,
#define RC_QUEUE_INVALID       (0 - lsQUEUE_INVALID)
lsQCOND_INVALID,
#define RC_QCOND_INVALID       (0 - lsQCOND_INVALID)
lsQUEUE_NO_MEMORY,
#define RC_QUEUE_NO_MEMORY     (0 - lsQUEUE_NO_MEMORY)
lsQUEUE_FULL,
#define RC_QUEUE_FULL          (0 - lsQUEUE_FULL)
lsQUEUE_EMPTY,
#define RC_QUEUE_EMPTY         (0 - lsQUEUE_EMPTY)

// mutex stuff
lsMUTEX_INVALID,
#define RC_MUTEX_INVALID       (0 - lsMUTEX_INVALID)
lsMUTEX_BUSY,
#define RC_MUTEX_BUSY          (0 - lsMUTEX_BUSY)
lsMUTEX_NESTED,
#define RC_MUTEX_NESTED        (0 - lsMUTEX_NESTED)

// memory stuff
lsMAP_INVALID,
#define RC_MAP_INVALID         (0 - lsMAP_INVALID)
lsMAP_CROSSED,
#define RC_MAP_CROSSED         (0 - lsMAP_CROSSED)
lsMAP_NO_MEMORY,
#define RC_MAP_NO_MEMORY       (0 - lsMAP_NO_MEMORY)
lsMAP_NONE_AVAIL,
#define RC_MAP_NONE_AVAIL      (0 - lsMAP_NONE_AVAIL)

// mailbox queue stuff
lsMBOX_INVALID,
#define RC_MBOX_INVALID        (0 - lsMBOX_INVALID)

lsCOUNT
} LSRC;


/********************              DEFINES                   ******************/

// NO_PARTIAL_LISTS:  commented out, the code will process all valid entries
// of a list (skipping those that are not valid); uncommented, the code will
// not process the list at all unless all entries are valid.
//#define NO_PARTIAL_LISTS


// DBG_WHEEL:  quick-and-dirty exec-path debugging; in the interest of speed,
// this code does not perform a buffer pointer wrap-around, so make sure the
// dbg_wheel buffer is big enough or change the code in dbg_add().  Also, you
// will need to provide your own debug display function, to make sense of the
// debug buffer as you chose to use it.  See example test_timer if you need.
//#define DBG_WHEEL

#ifdef DBG_WHEEL

#ifdef IS_L_TASK
// only define things once, in l_task.*
U8 dbg_wheel[32*1024];
U8 *dbg_ptr = dbg_wheel;
#else
extern U8 dbg_wheel[];
extern U8 *dbg_ptr;
#endif

#define dbg_add(x,y) {*dbg_ptr++ = x;*dbg_ptr++ = y;}

#else
#define dbg_add(x,y)
#endif

#define program_exit(x) lm_exit(__FILE__, __LINE__, x)

void lm_exit(char *fname, int lineno, int status);
void log_me(char *msg, ...);
long atoh(char *);
void show_linux_priority(const char *cptr);

/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/

// the ERC can be a result code, task number, semaphore number, etc.
typedef S32 ERC;

typedef U32 TICKS;

/********************        FUNCTION PROTOTYPES             ******************/
/********************           LOCAL VARIABLES              ******************/
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/


#endif
