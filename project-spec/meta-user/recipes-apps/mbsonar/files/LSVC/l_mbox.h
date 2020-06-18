/************************************************************************
*    Copyright (c) 2009  Techsonic Industries, Inc.                     *
*  - Contains CONFIDENTIAL and TRADE SECRET information -               *
*************************************************************************

$Workfile: l_mbox.h $
$Revision: $
Last $Modtime: 3/02/09 9:00a $
$Author: Thomas Christie $
Description: mailbox logic used by Matrix (header)
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
#ifndef L_MBOX_H
   #define L_MBOX_H

/********************           INCLUDE FILES                ******************/

#include "linuxsvc.h"
#include "l_sema.h"

/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/
/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/

typedef struct MsgHeader
{
struct MsgHeader *MsgNext;		// points to next message in mailbox queue
SEMA sema;							// signal this semaphore to ack
PRIORITY MsgPriority;			// the priority of this message
TASK task;							// for Matrix only: the sending task number
} MSGHEADER;


typedef S32 MBOX;

/********************        FUNCTION PROTOTYPES             ******************/
/********************           LOCAL VARIABLES              ******************/
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/
extern ERC L_MBOX_Send(MBOX, MSGHEADER *, PRIORITY, SEMA);
extern ERC L_MBOX_SendWait(MBOX, MSGHEADER *, PRIORITY, SEMA);
extern ERC L_MBOX_SendWaitTicks(MBOX, MSGHEADER *, PRIORITY, SEMA, TICKS);
extern ERC L_MBOX_SendWaitSeconds(MBOX, MSGHEADER *, PRIORITY, SEMA, U32);
extern MSGHEADER *L_MBOX_Receive(MBOX);
extern MSGHEADER *L_MBOX_ReceiveWait(MBOX);
extern MSGHEADER *L_MBOX_ReceiveWaitTicks(MBOX, TICKS);
extern MSGHEADER *L_MBOX_ReceiveWaitSeconds(MBOX, TICKS);
extern void L_MBOX_AckMessage(MSGHEADER *);
extern ERC L_MBOX_DefineSema(MBOX, SEMA);
extern void       MBOX_Init(void);
#endif
