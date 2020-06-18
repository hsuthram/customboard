/*******************************************************************************
*           Copyright (c) 2008  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: message request information structures

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
#ifndef GMSG_REQ_H
   #define GMSG_REQ_H
#ifdef __cplusplus
   extern "C" {
#endif
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include "global.h"
#include "types.h"
#include "gmsg_addr.h"
#include "mutex.h"
#include "datatype.h"

/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define GMSG_REQ_TIMEOUT       3000

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
   U32               u32MsgId;
   S32               s32RequestTimeout;
   U64               u64Token;
   U64               __u64ReplyToken;
   const  G_DATA_TYPE *pDataType;
   void             *pData;
} GMSG_REQUEST;

typedef struct __tag_GMSG_REQUEST_NODE
{
   GMSG_REQUEST MsgRequest;
   struct __tag_GMSG_REQUEST_NODE *pNext;
} GMSG_REQUEST_NODE;

typedef struct
{
   GMUTEX             RequestMutex;
   GMSG_REQUEST_NODE *pHead;
} GMSG_REQUEST_INFO;

/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/
#ifdef __cplusplus
   }
#endif
#endif //GMSG_REQ_H
