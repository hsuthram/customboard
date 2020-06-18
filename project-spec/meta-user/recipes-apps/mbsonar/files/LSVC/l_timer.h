/************************************************************************
*    Copyright (c) 2009  Techsonic Industries, Inc.                     *
*  - Contains CONFIDENTIAL and TRADE SECRET information -               *
*************************************************************************

$Workfile: l_timer.h $
$Revision: $
Last $Modtime: 3/02/09 9:00a $
$Author: Thomas Christie $
Description: timer logic used by Matrix (header)
(functional design in Enhanced 900 Linux Port document)

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
#ifndef L_TIMER_H
   #define L_TIMER_H

/********************           INCLUDE FILES                ******************/

#include "linuxsvc.h"
#include "l_sema.h"

/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/
/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/

typedef struct
{
U8 u8TimerState;
U8 u8TimerSignal;
TASK OwningTaskID;
SEMA Semaphore;
timer_t TimerID;

U32 timer_fired_count;
U32 timer_queue_count;
} TIMER_INFO;

/********************        FUNCTION PROTOTYPES             ******************/
extern void        TIMER_Init(void);
extern ERC L_TIMER_InitTimer(TIMER_INFO *, TASK);

extern TIMER_INFO *L_TIMER_Allocate(void);
extern ERC L_TIMER_Release(TIMER_INFO *);
extern TIMER_INFO *L_TIMER_Start(TIMER_INFO *, TICKS, TICKS, SEMA);
extern ERC L_TIMER_Stop(TIMER_INFO *);
extern TICKS L_TIMER_GetRemaining(TIMER_INFO *);
extern TICKS L_TIMER_GetElapsed(TICKS *);


/********************           LOCAL VARIABLES              ******************/
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/

#endif
