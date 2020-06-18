/*******************************************************************************
*           Copyright (c) 2010  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
*******************************************************************************/

/**
 * @file t_gmsg.h
 *
 */

/*******************************************************************************

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
#ifndef T_GMSG_H
   #define T_GMSG_H
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include "global.h"
#include "types.h"
#include "gsocket.h"
#include "gmsg.h"
#include "datatype.h"
#include "message.h"
#include "u_gmsg.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/

// CMD_ID_GMSG_NEW_MESSAGE
//    A socket is ready for receiving data
typedef struct tag_MSG_GMSG_NEW_MESSAGE
{
   GENERIC_MESSAGE   header;
   MBOX              Mailbox;
   int               nSocket;
} MSG_GMSG_NEW_MESSAGE;

// CMD_ID_GMSG_NEW_MESSAGE_EXT
//    A message is available from an external source
typedef struct tag_MSG_GMSG_NEW_MESSAGE_EXT
{
   GENERIC_MESSAGE   header;
   MBOX              Mailbox;
   unsigned char     *cpMessageBody;
   U32               u32MessageLength;
   BOOLEAN           bIsRaw;
} MSG_GMSG_NEW_MESSAGE_EXT;

/********************        FUNCTION PROTOTYPES             ******************/
extern void T_GMSG_vMain(void);
extern void T_GMSG_vSocketMain(void);
extern BOOLEAN T_GMSG_bGetMailboxAddress(GMSG_ADDRESS *pAddress, MBOX Mailbox);

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/
#endif //T_GMSG_H
