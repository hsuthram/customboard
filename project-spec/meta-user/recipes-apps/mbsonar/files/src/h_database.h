#ifndef H_DATABASE_H
#define H_DATABASE_H

#include "message.h"
#include "l_mbox.h"

/********************         TYPE DEFINITIONS               ******************/
typedef union
{
//   U32    RawData;     /* 32 bits of raw data - this value must cover all bits! - INTERNAL USE ONLY */
   U32    u32Val;      /* Generic 32bit value - used for all unsigned integers */
   S32    s32Val;      /* Generic 32bit value - used for all integers */
   float  fVal;        /* Most analogue values - used for all floats */
//   void   *vpPointer;  /* void pointer - includes pointers to functions */
} DB_DATA_TYPE; /* Fundamental data size held in database */

typedef U32 FOURCHARID;

typedef struct
{
  GENERIC_MESSAGE Header;     /* This will always contain an element called Cmd */
  FOURCHARID      FourCharId; /* 4 char identifier of the databse item */
  DB_DATA_TYPE    dbValue;    /* The latest value */
  BOOLEAN         bValid;     /* Set if the data is not invalid */
} MESSAGE_NEW_DATA;
/********************        FUNCTION PROTOTYPES             ******************/
extern void    H_DATABASE_vInit(void);
extern void    H_DATABASE_vSaveToFile(void);
extern BOOLEAN H_DATABASE_bValidDBAtStartup(void);
extern BOOLEAN H_DATABASE_bRegisterForData(FOURCHARID fcid, MBOX mbox);
extern BOOLEAN H_DATABASE_bUpdateValueU32(FOURCHARID FourCharId, U32 Value);
extern BOOLEAN H_DATABASE_bUpdateValueFloat(FOURCHARID FourCharId, float Value);
extern BOOLEAN H_DATABASE_bUpdateValueS32(FOURCHARID FourCharId, S32 Value);
extern BOOLEAN H_DATABASE_bUpdateValue(FOURCHARID fcid, DB_DATA_TYPE dbValue, MBOX exceptThisOne);
extern BOOLEAN H_DATABASE_bFetchU32(const FOURCHARID FourChardId, U32 * const pValue);
extern BOOLEAN H_DATABASE_bFetchBoolean(const FOURCHARID FourChardId, BOOLEAN * const pValue);
extern BOOLEAN H_DATABASE_bFetchFloat(const FOURCHARID FourChardId, float * const pValue);
extern BOOLEAN H_DATABASE_bUpdateValueU32Except(FOURCHARID fcid, U32 u32Value, MBOX exceptThisOne);
#endif
