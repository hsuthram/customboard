/*******************************************************************************
*           Copyright (c) 2008  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: message address structure

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
#ifndef GMSG_ADDR_H
   #define GMSG_ADDR_H
#ifdef __cplusplus
   extern "C" {
#endif
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include "global.h"
#include "types.h"
#include "datatype.h"

/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define GMSG_ADDRESS_SOCKET       0
#define GMSG_ADDRESS_QUEUE        1

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
   U32 u32Address;
   U32 u32Port;
   U32 u32Unused1;
   U32 u32Unused2;
} GSOCKET_ADDRESS;

typedef char QUEUE_ADDRESS[16];

typedef union
{
   GSOCKET_ADDRESS Socket;
   QUEUE_ADDRESS   Queue;
} GMSG_ADDRESS_U;

typedef U32 GMSG_ADDRESS_TYPE;

typedef struct
{
   GMSG_ADDRESS_TYPE MsgAddressType;
   GMSG_ADDRESS_U    uAddr;
} GMSG_ADDRESS;

/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/
DT_EXTERN_TYPE(D_GMSG_ADDRESS);

/********************              FUNCTIONS                 ******************/
#ifdef __cplusplus
   }
#endif
#endif //GMSG_ADDR_H
