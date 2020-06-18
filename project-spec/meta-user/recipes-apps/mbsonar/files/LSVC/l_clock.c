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
/********************           INCLUDE FILES                ******************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define _TIME_T
#include <pthread.h>
#include <unistd.h>

#include "types.h"

#include "l_clock.h"

/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/
/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/
/********************        FUNCTION PROTOTYPES             ******************/
/********************           LOCAL VARIABLES              ******************/

PRIVATE struct timespec StartTimespec;		// time since epoch at program start

/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/


/****************************************************************
 * Function    CLOCK_internalGetTimespec
 * Args:       ptr to struct timespec as defined in <time.h>
 * Return:     structure filled in with the time since 1/1/70
 * Purpose:    This function gets the time since 1/1/70 from the OS.
 * Note:       There is a copy of the structure saved for recording
 *             the time since program start, a handy value to have.
 ****************************************************************/
GLOBAL void CLOCK_internalGetTimespec(struct timespec *tsptr)
{
	clock_gettime(CLOCK_MONOTONIC, tsptr);		// get the time
	if (StartTimespec.tv_sec == 0)				// if first time this happened,
		memcpy(&StartTimespec, tsptr, sizeof(StartTimespec));
}

/****************************************************************
 * Function    CLOCK_internalGetTime
 * Args:       ptr to struct timespec as defined in <time.h>
 * Return:     structure filled in with the time since program start.
 * Purpose:    This function gets the time since program start.
 ****************************************************************/
PRIVATE void CLOCK_internalGetTime(struct timespec *tsptr)
{
	CLOCK_internalGetTimespec(tsptr);			// get the time

	// now bias the current time down to time since program start
	tsptr->tv_nsec -= StartTimespec.tv_nsec;	// remove nans at prog start
	if (tsptr->tv_nsec < 0) {						// if it wrapped,
		tsptr->tv_nsec += NANS_PER_SECOND;		// put it back above zero
		tsptr->tv_sec--;								// and carry the one
	}
	tsptr->tv_sec -= StartTimespec.tv_sec;		// remove seconds, too
}


/****************************************************************
 * Functions   L_CLOCK_qwGetTimeInNans
 *             L_CLOCK_qwGetTimeInMicros
 *             L_CLOCK_qwGetTimeInMillis
 *             L_CLOCK_qwGetTimeInTicks
 * Args:       none
 * Return:     U64 since program started
 * Purpose:    These functions get the current time from the OS, then
 *             return the time since program start.
 * Note:       If you're using this time as a timer, as opposed to a time
 *             stamp, you probably want to call the U32 version of these:
 *             L_CLOCK_dwGetTime (InNans, InMicros, InMillis, and InTicks).
 ****************************************************************/
GLOBAL U64 L_CLOCK_qwGetTimeInNans(void)
{
	struct timespec now;
	CLOCK_internalGetTime(&now);
	return((U64)now.tv_nsec + ((U64)now.tv_sec * NANS_PER_SECOND));
}

GLOBAL U64 L_CLOCK_qwGetTimeInMicros(void)
{
	return(L_CLOCK_qwGetTimeInNans() / NANS_PER_MICRO);
}

GLOBAL U64 L_CLOCK_qwGetTimeInMillis(void)
{
	return(L_CLOCK_qwGetTimeInNans() / NANS_PER_MILLI);
}

GLOBAL U64 L_CLOCK_qwGetTimeInTicks(void)
{
	return(L_CLOCK_qwGetTimeInNans() / NANS_PER_TICK);
}


/****************************************************************
 * Function    L_CLOCK_PrepTimeoutTicks
 * Args:       1: ptr to struct timespec as defined in <time.h>
 *             2: TICKS number of ticks until timeout
 * Return:     nothing (structure filled in)
 * Purpose:    This function prepares an ABSTIME timeout structure.
 * Note:       This function is actually coded twice; the commented
 *             out parts do 64-bit math, if for some reason that is
 *             preferred later.
 ****************************************************************/
GLOBAL void L_CLOCK_PrepTimeoutTicks(struct timespec *tsptr, TICKS sTicks)
{
	U32 sSecs, sNans;							// for busting ticks into secs and nans
//	U64 sNans;

	CLOCK_internalGetTimespec(tsptr);	// get now

	sSecs = sTicks / TICKS_PER_SECOND;
	sNans = (sTicks % TICKS_PER_SECOND) * NANS_PER_TICK;
//	sNans = sTicks * NANS_PER_TICK;		// this many nans in the future

	tsptr->tv_nsec += sNans;				// adjust current time forward
//	tsptr->tv_nsec += (sNans % NANS_PER_SECOND);
	if (tsptr->tv_nsec >= NANS_PER_SECOND) {
		tsptr->tv_nsec -= NANS_PER_SECOND;
		tsptr->tv_sec++;
	}
	tsptr->tv_sec += sSecs;
//	tsptr->tv_sec += (sNans / NANS_PER_SECOND);
}


/****************************************************************
 * Function    L_CLOCK_PrepTimeoutSeconds
 * Args:       1: ptr to struct timespec as defined in <time.h>
 *             2: U32 number of seconds until timeout
 * Return:     nothing (structure filled in)
 * Purpose:    This function prepares an ABSTIME timeout structure.
 * Note:       This function is actually coded twice; the commented
 *             out parts do 64-bit math, if for some reason that is
 *             preferred later.
 ****************************************************************/
GLOBAL void L_CLOCK_PrepTimeoutSeconds(struct timespec *tsptr, U32 sSecs)
{
	CLOCK_internalGetTimespec(tsptr);	// get now
	tsptr->tv_sec += sSecs;
}


/****************************************************************
 * Function    L_CLOCK_StoreTimerspec
 * Args:       1: ptr to struct itimerspec as defined in <time.h>
 *             2: TICKS start interval value
 *             3: TICKS recurring interval value
 * Return:     nothing (structure filled in)
 * Purpose:    This function prepares the itimerspec structure.
 * Note:       This function is only in this file because of my desire
 *             to keep all "ticks-math" in the same location.
 ****************************************************************/

// little helper function
PRIVATE void GCS_doit(struct timespec *tsptr, TICKS sTicks)
{
	tsptr->tv_sec = sTicks / TICKS_PER_SECOND;
	tsptr->tv_nsec = (sTicks % TICKS_PER_SECOND) * NANS_PER_TICK;
}

GLOBAL void L_CLOCK_StoreTimerspec(struct itimerspec *itptr, TICKS sStart, TICKS sRecur)
{
	GCS_doit(&itptr->it_value, sStart);
	GCS_doit(&itptr->it_interval, sRecur);
}


/****************************************************************
 * Function    L_CLOCK_StoreTimerspec
 * Args:       1: ptr to struct itimerspec as defined in <time.h>
 * Return:     TICKS representation of this timerspec's value
 * Purpose:    This function prepares the itimerspec structure.
 * Note:       This function is only in this file because of my desire
 *             to keep all "ticks-math" in the same location.
 ****************************************************************/
GLOBAL TICKS L_CLOCK_ScanTimerspec(struct itimerspec *itptr)
{
	struct timespec *tsptr = &itptr->it_value;
	return((tsptr->tv_sec * TICKS_PER_SECOND) + ((tsptr->tv_nsec + (NANS_PER_TICK / 2)) / NANS_PER_TICK));
}


/****************************************************************
 * Function    L_TIMER_GetElapsed
 * Args:       1: pointer to TICKS
 * Return:     1: TICKS number of ticks since last time called
 *             2: value at TICKS pointer updated to current tick time
 * Purpose:    Perform delta timing.
 ****************************************************************/
GLOBAL void L_CLOCK_Sleep(TICKS sTicks)
{
	usleep(sTicks * MICS_PER_TICK);
}


