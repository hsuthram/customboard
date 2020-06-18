/*******************************************************************************
*        Copyright (c) 2010 Johnson Outdoors Marine Electronics, Inc.          *
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
#ifndef L_UTILS_H
   #define  L_UTILS_H

#ifndef __USE_XOPEN2K
   #define  __USE_XOPEN2K
#endif
/********************           INCLUDE FILES                ******************/
#include "types.h"
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/

/********************        FUNCTION PROTOTYPES             ******************/
extern int         lm_gettime( struct timeval *tvptr );
extern void        log_me( char *msg, ... );
extern void        show_linux_priority( const char *cptr );
extern void        lm_exit( char *fname, int lineno, int status );
extern void        itoa( int value, char *optr, int radix );
extern long        atoh( char *cptr );

extern const char *get_task_name( U32 value );
extern const char *get_semaphore_name( U32 value );
extern const char *get_queue_name( U32 value );
extern const char *get_mutex_name( U32 value );
extern const char *get_mailbox_name( U32 value );
//extern const char *get_memory_name( U32 value );

extern pthread_t   L_UTILS_threadCreateSimpleThread( char *pThreadName, void * (* start_routine)( void * ), void *arg );
extern void        L_UTILS_vSetSysDate( U32 u32DateTime );
extern char       *L_UTILS_cpGetPathnamePtr( void );

extern void        MBOX_Init( void );
//extern void        MEM_Init( void );
extern void        MUTEX_Init( void );
extern void        QUEUE_Init( void );
extern void        SEMA_Init( void );
extern void        TASK_Init( void );
extern void        TIMER_Init( void );

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/

#endif   // #ifndef L_UTILS_H
