/*******************************************************************************
*           Copyright (c) 2008  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description:

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
#ifndef GMSG_INFO_H
   #define GMSG_INFO_H
#ifdef __cplusplus
   extern "C" {
#endif
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include "global.h"
#include "types.h"
#include "gmsg_addr.h"
#include "datatype.h"

/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
   U32         u32MsgSignature;
   U32         u32MsgId;
   U32         u32MsgFlags;
   U32         u32MsgSize;
   GMSG_ADDRESS SrcAddress;
   GMSG_ADDRESS DstAddress;
   U64         u64Token;
   U64         __u64ReplyToken;
} GMSG_HEADER;

typedef struct __tag_GMSG_INFO
{
   GMSG_HEADER     *pMsgHeader;
   const G_DATA_TYPE *pDataType;
   void            *pRawMsg;
   void            *pStructMsg;
   const G_DATA_TYPE *pRequestDataType;
   void            *pRequestMsg;
   BOOLEAN          bReceived;
   BOOLEAN          bReply;
   U32              u32UdpLength;
   U32              u32TimeoutInMs;
} GMSG_INFO;

/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/

#ifdef __cplusplus
   }
#endif
#endif //GMSG_INFO_H
