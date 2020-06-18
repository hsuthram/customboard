/*******************************************************************************
*           Copyright (c) 2008  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: Siplified Mutexes - aka Resource Locks -
                        Sometimes synonymous with "semaphore"

                        Lock and Unlock shared resources

      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      + This software is the proprietary property of:                   +
      +                                                                 +
      +  Techsonic Industries, Inc.                                     +
      +                                                                 +
      +  1220 Old Alpharetta Rd           1 Humminbird Lane             +
      +  Suite 340                        Eufaula, AL  36027            +
      +  Alpharetta, GA  30005                                          +
      +                                                                 +
      + Use, duplication, or disclosure of this material, in any form,  +
      + is forbidden without permission from Techsonic Industries, Inc. +
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

*******************************************************************************/


/********************           COMPILE FLAGS                ******************/
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#ifdef __RTXC__
//   #include "matrixOS.h"
//   #include "nomalloc.h"
//   #include "model.h"
#else
   #include "global.h"
   #include "types.h"
#endif
#include "mutex.h"
#ifndef __RTXC__
   #include <pthread.h>
#endif
//#if MODEL_HAS_NETWORKING
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/

/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/
#ifndef __RTXC__
   GLOBAL const GMUTEX gMUTEX_MutexInitializer            = STATIC_MUTEX_INITIALIZER;
   GLOBAL const GMUTEX gMUTEX_RecursiveMutexInitializer   = STATIC_RECURSIVE_MUTEX_INITIALIZER;
#endif

/********************              FUNCTIONS                 ******************/

/******************************************************************************
 *
 *    Function: MUTEX_bInit
 *
 *    Args:    pMutex - ptr to mutex object
 *             bRecursive - can the same thread relock a locked resource?
 *
 *    Return:  BOOLEAN - TRUE if successful
 *
 *    Purpose: Initialize a Mutex object
 *
 *    Notes:   see also MUTEX_FAST_INIT macro
 *
 ******************************************************************************/
GLOBAL BOOLEAN MUTEX_bInit(GMUTEX *pMutex, BOOLEAN bRecursive)
{
#ifdef __RTXC__
   //nothing to init
   return(TRUE);
#else
   if (bRecursive)
   {
      pthread_mutexattr_t lock_attribute;

      return ((0 == pthread_mutexattr_init(&lock_attribute) &&
               0 == pthread_mutexattr_settype(&lock_attribute, PTHREAD_MUTEX_RECURSIVE) &&
               0 == pthread_mutex_init(pMutex, &lock_attribute)) ? TRUE : FALSE);
   }
   else
   {
      return(0 == pthread_mutex_init(pMutex, NULL) ? TRUE : FALSE);
   }
#endif
}

/******************************************************************************
 *
 *    Function: MUTEX_bDestroy
 *
 *    Args:    pMutex - ptr to mutex object
 *
 *    Return:  BOOLEAN - TRUE if successful
 *
 *    Purpose: Destoy a Mutex
 *
 *    Notes:   undefined destroying locked mutex
 *             not totally necessary to destroy a mutex on Linux
 *
 ******************************************************************************/
GLOBAL BOOLEAN MUTEX_bDestroy(GMUTEX *pMutex)
{
#ifdef __RTXC__
   //nothing to destroy
   return(TRUE);
#else
   return((0 == pthread_mutex_destroy(pMutex)) ? TRUE : FALSE);
#endif
}

/******************************************************************************
 *
 *    Function: MUTEX_bLock
 *
 *    Args:    pMutex - ptr to Mutex object
 *
 *    Return:  BOOLEAN - TRUE if successful
 *
 *    Purpose: Lock a Mutex
 *
 *    Notes:   only recursive locks can be relocked after being locked in
 *             a given thread, otherwise - deadlock
 *             always pair locks and unlocks
 *
 ******************************************************************************/
GLOBAL BOOLEAN MUTEX_bLock(GMUTEX *pMutex)
{
#ifdef __RTXC__
    if (RC_GOOD == KS_lockw((RESOURCE)*pMutex))
    {
       return(TRUE);
    }
    else
    {
       return(FALSE);
    }
#else
   return((0 == pthread_mutex_lock(pMutex)) ? TRUE : FALSE);
#endif
}

/******************************************************************************
 *
 *    Function: MUTEX_bUnlock
 *
 *    Args:    pMutex - ptr to Mutex object
 *
 *    Return:  BOOLEAN - TRUE if successful
 *
 *    Purpose: Unlock a Mutex
 *
 *    Notes:   always pair locks and unlocks
 *
 ******************************************************************************/
GLOBAL BOOLEAN MUTEX_bUnlock(GMUTEX *pMutex)
{
#ifdef __RTXC__
   if (RC_GOOD == KS_unlock((RESOURCE)*pMutex))
   {
      return(TRUE);
   }
   else
   {
      return(FALSE);
   }
#else
   return((0 == pthread_mutex_unlock(pMutex)) ? TRUE : FALSE);
#endif
}

/******************************************************************************
 *
 *    Function: MUTEX_bTrylock
 *
 *    Args:    pMutex - ptr to Mutex object
 *
 *    Return:  BOOLEAN - TRUE if successful
 *
 *    Purpose: try lock a Mutex
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN MUTEX_bTryLock(GMUTEX *pMutex)
{
#ifdef __RTXC__
   if (RC_BUSY != KS_lock((RESOURCE)*pMutex))
   {
      return(TRUE);
   }
   else
   {
      return(FALSE);
   }
#else
   return((0 == pthread_mutex_trylock(pMutex)) ? TRUE : FALSE);
#endif
}
//#endif //MODEL_HAS_NETWORKING

