/*******************************************************************************
*           Copyright (c) 2009 - 2010 Techsonic Industries, Inc.               *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: task logic used by Matrix (header)
               (functional design in Enhanced 900 Linux Port document)  

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
#ifndef L_TASK_H
#define L_TASK_H

/********************           INCLUDE FILES                ******************/
#include <time.h>
#include <pthread.h>
#include "linuxsvc.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define L_TASK_ResetPriority() L_TASK_SetPriority(0, 0)
/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef S32    TASK;
typedef U16    PRIORITY;

typedef struct 
{
   U16      TParamStackSize;              // bytes needed for stack space
   PRIORITY TParamPriority;         // starting priority
   void     (*TParamStartFunction)(void);
} TASK_PARAM;
/********************        FUNCTION PROTOTYPES             ******************/
extern TASK L_TASK_GetID(pthread_t);
extern ERC  L_TASK_LockMutex(TASK);
extern ERC  L_TASK_UnlockMutex(TASK);
extern ERC  L_TASK_WaitConditional(TASK);
extern ERC  L_TASK_WaitConditionalTimed(TASK, struct timespec *);
extern ERC  L_TASK_SignalConditional(TASK);

extern ERC L_TASK_Start(TASK);
extern ERC L_TASK_GetPriority(TASK);
extern ERC L_TASK_SetPriority(TASK, PRIORITY);
extern ERC L_TASK_Terminate(TASK);

extern void L_TASK_Activate(void);
extern void L_TASK_Deactivate(void);
extern void TASK_Init(void);
/********************           LOCAL VARIABLES              ******************/
/********************          GLOBAL VARIABLES              ******************/
/********************              FUNCTIONS                 ******************/

#endif
