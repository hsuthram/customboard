/************************************************************************
*    Copyright (c) 2009  Techsonic Industries, Inc.                     *
*  - Contains CONFIDENTIAL and TRADE SECRET information -               *
*************************************************************************

$Workfile: l_sema.h $
$Revision: $
Last $Modtime: 3/02/09 9:00a $
$Author: Thomas Christie $
Description: semaphore logic used by Matrix (header)
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
#ifndef L_SEMA_H
   #define L_SEMA_H

/********************           INCLUDE FILES                ******************/

#include "linuxsvc.h"
#include "l_task.h"

/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/
/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/

typedef S32 SEMA;

/********************        FUNCTION PROTOTYPES             ******************/
extern TASK L_SEMA_GetWaitingTaskID(SEMA);
extern ERC  L_SEMA_SetPending(SEMA);
extern ERC  L_SEMA_SetPendingMultiple(SEMA *);
extern ERC  L_SEMA_Signal(SEMA);
extern ERC  L_SEMA_SignalMultiple(SEMA *);
extern ERC  L_SEMA_Wait(SEMA);
extern SEMA L_SEMA_WaitMultiple(SEMA *);
extern ERC  L_SEMA_WaitTicks(SEMA, TICKS);
extern ERC  L_SEMA_WaitSeconds(SEMA, S32);
extern void SEMA_Init(void);
/********************           LOCAL VARIABLES              ******************/
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/


#endif
