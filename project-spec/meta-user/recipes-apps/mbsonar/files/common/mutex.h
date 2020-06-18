/*******************************************************************************
*           Copyright (c) 2008  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: Functions to initialize, lock, and unlock mutexes, also
                        known as resource locks.

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
#ifndef MUTEX_H
   #define MUTEX_H
#ifdef __cplusplus
   extern "C" {
#endif
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
//#include "matrixOS.h"
#include "global.h"
#include "types.h"
#include "rtx2lsvc.h"

/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef RESOURCE GMUTEX;

/********************        FUNCTION PROTOTYPES             ******************/
extern BOOLEAN MUTEX_bInit(GMUTEX *pMutex, BOOLEAN bRecursive);
extern BOOLEAN MUTEX_bDestroy(GMUTEX *pMutex);
extern BOOLEAN MUTEX_bLock(GMUTEX *pMutex);
extern BOOLEAN MUTEX_bUnlock(GMUTEX *pMutex);
extern BOOLEAN MUTEX_bTryLock(GMUTEX *pMutex);

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/
#ifdef __cplusplus
   }
#endif
#endif      // MUTEX_H
