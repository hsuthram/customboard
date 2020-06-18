/*******************************************************************************
*        Copyright (c) 2009 - 2010 Johnson Outdoors Marine Electronics, Inc.   *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description:

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

// must define this in order to take advantage of pthread's recursive mutexes
#define _XOPEN_SOURCE 500

/********************           INCLUDE FILES                ******************/
#include <stdlib.h>
#include <stdio.h>
#include <features.h>
#include <string.h>
#include <time.h>
#define _TIME_T
#include <pthread.h>

#include "types.h"
#include "lsvc.h"
#include "l_mbox.h"
#include "l_task.h"
#include "l_clock.h"
#include "l_utils.h"

/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/

//#define MBOX_LOGGER

typedef struct
{
   MSGHEADER        *MboxHeadPtr;   // points to first message in mailbox queue
   SEMA              MboxSemaphore; // signal this semaphore when going non-empty
   pthread_mutex_t   MboxMutex;     // for preventing concurrent access
   pthread_condattr_t    MboxCondAttr;      // for alerting waiting tasks
   pthread_cond_t    MboxCond;      // for alerting waiting tasks
} MBOX_INFO;

/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/
/********************        FUNCTION PROTOTYPES             ******************/
/********************           LOCAL VARIABLES              ******************/

PRIVATE MBOX_INFO *ptrMboxArray = NULL;
PRIVATE MBOX_INFO *endMboxArray;

#ifdef MBOX_LOGGER
PRIVATE char *xcmd_strs[] = {
   "INVALID_ID",
   "SHUTDOWN",
   "NEW_DATA",
   "RESET_POS",

   /* T_ADC messages */
   "SONAR_PROCESS_PING",
   "SONAR_PROCESS_TRIPLE_PING",
   "BLANK_SCROLL_PING",
   "REVERSE_SCROLL_PING",
   "NEW_PLAYBACK_STATE",

   /* t_display messages */
   "DISPLAY_SCREEN",
   "SET_COLOR_PALETTE",
   "TRIGGER_SCREEN_DUMP",
   "START_VIDEO_OUT_CHECK",

   /* t_view messages */
   "REGISTER_LAYER",
   "UNREGISTER_LAYER",
   "PING_UPDATE",
   "PING_START",

   /* t_menu messages */
   "REGISTER_MENU_TAB",
   "REGISTER_MENU_ITEM",
   "REGISTER_HOT_MENU",
   "REGISTER_INFO_MENU",
   "UNREGISTER_MENU_TAB",
   "UNREGISTER_MENU_ITEM",
   "ACTIVATE_INFO_MENU",
   "CLEAR_MENU",

   /* t_keyboard messages */
   "REGISTER_FOR_KEYS",
   "UNREGISTER_FOR_KEYS",
   "KEY_CHANGE",
   "INITIATE_SHUTDOWN",

   /* t_startup messages */
   "RUN_SYSTEM",

   /* t_nvram messages */
   "WRITE_TO_HEADER",
   "WRITE_TO_NVR",
   "SCRATCH_NVR",
   "READ_SONAR_DATA",
   "WRITE_SONAR_DATA",
   "READ_PERSONALIZED",
   "WRITE_PERSONALIZED",

   /* t_tone messages */
   "PLAY_TONE",

   /* t_uart messages */
   "CHANGE_BAUD_RATE",
   "DISCONNECT",
   "CAPTURE_CHANNEL",

   /* t_inline_test messages */
   "USER_TEST",

   /* t_acc messages */
   "UNIT_FOUND",
   "ATTENTION",
   "UNIT_RESET",

   /* t_acc_comm messages */
   "SEND_2_ACCESSORY",

   /* t_alarm messages */
   "SET_ALARM",
   "CLEAR_ALARM",

   /* t_mmc messages */
   "MMC_CHANGE_STATE",
   "EXPORT_NAV_DATA",
   "SCREEN_DUMP",
   "LOAD_BMP",
   "DELETE_IMAGE",
   "LOAD_THUMBNAIL",
   "LOAD_RECORDING",
   "LOAD_XMWEATHER_IMAGE",
   "LOAD_LAKEMASTER_IMAGE",

   /* t_chart_info messages */
   "CHART_INFO_MENU",
   "CHART_INFO_REQUEST",
   "CHART_WAKE_UP",
   "PORT_INFO_MENU",

   /* t_nema messages */
   "NMEA_TOGGLE_ECHO",

   /* t_bathymetry messages */
   "BATHYMETRY_WRITE_PING_DATA",

   /* t_html messages */
   "HTML_LOAD_URL",

   /* t_gateway messages */
   "GATEWAY_DATA",

   /* t_nav messages */
   "NAV_BULK_TRANSFER",

   /* t_sound messages */
   "REGISTER_SOUND",
   "PLAY_SOUND",

   /* t_ethernet messages */
   "ETHERNET_EVENT",
   "SAVE_IP_SETUP",

   /* t_chart_3d messages */
   "FREEZE_3D_LAYER",

   /* t_xmweather messages */
   "XMWEATHER_INFO_MENU",
   "XMWEATHER_INFO_MENU_RESPONSE",
   "XMWEATHER_PLACE_DATA",
   "XMWEATHER_INFO_REQUEST",
   "XMWEATHER_INFO_RESPONSE",
   "XMWEATHER_OVERLAY_NEW_DATA",
   "XMWEATHER_OVERLAY_NEW_DATA_REQUEST",
   "XMWEATHER_OVERLAY_DATA_REQUEST",
   "XMALERT_MSG",
   "XMWEATHER_WARNING_ALERT",
   "XMWEATHER_DATA_REQUEST",
   "XMWEATHER_OVERLAY_TILE_RESPONSE",
   "XMWEATHER_DIAGNOSTIC_DEBUG",
   "XMWEATHER_DIAGNOSTIC_CLEAR_NVM"
};
#endif

/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/

/****************************************************************
 * Function    MBOX_Init
 * Args:       none
 * Return:     none
 * Purpose:    This function will allocate and initialize the
 *             mailbox structure array.
 ****************************************************************/
GLOBAL void MBOX_Init(void)
{
   MBOX_INFO *miptr;

   // declaring this static in case the OS tries to access it later
   PRIVATE pthread_mutexattr_t our_mutexattr;

   ptrMboxArray = calloc(nmboxes, sizeof(MBOX_INFO));
   if (ptrMboxArray == NULL)              // unlikely, but check anyway
      return;

   endMboxArray = &ptrMboxArray[nmboxes];

   pthread_mutexattr_init(&our_mutexattr);
   pthread_mutexattr_settype(&our_mutexattr, PTHREAD_MUTEX_RECURSIVE);

   // now, spin through them all init'ing the control structure
   for (miptr = ptrMboxArray; miptr != endMboxArray; miptr++) {
      pthread_mutex_init(&miptr->MboxMutex, &our_mutexattr);
      pthread_condattr_init(&miptr->MboxCondAttr);
      pthread_condattr_setclock(&miptr->MboxCondAttr, CLOCK_MONOTONIC);
      pthread_cond_init(&miptr->MboxCond, &miptr->MboxCondAttr);
   }
}


/****************************************************************
 * Function    MBOX_GetPtr
 * Args:       1: MBOX mbox number
 * Return:     pointer to mbox info structure (or NULL if invalid)
 * Purpose:    This function will check the mbox value.
 ****************************************************************/
PRIVATE MBOX_INFO *MBOX_GetPtr(MBOX sMbox)
{
   // less than one OR too high a number gets an error here
   if (sMbox < 1 || sMbox > nmboxes)
      return(NULL);

   // make sure the array is initialized
   if (ptrMboxArray == NULL)
      return(NULL);

   return(&ptrMboxArray[sMbox - 1]);
}


/****************************************************************
 * Function    L_MBOX_Send
 * Args:       1: MBOX mbox number
 *             2: pointer to MSGHEADER and message
 *             3: PRIORITY (1 = highest)
 *             4: SEMA semaphore number for ack'ing this send
 * Return:     ERC RC_SUCCESS      if all is okay OR
 *                 RC_MBOX_INVALID if invalid mailbox number
 * Purpose:    Add the message to the mailbox queue, respecting priority.
 ****************************************************************/
GLOBAL ERC L_MBOX_Send(MBOX sMbox, MSGHEADER *mhptr, PRIORITY sPriority, SEMA sSemaphore)
{
   MBOX_INFO *miptr;
   MSGHEADER *mhpInsert;                     // link-list insert point
   MSGHEADER *mhpNextMH;                     // link-list next msg header

   miptr = MBOX_GetPtr(sMbox);
   if (miptr == NULL)
      return(RC_MBOX_INVALID);

   // set the message header structure up the way we want
   // commenting out the memset, since all fields are explicitly setup
   //memset(mhptr, 0, sizeof(MSGHEADER));
   mhptr->sema = sSemaphore;
   mhptr->MsgPriority = sPriority;
   mhptr->task = L_TASK_GetID(0);            // matrix only: our task ID

#ifdef MBOX_LOGGER
{
	char xbuf[8];
	int xcmd = *(int *)(mhptr + 1);
	if (xcmd > (sizeof(xcmd_strs) / sizeof(char *)))
		xcmd = 0;
	xbuf[0] = 0;
	if (xcmd == 2) {								// special case for NEW DATA
		char *cptr = (char *)(mhptr + 1);	// show the four char ID also
		cptr += 4;
		sprintf(xbuf, " (%4.4s)", cptr);
	}
	log_me("send %s cmd %s%s (priority %d return-sema %d)",
	get_mailbox_name(sMbox), xcmd_strs[xcmd], xbuf, sPriority, sSemaphore);
}
#endif

   pthread_mutex_lock(&miptr->MboxMutex);

   // find the place where it belongs, priority-wise
   mhpNextMH = miptr->MboxHeadPtr;           // head of the list
   if (mhpNextMH == NULL ||                  // if no messages now,
      mhpNextMH->MsgPriority > sPriority) {  // or new has better priority
      miptr->MboxHeadPtr = mhptr;            // new message is first

      // when transitioning from empty to non-empty, signal any waiters
      if (mhpNextMH == NULL) {                  // only if previously empty
         L_SEMA_Signal(miptr->MboxSemaphore);   // signal any waiters
         pthread_cond_signal(&miptr->MboxCond); // signal any waiters
      }
   } else {
      do {
         mhpInsert = mhpNextMH;                 // keep our insert point here
         mhpNextMH = mhpNextMH->MsgNext;        // this is next in line
      } while (mhpNextMH != NULL &&             // while there are more msgs
         mhpNextMH->MsgPriority <= sPriority);  // and with better priority
      mhpInsert->MsgNext = mhptr;               // save new at insert point
   }
   mhptr->MsgNext = mhpNextMH;                  // chain is now complete

   pthread_mutex_unlock(&miptr->MboxMutex);

   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_MBOX_SendWait
 * Args:       1: MBOX mbox number
 *             2: pointer to MSGHEADER and message
 *             3: PRIORITY (1 = highest)
 *             4: SEMA semaphore number for ack'ing this send
 * Return:     ERC RC_SUCCESS      if all is okay OR
 *                 RC_MBOX_INVALID if invalid mailbox number
 * Purpose:    MBOX_Send() and await an ACK on the semaphore (no timeout)
 ****************************************************************/
GLOBAL ERC L_MBOX_SendWait(MBOX sMbox, MSGHEADER *mhptr, PRIORITY sPriority, SEMA sSemaphore)
{
   ERC erc;

   L_SEMA_SetPending(sSemaphore);
   printf("[%d]%s: here\n", __LINE__, __FUNCTION__);
   erc = L_MBOX_Send(sMbox, mhptr, sPriority, sSemaphore);
   printf("[%d]%s(%d): erc %d, sMbox %d, sSemaphore %d\n", __LINE__, __FUNCTION__, L_TASK_GetID(0),  erc, sMbox, sSemaphore);

   if (erc == RC_SUCCESS)
   {
      printf("[%d]%s(%d): wait for sSemaphore %d\n", __LINE__, __FUNCTION__, L_TASK_GetID(0), sSemaphore);
      erc = L_SEMA_Wait(sSemaphore);
      printf("[%d]%s(%d): erc %d, sMbox %d, sSemaphore %d\n", __LINE__, __FUNCTION__, L_TASK_GetID(0),  erc, sMbox, sSemaphore);
   }

   return(erc);
}


/****************************************************************
 * Function    L_MBOX_SendWaitTicks
 * Args:       1: MBOX mbox number
 *             2: pointer to MSGHEADER and message
 *             3: PRIORITY (1 = highest)
 *             4: SEMA semaphore number for ack'ing this send
 *             5: TICKS number of ticks to wait for the ACK
 * Return:     ERC RC_SUCCESS      if all is okay OR
 *                 RC_MBOX_INVALID if invalid mailbox number OR
 *                 RC_TIMEOUT      if no ack received in time
 * Purpose:    MBOX_Send() and await an ACK on the semaphore (with timeout)
 ****************************************************************/
GLOBAL ERC L_MBOX_SendWaitTicks(MBOX sMbox, MSGHEADER *mhptr, PRIORITY sPriority, SEMA sSemaphore, TICKS sTicks)
{
   ERC erc;

   L_SEMA_SetPending(sSemaphore);

   erc = L_MBOX_Send(sMbox, mhptr, sPriority, sSemaphore);
   if (erc == RC_SUCCESS)
      erc = L_SEMA_WaitTicks(sSemaphore, sTicks);

   return(erc);
}


/****************************************************************
 * Function    L_MBOX_SendWaitSeconds
 * Args:       1: MBOX mbox number
 *             2: pointer to MSGHEADER and message
 *             3: PRIORITY (1 = highest)
 *             4: SEMA semaphore number for ack'ing this send
 *             5: U32 number of seconds to wait for the ACK
 * Return:     ERC RC_SUCCESS      if all is okay OR
 *                 RC_MBOX_INVALID if invalid mailbox number OR
 *                 RC_TIMEOUT      if no ack received in time
 * Purpose:    MBOX_Send() and await an ACK on the semaphore (with timeout)
 ****************************************************************/
GLOBAL ERC L_MBOX_SendWaitSeconds(MBOX sMbox, MSGHEADER *mhptr, PRIORITY sPriority, SEMA sSemaphore, U32 sSeconds)
{
   ERC erc;

   L_SEMA_SetPending(sSemaphore);

   erc = L_MBOX_Send(sMbox, mhptr, sPriority, sSemaphore);
   if (erc == RC_SUCCESS)
      erc = L_SEMA_WaitSeconds(sSemaphore, sSeconds);

   return(erc);
}


/****************************************************************
 * Function    L_MBOX_Receive
 * Args:       1: MBOX mbox number
 * Return:     MSGHEADER pointer to the received message OR
 *                       NULL if invalid mailbox number passed in OR
 *                       NULL if there is no message in the mailbox
 * Purpose:    Get the next message from the mailbox queue (return
 *             immediately if there are no messages).
 ****************************************************************/
GLOBAL MSGHEADER *L_MBOX_Receive(MBOX sMbox)
{
   MBOX_INFO *miptr;
   MSGHEADER *mhptr;                      // message header to return

   miptr = MBOX_GetPtr(sMbox);
   if (miptr == NULL)
      return(NULL);

   pthread_mutex_lock(&miptr->MboxMutex); // lock down

   mhptr = miptr->MboxHeadPtr;            // get the head message pointer
   if (mhptr != NULL)                     // if there is a message,
      miptr->MboxHeadPtr = mhptr->MsgNext;// remove it from the link-list

   pthread_mutex_unlock(&miptr->MboxMutex);

#ifdef MBOX_LOGGER
if (mhptr != NULL)
{
	char xbuf[8];
	int xcmd = *(int *)(mhptr + 1);
	if (xcmd > (sizeof(xcmd_strs) / sizeof(char *)))
		xcmd = 0;
	xbuf[0] = 0;
	if (xcmd == 2) {								// special case for NEW DATA
		char *cptr = (char *)(mhptr + 1);	// show the four char ID also
		cptr += 4;
		sprintf(xbuf, " (%4.4s)", cptr);
	}
	log_me("rcve %s cmd %s%s", get_mailbox_name(sMbox), xcmd_strs[xcmd], xbuf);
}
#endif

   return(mhptr);
}


/****************************************************************
 * Function    L_MBOX_ReceiveWait
 * Args:       1: MBOX mbox number
 * Return:     MSGHEADER pointer to the received message OR
 *                       NULL if invalid mailbox number passed in
 * Purpose:    Get the next message from the mailbox queue (wait
 *             indefinitely if there are no messages).
 ****************************************************************/
GLOBAL MSGHEADER *L_MBOX_ReceiveWait(MBOX sMbox)
{
   MBOX_INFO *miptr;
   MSGHEADER *mhptr;                      // message header to return

   miptr = MBOX_GetPtr(sMbox);
   if (miptr == NULL)
      return(NULL);

   pthread_mutex_lock(&miptr->MboxMutex);
   while ((mhptr = L_MBOX_Receive(sMbox)) == NULL) {
      L_TASK_Deactivate();
      pthread_cond_wait(&miptr->MboxCond, &miptr->MboxMutex);
      L_TASK_Activate();
   }
   pthread_mutex_unlock(&miptr->MboxMutex);

   return(mhptr);
}


/****************************************************************
 * Function    MBOX_ReceiveWaitTimed
 * Args:       1: MBOX mbox number
 *             2: pointer to timeout structure
 * Return:     MSGHEADER pointer to the received message OR
 *                       NULL if invalid mailbox number passed in OR
 *                       NULL if no messages in mailbox queue
 * Purpose:    Get the next message from the mailbox queue (with timeout).
 * Note:       Don't use this; use L_MBOX_ReceiveWaitTicks or Seconds.
 ****************************************************************/
GLOBAL MSGHEADER *MBOX_ReceiveWaitTimed(MBOX sMbox, struct timespec *tsptr)
{
   int x;
   MBOX_INFO *miptr;
   MSGHEADER *mhptr;                      // message header to return

   miptr = MBOX_GetPtr(sMbox);
   if (miptr == NULL)
      return(NULL);

   pthread_mutex_lock(&miptr->MboxMutex);
   while ((mhptr = L_MBOX_Receive(sMbox)) == NULL) {
      L_TASK_Deactivate();
      x = pthread_cond_timedwait(&miptr->MboxCond, &miptr->MboxMutex, tsptr);
      L_TASK_Activate();
      if (x != 0)
         break;
   }
   pthread_mutex_unlock(&miptr->MboxMutex);
   return(mhptr);
}


/****************************************************************
 * Function    L_MBOX_ReceiveWaitTicks
 * Args:       1: MBOX mbox number
 *             2: TICKS number of ticks to await a message
 * Return:     MSGHEADER pointer to the received message OR
 *                       NULL if invalid mailbox number passed in OR
 *                       NULL if no messages in mailbox queue
 * Purpose:    Get the next message from the mailbox queue (wait
 *             the number of ticks specified, then return NULL).
 ****************************************************************/
GLOBAL MSGHEADER *L_MBOX_ReceiveWaitTicks(MBOX sMbox, TICKS sTicks)
{
   // prepare the timeout structure (ABSTIME)
   struct timespec ts_timeout;
   L_CLOCK_PrepTimeoutTicks(&ts_timeout, sTicks);
   return(MBOX_ReceiveWaitTimed(sMbox, &ts_timeout));
}


/****************************************************************
 * Function    L_MBOX_ReceiveWaitSeconds
 * Args:       1: MBOX mbox number
 *             2: TICKS number of seconds to await a message
 * Return:     MSGHEADER pointer to the received message OR
 *                       NULL if invalid mailbox number passed in OR
 *                       NULL if no messages in mailbox queue
 * Purpose:    Get the next message from the mailbox queue (wait
 *             the number of seconds specified, then return NULL).
 ****************************************************************/
GLOBAL MSGHEADER *L_MBOX_ReceiveWaitSeconds(MBOX sMbox, TICKS sSeconds)
{
   // prepare the timeout structure (ABSTIME)
   struct timespec ts_timeout;
   L_CLOCK_PrepTimeoutSeconds(&ts_timeout, sSeconds);
   return(MBOX_ReceiveWaitTimed(sMbox, &ts_timeout));
}


/****************************************************************
 * Function    L_MBOX_AckMessage
 * Args:       1: MSGHEADER pointer
 * Return:     nothing
 * Purpose:    Signal the semaphore in the message header, thereby
 *             acknowledging the message.
 ****************************************************************/
GLOBAL void L_MBOX_AckMessage(MSGHEADER *mhptr)
{
   L_SEMA_Signal(mhptr->sema);
}


/****************************************************************
 * Function    L_MBOX_DefineSema
 * Args:       1: MBOX mailbox number
 *             2: SEMA semaphore number
 * Return:     ERC RC_SUCCESS          if no problems OR
 *                 RC_MBOX_INVALID     if invalid mailbox or init error
 *                 RC_SEMA_INVALID     if invalid semaphore condition
 * Purpose:    Associate a semaphore with a mailbox.
 ****************************************************************/
GLOBAL ERC L_MBOX_DefineSema(MBOX mbox, SEMA sema)
{
   MBOX_INFO *miptr;

   miptr = MBOX_GetPtr(mbox);
   if (miptr == NULL)
      return(RC_MBOX_INVALID);

   // allow 0-n, not 1-n, since 0 is the way to disassociate the semaphore
   if (sema < 0 || sema > nsemas)
      return(RC_SEMA_INVALID);

   miptr->MboxSemaphore = sema;
   return(RC_SUCCESS);
}


/****************************************************************
 * Function    L_MBOX_Debug
 * Args:       none
 * Return:     none
 * Purpose:    This function logs information.
 ****************************************************************/
GLOBAL void L_MBOX_Debug(void)
{
   int x = 1;
   MBOX_INFO *siptr;
   int msgno;
   MSGHEADER *mhptr;

   log_me("mbox information (%d total)", (int)(endMboxArray - ptrMboxArray));
   for (siptr = ptrMboxArray; siptr != endMboxArray; siptr++, x++) {
      log_me("  %02d %s: sema #%d (%s) mutex.lock %d", x,
      get_mailbox_name(x), siptr->MboxSemaphore, get_semaphore_name(siptr->MboxSemaphore),
      siptr->MboxMutex.__data.__lock);

      for (msgno = 1, mhptr = siptr->MboxHeadPtr;
         mhptr != NULL; msgno++, mhptr = mhptr->MsgNext)
         log_me("    msg #%d: sema %d (%s) priority %d task %d (%s)",
         msgno, mhptr->sema, get_semaphore_name(mhptr->sema),
         mhptr->MsgPriority, mhptr->task, get_task_name(mhptr->task));
   }
}


