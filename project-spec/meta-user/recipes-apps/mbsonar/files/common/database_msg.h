/*******************************************************************************
*           Copyright (c) 2008  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: Database interface functions with standard database
                        message handlers. Allows storage of data primitives
                        as well as complex structures. See datatype.h

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
#ifndef DATABASE_H
   #define DATABASE_H
#ifdef __cplusplus
   extern "C" {
#endif
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include "global.h"
#include "types.h"
#include "datatype.h"
#include "gmsg.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define DEFAULT_DATABASE_HASH          53

#define VARIABLE_DATA_SIZE             ((U32)-1)      /*requires a serialization function*/

#define NO_TIMEOUT                     0xffffffffUL

#define DATABASE_CONDITION_CHANGED     0x00000001
#define DATABASE_CONDITION_INVALID     0x00000002
#define DATABASE_CONDITION_UPDATED     0x00000004

#define DATABASE_TIMER_PERIOD          1000 //1 second

/********************               MACROS                   ******************/
#define ON_GMSG_DATABASE_FETCH(handler, database) \
           ON_MESSAGE(GMSG_ID_DATABASE_FETCH, handler, D_GMSG_DATABASE_FETCH, database, NULL)
#define ON_GMSG_DATABASE_REGISTER(handler, database) \
           ON_MESSAGE(GMSG_ID_DATABASE_REGISTER, handler, D_GMSG_DATABASE_REGISTER, database, NULL)
#define ON_GMSG_DATABASE_UNREGISTER(handler, database) \
           ON_MESSAGE(GMSG_ID_DATABASE_UNREGISTER, handler, D_GMSG_DATABASE_UNREGISTER, database, NULL)
#define ON_GMSG_DATABASE_UPDATE(handler, database) \
           ON_MESSAGE(GMSG_ID_DATABASE_UPDATE, handler, D_GMSG_DATABASE_UPDATE, database, NULL)
#define ON_GMSG_DATABASE_NEW_DATA(handler) \
           ON_MESSAGE(GMSG_ID_DATABASE_NEW_DATA, handler, D_GMSG_DATABASE_NEW_DATA, NULL, NULL)

/********************         TYPE DEFINITIONS               ******************/

typedef struct
{
   GMSG_ADDRESS   DatabaseAddress;
   MBOX           Mailbox;
} DATABASE_CLIENT;

typedef struct
{
   const char *pDataId;
} GMSG_DATABASE_FETCH;

typedef struct
{
   const char *pDataId;
   U32         u32ConditionFlags;
} GMSG_DATABASE_REGISTER;

typedef struct
{
   const char *pDataId;
} GMSG_DATABASE_UNREGISTER;

typedef struct
{
   const char *pDataId;
   C_BUFFER    Buffer;
} GMSG_DATABASE_UPDATE;

typedef struct
{
   const char *pDataId;
   U32         bValid;
   C_BUFFER    Buffer;
} GMSG_DATABASE_NEW_DATA;


/********************        FUNCTION PROTOTYPES             ******************/

//client side functions
extern BOOLEAN DATABASE_bClientInitDatabase(DATABASE_CLIENT *pDatabaseClient, GMSG_ADDRESS *pDatabaseAddress,
                                            MBOX Mailbox);
extern BOOLEAN DATABASE_bClientRegisterData(DATABASE_CLIENT *pDatabaseClient, const char *pcDataId, BOOLEAN bChanged,
                                            BOOLEAN bInvalid, BOOLEAN bUpdated, U64 u64Token);
extern BOOLEAN DATABASE_bClientUnregisterData(DATABASE_CLIENT *pDatabaseClient, const char *pcDataId, U64 u64Token);
extern BOOLEAN DATABASE_bClientFetchData(DATABASE_CLIENT *pDatabaseClient, const char *pcDataId, U64 u64Token);
extern BOOLEAN DATABASE_bClientUpdateData(DATABASE_CLIENT *pDatabaseClient, const char *pcDataId,
                                          const G_DATA_TYPE *pDataType, void *pData, U64 u64Token);
extern BOOLEAN DATABASE_bGetNewData(GMSG_DATABASE_NEW_DATA *pNewDataMsg, void **ppStruct, const G_DATA_TYPE *pDataType);
extern BOOLEAN DATABASE_bFreeNewData(GMSG_DATABASE_NEW_DATA *pNewDataMsg, void *pStruct, const G_DATA_TYPE *pDataType);


/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/* message structures */
DT_EXTERN_TYPE(D_GMSG_DATABASE_FETCH);
DT_EXTERN_TYPE(D_GMSG_DATABASE_REGISTER);
DT_EXTERN_TYPE(D_GMSG_DATABASE_UNREGISTER);
DT_EXTERN_TYPE(D_GMSG_DATABASE_UPDATE);
DT_EXTERN_TYPE(D_GMSG_DATABASE_NEW_DATA);

/********************              FUNCTIONS                 ******************/
#ifdef __cplusplus
   }
#endif
#endif      // DATABASE_H
