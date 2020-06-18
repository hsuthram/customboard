/************************************************************************
*    Copyright (c) 2009  Techsonic Industries, Inc.                     *
*  - Contains CONFIDENTIAL and TRADE SECRET information -               *
*************************************************************************

$Workfile: l_mutex.h $
$Revision: $
Last $Modtime: 3/02/09 9:00a $
$Author: Thomas Christie $
Description: resource logic used by Matrix (header)
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
#ifndef L_MUTEX_H
   #define L_MUTEX_H

/********************           INCLUDE FILES                ******************/

#include "linuxsvc.h"

/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/
/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/

typedef S32 MUTEX;

/********************        FUNCTION PROTOTYPES             ******************/
extern void MUTEX_Init(void);
extern ERC L_MUTEX_Lock(MUTEX);
extern ERC L_MUTEX_LockWait(MUTEX);
extern ERC L_MUTEX_LockTime(MUTEX,TICKS);
extern ERC L_MUTEX_Unlock(MUTEX);
extern TASK L_MUTEX_GetOwnerTaskID(MUTEX);

/********************           LOCAL VARIABLES              ******************/
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/

#endif
