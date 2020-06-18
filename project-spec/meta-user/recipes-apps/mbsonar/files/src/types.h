/*******************************************************************************
*     Copyright (c) 2002 - 2017 Johnson Outdoors Marine Electronics, Inc.      *
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
#ifndef TYPES_H
   #define TYPES_H
/********************           INCLUDE FILES                ******************/

/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#ifndef FALSE
   #define FALSE (0)
#endif
#ifndef TRUE
   #define TRUE  (1)
#endif

#ifndef NULL
   #define NULL         ((void *)0)
#endif

#define PRIVATE  static /* Only functions within module can access this function/variable */
#define GLOBAL          /* Anyone can use this variable/call this function */

#ifndef linux
#define NOINIT_DATA __attribute__((section("no_init"), zero_init)) /* don't need to init this data structure */
#define INLINE __inline
#else
#define NOINIT_DATA
#define INLINE inline
#endif
/********************               MACROS                   ******************/
/* Generic macros */
#ifdef DEBUG
   #define TEST(x)     x
#else
   #define TEST(x)
#endif

#define MIN(x,y)           ( (x)<(y) ? (x) : (y) )
#define MAX(x,y)           ( (x)>(y) ? (x) : (y) )
#define CLIP(x,min,max)    ( (x)>(max) ? (max) : ((x)<(min) ? (min) : (x)))
/********************         TYPE DEFINITIONS               ******************/
typedef unsigned char      BYTE;
typedef signed char        SBYTE;
typedef unsigned char      U8;
typedef signed char        S8;

typedef unsigned short     U16;
typedef signed short       S16;
typedef unsigned short     WORD;
typedef signed short       SWORD;
typedef unsigned short     UCHAR16; // for 16-bit UNICODE characters

typedef unsigned int       U32;
typedef signed int         S32;
typedef unsigned int       DWORD;
typedef signed int         SDWORD;

typedef unsigned long long U64;
typedef signed long long   S64;

typedef unsigned char      BOOLEAN;

/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/

#endif      // #ifndef TYPES_H
