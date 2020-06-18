/************************************************************************
*    Copyright (c) 2009  Techsonic Industries, Inc.                     *
*  - Contains CONFIDENTIAL and TRADE SECRET information -               *
*************************************************************************

$Workfile: l_clock.h $
$Revision: $
Last $Modtime: 3/02/09 9:00a $
$Author: Thomas Christie $
Description: OS level clock stuff for timers and timed waiting

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
#ifndef L_CLOCK_H
   #define L_CLOCK_H

/********************           INCLUDE FILES                ******************/

#include "linuxsvc.h"

/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/

// these are pretty straight-forward
#define NANS_PER_SECOND (1000000000)
#define NANS_PER_MILLI  (1000000)
#define NANS_PER_MICRO  (1000)
#define MICS_PER_SECOND (1000000)
#define MICS_PER_MILLI  (1000)
#define MILS_PER_SECOND (1000)

// !!! this value will likely change one day; but for now, a tick is a milli
#define NANS_PER_TICK NANS_PER_MILLI
#define MICS_PER_TICK MICS_PER_MILLI

// extrapolated from tick frequency in nans is the tick frequency in seconds
#define TICKS_PER_SECOND (NANS_PER_SECOND / NANS_PER_TICK)

// some convenience macros to get elements in a U32 instead of a U64
#define L_CLOCK_dwGetTimeInNans   (U32)L_CLOCK_qwGetTimeInNans
#define L_CLOCK_dwGetTimeInMicros (U32)L_CLOCK_qwGetTimeInMicros
#define L_CLOCK_dwGetTimeInMillis (U32)L_CLOCK_qwGetTimeInMillis
#define L_CLOCK_GetTimeInTicks    (TICKS)L_CLOCK_qwGetTimeInTicks

/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/
/********************        FUNCTION PROTOTYPES             ******************/

extern U64 L_CLOCK_qwGetTimeInNans(void);
extern U64 L_CLOCK_qwGetTimeInMicros(void);
extern U64 L_CLOCK_qwGetTimeInMillis(void);
extern U64 L_CLOCK_qwGetTimeInTicks(void);

extern void L_CLOCK_PrepTimeoutTicks(struct timespec *, TICKS);
extern void L_CLOCK_PrepTimeoutSeconds(struct timespec *, U32);
extern void L_CLOCK_StoreTimerspec(struct itimerspec *, TICKS, TICKS);
extern TICKS L_CLOCK_ScanTimerspec(struct itimerspec *);

extern void L_CLOCK_Sleep(TICKS);

/********************           LOCAL VARIABLES              ******************/
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/

#endif
