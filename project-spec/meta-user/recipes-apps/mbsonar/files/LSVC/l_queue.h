/************************************************************************
*    Copyright (c) 2009  Techsonic Industries, Inc.                     *
*  - Contains CONFIDENTIAL and TRADE SECRET information -               *
*************************************************************************

$Workfile: l_queue.h $
$Revision: $
Last $Modtime: 3/02/09 9:00a $
$Author: Thomas Christie $
Description: queue logic used by Matrix (header)
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
#ifndef L_QUEUE_H
   #define L_QUEUE_H

/********************           INCLUDE FILES                ******************/

#include "linuxsvc.h"
#include "l_sema.h"

/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/
/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/

typedef S32 QUEUE;

typedef struct {
U32 QParamWidth;				// how many bytes in each entry
U32 QParamDepth;				// maximum number of entries
} QUEUE_PARAM;

/********************        FUNCTION PROTOTYPES             ******************/
/********************           LOCAL VARIABLES              ******************/
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/
extern void QUEUE_Init(void);
extern ERC L_QUEUE_Define(QUEUE, U32, U32, U8 *, U32);
extern ERC L_QUEUE_Put(QUEUE, void *);
extern ERC L_QUEUE_PutWait(QUEUE, void *);
extern ERC L_QUEUE_PutWaitTicks(QUEUE, void *, TICKS);
extern ERC L_QUEUE_PutWaitSeconds(QUEUE, void *, U32);
extern ERC L_QUEUE_Get(QUEUE, void *);
extern ERC L_QUEUE_GetWait(QUEUE, void *);
extern ERC L_QUEUE_GetWaitTicks(QUEUE, void *, TICKS);
extern ERC L_QUEUE_GetWaitSeconds(QUEUE, void *, U32);
extern ERC L_QUEUE_Purge(QUEUE);
extern ERC L_QUEUE_DefineSema(QUEUE, SEMA, QUEUE_CONDITION);
extern ERC L_QUEUE_Inquire(QUEUE);


#endif
