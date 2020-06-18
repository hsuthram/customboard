/*******************************************************************************
*           Copyright (c) 2011 Johnson Outdoors Marine Electronics, Inc.       *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: Simple UDP Protocol Outbound Message/Packet Handler 

      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      + This software is the proprietary property of:                   +
      +                                                                 +
      +  Johnson Outdoors Marine Electronics, Inc.                      +
      +                                                                 +
      +  1220 Old Alpharetta Rd Ste 340   1 Humminbird Lane             +
      +  Alpharetta, GA  30005            Eufaula, AL  36027            +
      +                                                                 +
      + Use, duplication, or disclosure of this material, in any form,  +
      + is forbidden without permission from Johnson Outdoors Marine    +
      + Electronics, Inc.                                               +
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

*******************************************************************************/


/********************           COMPILE FLAGS                ******************/
#ifndef T_SUDP_OUT_H
   #define T_SUDP_OUT_H
//#include "compile_flags.h"

/********************           INCLUDE FILES                ******************/   
#include "message.h"

/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
  GENERIC_MESSAGE   header;
  unsigned char *ucpMessage;
  U32 u32MessageLength;
  U32 u32Address;
  U32 u32Port;
  U32 u32Raw;
  U32 u32MultiPacket;
} MSG_SUDP_OUT_SEND;

/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/

#endif
