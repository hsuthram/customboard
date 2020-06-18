/************************************************************************
*    Copyright (c) 2009  Techsonic Industries, Inc.                     *
*  - Contains CONFIDENTIAL and TRADE SECRET information -               *
*************************************************************************

$Workfile: rtx2lsvc.h $
$Revision: $
Last $Modtime: 3/02/09 9:00a $
$Author: Thomas Christie $
Description: Enhanced 900 Series header for RTXC backward-compatibility

The Enhanced 900 Series is expected to run on embedded Linux.  The RTXC
environment used to define some things that are no longer available.
This header file should be included by any code that used to work in
RTXC to provide some glue to the enhanced 900 way of doing things.

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
#ifndef RTX2LSVC_H
   #define RTX2LSVC_H

/********************           INCLUDE FILES                ******************/
#include <pthread.h>
#include <unistd.h>
#include "types.h"

#include "l_timer.h"
#include "l_mutex.h"
#include "l_task.h"
#include "l_sema.h"
#include "l_mbox.h"
#include "l_queue.h"
//#include "l_mem.h"

/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/

#define RC_GOOD             RC_SUCCESS
#define RC_BUSY             RC_MUTEX_BUSY

#define SELFTASK            ((TASK)0)
#define NULLTASK_PRIORITY   101        // lowest supported priority

// some semaphore return codes
#define SEMA_PENDING RC_SEMA_PENDING
#define SEMA_DONE    RC_SEMA_DONE

// function translations
#define KS_inqsema         L_SEMA_GetWaitingTaskID
#define KS_pend            L_SEMA_SetPending
#define KS_pendm           L_SEMA_SetPendingMultiple
#define KS_signal          L_SEMA_Signal
#define KS_signalm         L_SEMA_SignalMultiple
#define KS_wait            L_SEMA_Wait
#define KS_waitm           L_SEMA_WaitMultiple
#define KS_waitt           L_SEMA_WaitTicks

#define KS_alloc_timer            L_TIMER_Allocate
#define KS_free_timer             L_TIMER_Release
#define KS_start_timer            L_TIMER_Start
#define KS_restart_timer(a,b,c)   L_TIMER_Start(a,b,c,-1)
#define KS_stop_timer             L_TIMER_Stop
#define KS_inqtimer               L_TIMER_GetRemaining
#define KS_elapse                 L_TIMER_GetElapsed

#define KS_lock            L_MUTEX_Lock
#define KS_lockw           L_MUTEX_LockWait
#define KS_lockt           L_MUTEX_LockTime
#define KS_unlock          L_MUTEX_Unlock
#define KS_inqres          L_MUTEX_GetOwnerTaskID

#define KS_defqueue        L_QUEUE_Define
#define KS_enqueue         L_QUEUE_Put
#define KS_enqueuew        L_QUEUE_PutWait
#define KS_enqueuet        L_QUEUE_PutWaitTicks
#define KS_dequeue         L_QUEUE_Get
#define KS_dequeuew        L_QUEUE_GetWait
#define KS_dequeuet        L_QUEUE_GetWaitTicks
#define KS_purgequeue      L_QUEUE_Purge
#define KS_defqsema        L_QUEUE_DefineSema
#define KS_inqqueue        L_QUEUE_Inquire

#define KS_execute         L_TASK_Start
#define KS_inqpriority     L_TASK_GetPriority
#define KS_defpriority     L_TASK_SetPriority
#define KS_delay(y,x)      usleep((x) * 1000)
#define KS_terminate       L_TASK_Terminate
#define KS_inqtask()       L_TASK_GetID(0)

//#define KS_alloc           L_MEM_Allocate
//#define KS_allocw          L_MEM_AllocateWait
//#define KS_alloct          L_MEM_AllocateWaitTicks
//#define KS_free            L_MEM_Free
//#define KS_inqmap          L_MEM_GetBlockSize
//#define KS_create_part     L_MEM_Create

#define KS_send                   L_MBOX_Send
#define KS_sendw                  L_MBOX_SendWait
#define KS_sendt                  L_MBOX_SendWaitTicks
#define KS_receive(x1,x2)         L_MBOX_Receive(x1)
#define KS_receivew(x1,x2)        L_MBOX_ReceiveWait(x1)
#define KS_receivet(x1,x2,x3,x4)  L_MBOX_ReceiveWaitTicks(x1,x3)
#define KS_ack                    L_MBOX_AckMessage
#define KS_defmboxsema            L_MBOX_DefineSema

#define KS_inqtime()       time(NULL)
#define KS_yield           pthread_yield

#define ENABLE
#define DISABLE

#define KS_defslice(x,y)
#define KS_ISRsignal(x)

/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/
typedef TIMER_INFO CLKBLK;
typedef MUTEX      RESOURCE;
typedef MSGHEADER  RTXCMSG;
typedef LSRC       KSRC;

/********************        FUNCTION PROTOTYPES             ******************/

// this function is declared only when RTXC is available, which we simulate
extern void  H_MEMORY_vDumpMail( RTXCMSG *pMsg );


// don't know what I'm going to do about Yield yet.
//KSRC KS_yield(void);

/********************           LOCAL VARIABLES              ******************/
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/


#endif
