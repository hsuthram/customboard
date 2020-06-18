/*******************************************************************************
*           Copyright (c) 2009  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
*******************************************************************************/

/**
 * @file database.c 
 *  
 * Database interface functions with standard database message handlers. Allows 
 * storage of data primitives as well as complex structures. See datatype.h 
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
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include <stdio.h>
#include <stdlib.h>
//#include "matrixOS.h"
//#include "nomalloc.h"
//#include "model.h"
#include "error.h"
#include "datatype.h"
#include "database_msg.h"
#include "gmsg.h"
#include "u_gmsg.h"
//#if MODEL_HAS_NETWORKING
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/

/********************        FUNCTION PROTOTYPES             ******************/
PRIVATE BOOLEAN      DATABASE_bClientRequest(DATABASE_CLIENT *pDatabaseClient, GMSG_ID_TYPE eMsgId,
                                             const G_DATA_TYPE *pDataType, void *pData, U64 u64Token);

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/
DT_STRUCT_TYPE(GLOBAL, D_GMSG_DATABASE_FETCH, 1, NULL, NULL, sizeof(GMSG_DATABASE_FETCH), NULL, DT_PTR_TO_STRING);
DT_STRUCT_TYPE(GLOBAL, D_GMSG_DATABASE_REGISTER, 1, NULL, NULL, sizeof(GMSG_DATABASE_REGISTER), NULL, DT_PTR_TO_STRING, DT_UNSIGNED32);
DT_STRUCT_TYPE(GLOBAL, D_GMSG_DATABASE_UNREGISTER, 1, NULL, NULL, sizeof(GMSG_DATABASE_UNREGISTER), NULL, DT_PTR_TO_STRING);
DT_STRUCT_TYPE(GLOBAL, D_GMSG_DATABASE_UPDATE, 1, NULL, NULL, sizeof(GMSG_DATABASE_UPDATE), NULL, DT_PTR_TO_STRING, DT_BUFFER);
DT_STRUCT_TYPE(GLOBAL, D_GMSG_DATABASE_NEW_DATA, 1, NULL, NULL, sizeof(GMSG_DATABASE_NEW_DATA), NULL, DT_PTR_TO_STRING, DT_UNSIGNED32, DT_BUFFER);

/********************              FUNCTIONS                 ******************/

/******************************************************************************/
/**
 * Setup Database client - C interface
 * 
 * @param pDatabaseClient  client side database reference to initialize
 * @param pDatabaseAddress address of foreign database
 * @param pMyMailbox       client mailbox
 * 
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN DATABASE_bClientInitDatabase(DATABASE_CLIENT *pDatabaseClient, GMSG_ADDRESS *pDatabaseAddress, MBOX Mailbox)
{
   pDatabaseClient->DatabaseAddress = *pDatabaseAddress;
   pDatabaseClient->Mailbox = Mailbox;
   return(TRUE);
}

/******************************************************************************/
/**
 * Register for data on client side of database 
 *  
 * note: must handle GMSG_ID_DATABASE_NEW_DATA in message map for initial 
 * response and subsequent updates 
 * 
 * @param pDatabaseClient Reference to database client
 * @param pcDataId        data id to register
 * @param bChanged        detect when data changes
 * @param bInvalid        detect when data becomes invalid (times out)
 * @param bUpdated        detect when data is updated
 * @param u64Token        user token to compare in responce 
 *                        (and changes/timeouts/updates)
 * 
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN DATABASE_bClientRegisterData(DATABASE_CLIENT *pDatabaseClient, const char *pcDataId, BOOLEAN bChanged,
                                            BOOLEAN bInvalid, BOOLEAN bUpdated, U64 u64Token)
{
   GMSG_DATABASE_REGISTER MsgDatabaseRegister;

   MsgDatabaseRegister.pDataId           = pcDataId;
   MsgDatabaseRegister.u32ConditionFlags = (bChanged ? DATABASE_CONDITION_CHANGED : 0) | (bInvalid ? DATABASE_CONDITION_INVALID : 0 ) |
                                           (bUpdated ? DATABASE_CONDITION_UPDATED : 0);
   return(DATABASE_bClientRequest(pDatabaseClient, GMSG_ID_DATABASE_REGISTER, D_GMSG_DATABASE_REGISTER,
                                  (void*)&MsgDatabaseRegister, u64Token));
}

/******************************************************************************/
/**
 * Unregister for data on client side of database
 *  
 * note: must handle GMSG_ID_STATUS in message map for status of unregistration 
 * request
 *  
 * @param pDatabaseClient Reference to database client
 * @param pcDataId        database item to unregister 
 * @param u64Token        user token to compare in responce 
 * 
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN DATABASE_bClientUnregisterData(DATABASE_CLIENT *pDatabaseClient, const char *pcDataId, U64 u64Token)
{
   GMSG_DATABASE_UNREGISTER MsgDatabaseUnregister;

   MsgDatabaseUnregister.pDataId = pcDataId;
   return(DATABASE_bClientRequest(pDatabaseClient, GMSG_ID_DATABASE_UNREGISTER, D_GMSG_DATABASE_UNREGISTER,
                                  (void*)&MsgDatabaseUnregister, u64Token));
}

/******************************************************************************/
/**
 * Fetch data from client side of database 
 *  
 * note: must handle GMSG_ID_STATUS in message map for status of fetch request 
 *  
 * @param pDatabaseClient Reference to database client
 * @param pcDataId        database item to fetch
 * @param u64Token        user token to compare in responce
 * 
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN DATABASE_bClientFetchData(DATABASE_CLIENT *pDatabaseClient, const char *pcDataId, U64 u64Token)
{
   GMSG_DATABASE_FETCH MsgDatabaseFetch;

   MsgDatabaseFetch.pDataId = pcDataId;
   return(DATABASE_bClientRequest(pDatabaseClient, GMSG_ID_DATABASE_FETCH, D_GMSG_DATABASE_FETCH,
                                  (void*)&MsgDatabaseFetch, u64Token));
}

/******************************************************************************/
/**
 * Update a database item from client
 *  
 * Database item must already exist and be initialized on the server side 
 *  
 * must handle GMSG_ID_STATUS to check if update was successful
 * 
 * @param pDatabaseClient Reference to database client
 * @param pcDataId        data id to update
 * @param pDataType       type of data being updated
 * @param pData           void pointer to data
 * @param u64Token        user token to compare in response
 * 
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN DATABASE_bClientUpdateData(DATABASE_CLIENT *pDatabaseClient, const char *pcDataId,
                                          const G_DATA_TYPE *pDataType, void *pData, U64 u64Token)
{
   GMSG_DATABASE_UPDATE MsgDatabaseUpdate;
   int nRemaining;

   MsgDatabaseUpdate.pDataId      = pcDataId;
   MsgDatabaseUpdate.Buffer.nSize = 0;
   MsgDatabaseUpdate.Buffer.pData = NULL;
   nRemaining = 0;
   if (0 > pDataType->pfSerialize(pData, &MsgDatabaseUpdate.Buffer.pData, &nRemaining,
                                  &MsgDatabaseUpdate.Buffer.nSize, pDataType))
   {
      printf("DATABASE_bClientUpdateData: Error Serializing Data for %s", pcDataId);
      return(FALSE);
   }
   if(!DATABASE_bClientRequest(pDatabaseClient, GMSG_ID_DATABASE_UPDATE, D_GMSG_DATABASE_UPDATE,
                               (void*)&MsgDatabaseUpdate, u64Token))
   {
      printf("DATABASE_bClientUpdateData: Error Processing Request for %s", pcDataId);
      free(MsgDatabaseUpdate.Buffer.pData);
      return(FALSE);
   }
   free(MsgDatabaseUpdate.Buffer.pData);
   return(TRUE);
}

/******************************************************************************/
/**
 * Deserializes data buffer inside a new data message 
 *  
 * note: Call DATABASE_bFreeNewData when finished
 * 
 * @param pNewDataMsg New Data Message
 * @param ppStruct    address of ptr to struct to be deserialized
 * @param pDataType   data type of new data
 * 
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN DATABASE_bGetNewData(GMSG_DATABASE_NEW_DATA *pNewDataMsg, void **ppStruct, const G_DATA_TYPE *pDataType)
{
   int nSize, nRemaining;

   *ppStruct = NULL;
   if (pNewDataMsg->Buffer.nSize == 0 || pNewDataMsg->Buffer.pData == NULL)
   {
      return(FALSE);
   }
   nSize = 0;
   nRemaining = pNewDataMsg->Buffer.nSize;
   if (0 > pDataType->pfDeserialize(ppStruct, pNewDataMsg->Buffer.pData, &nRemaining, &nSize, pDataType))
   {
      printf("DATABASE_bGetNewData: Deserialization failed");
      return(FALSE);
   }
   return(TRUE);
}

/******************************************************************************/
/**
 * Frees allocated struct data from DATABASE_bGetNewData
 * 
 * @param pNewDataMsg New Data Message
 * @param pStruct     ptr to struct to be freed
 * @param pDataType   data type of new data
 * 
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN DATABASE_bFreeNewData(GMSG_DATABASE_NEW_DATA *pNewDataMsg, void *pStruct, const G_DATA_TYPE *pDataType)
{
   if (pNewDataMsg->Buffer.nSize == 0 || pNewDataMsg->Buffer.pData == NULL || pStruct == NULL)
   {
      return(FALSE);
   }
   if (0 > pDataType->pfDeallocate(pStruct, TRUE, pDataType))
   {
      printf("DATABASE_bFreeNewData: Deallocation failed");
      return(FALSE);
   }
   return(TRUE);
}

/******************************************************************************/
/**
 * 
 * 
 * @param pDatabaseClient 
 * @param eMsgId 
 * @param pDataType 
 * @param pData 
 * @param u64Token 
 * 
 * @return BOOLEAN 
 */
/******************************************************************************/
PRIVATE BOOLEAN DATABASE_bClientRequest(DATABASE_CLIENT *pDatabaseClient, GMSG_ID_TYPE eMsgId,
                                        const G_DATA_TYPE *pDataType, void *pData, U64 u64Token)
{
   return(U_GMSG_bRequest(pDatabaseClient->Mailbox, &pDatabaseClient->DatabaseAddress, eMsgId, pDataType, pData, u64Token));
}
//#endif //MODEL_HAS_NETWORKING
