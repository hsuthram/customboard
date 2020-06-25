#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "types.h"
#include "fcid.h"
#include "lsvc.h"
//#include "datadict.h"
#include "h_database.h"
#include "l_mbox.h"
#include "l_mutex.h"

/********************              DEFINES                   ******************/
#define SIZE_OF_DATABASE        64
#define MAX_NUM_REGISTERED_MBOX 16
/********************               MACROS                   ******************/
#define WRITE_IF_PTR_NOT_NULL(p,x) {if ((p)!=NULL) *(p)=(x);}
/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
//  DB_CONDITION_FLAGS Condition;   /* Conditions whereupon message is sent */
  MBOX           Mailbox;     /* Mailbox to receive message */
} REGISTERED_MBOX;

typedef struct
{ 
  FOURCHARID         Id;            /* 4 character ID stored as a U32*/
  DB_DATA_TYPE       dbValue;       /* Raw value - maybe cast to anything else */
  DATA_FORMAT        eFormat;       /* native format type */
//  DB_ATTRIBUTES      ExtraData;     /* Details of units and textual description */
//  time_t             TimeStamp;     /* System time that dbValue was updated */
//  U32                u32Priority;   /* Used for determining if new data should overwrite current entry */
  MBOX               mboxOwner;     /* The mailbox of the supplier of the data (ooohh - dangerous!) */
  REGISTERED_MBOX    mboxNotifyList[MAX_NUM_REGISTERED_MBOX];     /* Linked list of mailboxes for when changed etc.*/
//  DB_CONDITION_FLAGS Condition;
//  BOOLEAN            bCreated;      /* TRUE only if a value has been stored, FALSE if only a task has registered for data yet it hasn't been created yet*/
} DATABASE_ELEMENT;

typedef struct
{
   FOURCHARID  fcid;
   DATA_FORMAT eFormat;
} DB_N_TYPE;

typedef struct
{
   FOURCHARID   Id;            /* 4 character ID stored as a U32*/
   DB_DATA_TYPE dbValue;       /* Raw value - maybe cast to anything else */
   DATA_FORMAT  eFormat;       /* native format type */
} DB_SAVE;

/********************          LOCAL VARIABLES               ******************/
PRIVATE DATABASE_ELEMENT s_DataBase[SIZE_OF_DATABASE];
PRIVATE DB_SAVE          s_DBSave[SIZE_OF_DATABASE];
PRIVATE U32              s_u32SavedDBItems = 0;
PRIVATE BOOLEAN          s_DBExistedAtStartup = TRUE;
/********************          DEFINES                       ******************/
#define WRITE_IF_PTR_NOT_NULL(p,x) {if ((p)!=NULL) *(p)=(x);}
/********************          GLOBAL VARIABLES              ******************/
PRIVATE char s_db_filename[] = "/home/root/dbfile.dat";
PRIVATE const DB_N_TYPE fcidList[SIZE_OF_DATABASE] = {{MBIW, DF_UNSIGNED32},  //in/out water 3 seconds
                                                      {MBSW, DF_UNSIGNED32},  //in water
                                                      {MSWR, DF_UNSIGNED32},  //software update ready
                                                      {SBRD, DF_FLOAT},       //down ping range - real value when in auto mode
                                                      {SBRF, DF_FLOAT},       //forward ping range - real value when in auto mode
                                                      {SBMR, DF_UNSIGNED32},  //actual mode as reported by the IMU (gyro)
                                                      {SEDU, DF_UNSIGNED32},  //burn-in date code
                                                      {SEIU, DF_UNSIGNED32},  //post burn-in line number
                                                      {SENU, DF_UNSIGNED32},  //sequential unit id
                                                      {SM1D, DF_FLOAT},       //mode 1 (full) depth range
                                                      {SM1F, DF_FLOAT},       //mode 1 (full) forward range
                                                      {SM2D, DF_FLOAT},       //mode 2 (down) depth range
	                                              {SM2L, DF_FLOAT},       //chirp freq lower limit 
	                                              {SM2U, DF_FLOAT},       //chirp freq upper limit
                                                      {SM3D, DF_FLOAT},       //mode 3 (forward) depth range
                                                      {SM3F, DF_FLOAT},       //mode 3 (forward) forward range
                                                      {SM4F, DF_FLOAT},       //mode 4 (outlook) forward range
                                                      {SMBN, DF_UNSIGNED32},  //ping on/off
                                                      {SMDL, DF_UNSIGNED32},  //numerical model ID (in this case 119, found in t_ethernet.h in Pandora/Main)
                                                      {SMLM, DF_UNSIGNED32},  //user selected mode (auto/full/down/forward/outlook)
                                                      {SVER, DF_UNSIGNED32},  //version number
                                                      {YBIW, DF_UNSIGNED32},  //in/out water 3 seconds
                                                      {YBRD, DF_FLOAT},       //down ping range - real value when in auto mode
                                                      {YBRF, DF_FLOAT},       //forward ping range - real value when in auto mode
                                                      {YBMR, DF_UNSIGNED32},  //actual mode as reported by the IMU (gyro)
                                                      {YM1D, DF_FLOAT},       //mode 1 (full) depth range
                                                      {YM1F, DF_FLOAT},       //mode 1 (full) forward range
                                                      {YM2D, DF_FLOAT},       //mode 2 (down) depth range
	                                              {YM2L, DF_UNSIGNED32},  //chirp freq lower limit 
	                                              {YM2U, DF_UNSIGNED32},  //chirp freq upper limit
                                                      {YM3D, DF_FLOAT},       //mode 3 (forward) depth range
                                                      {YM3F, DF_FLOAT},       //mode 3 (forward) forward range
                                                      {YM4F, DF_FLOAT},       //mode 4 (outlook) forward range
                                                      {YMBN, DF_UNSIGNED32},  //ping on/off
                                                      {YMLM, DF_UNSIGNED32},  //user selected mode (auto/full/down/forward/outlook)
                                                      {YSWR, DF_UNSIGNED32},  //software update ready
                                                      {0, DF_ERROR},  //37
                                                      {0, DF_ERROR},  //38
                                                      {0, DF_ERROR},  //39
                                                      {0, DF_ERROR},  //40
                                                      {0, DF_ERROR},  //41
                                                      {0, DF_ERROR},  //42
                                                      {0, DF_ERROR},  //43
                                                      {0, DF_ERROR},  //44
                                                      {0, DF_ERROR},  //45
                                                      {0, DF_ERROR},  //46
                                                      {0, DF_ERROR},  //47
                                                      {0, DF_ERROR},  //48
                                                      {0, DF_ERROR},  //49
                                                      {0, DF_ERROR},  //50
                                                      {0, DF_ERROR},  //51
                                                      {0, DF_ERROR},  //52
                                                      {0, DF_ERROR},  //53
                                                      {0, DF_ERROR},  //54
                                                      {0, DF_ERROR},  //55
                                                      {0, DF_ERROR},  //56
                                                      {0, DF_ERROR},  //57
                                                      {0, DF_ERROR},  //58
                                                      {0, DF_ERROR},  //59
                                                      {0, DF_ERROR},  //60
                                                      {0, DF_ERROR},  //61
                                                      {0, DF_ERROR},  //62
                                                      {0, DF_ERROR},  //63
                                                      {0, DF_ERROR}}; //64
/********************              FUNCTIONS                 ******************/
PRIVATE void H_DATABASE_vPrintContents(U32 u32Option)
{
   U32 u32i;

   if ((u32Option == 0) || (u32Option == 1))
   {
      for (u32i = 0; u32i < s_u32SavedDBItems; u32i++)
      {
         printf("%x(%c%c%c%c): %d %d %d %f\n", s_DBSave[u32i].Id, 
                                               (U8)s_DBSave[u32i].Id,
                                               (U8)(s_DBSave[u32i].Id >> 8),
                                               (U8)(s_DBSave[u32i].Id >> 16),
                                               (U8)(s_DBSave[u32i].Id >> 24),
                                               s_DBSave[u32i].eFormat,
                                               s_DBSave[u32i].dbValue.u32Val,
                                               s_DBSave[u32i].dbValue.s32Val,
                                               s_DBSave[u32i].dbValue.fVal);
      }

      printf("\n");
   } 


   if ((u32Option == 0) || (u32Option == 2))
   {
      for (u32i = 0; u32i < SIZE_OF_DATABASE; u32i++)
      {
         U32 u32j;

         printf("%x(%c%c%c%c): %d %d %f %d\n", s_DataBase[u32i].Id, 
                                               (U8)s_DataBase[u32i].Id,
                                               (U8)(s_DataBase[u32i].Id >> 8),
                                               (U8)(s_DataBase[u32i].Id >> 16),
                                               (U8)(s_DataBase[u32i].Id >> 24),
                                               s_DataBase[u32i].dbValue.u32Val, 
                                               s_DataBase[u32i].dbValue.s32Val, 
                                               s_DataBase[u32i].dbValue.fVal,
                                               s_DataBase[u32i].eFormat);
#if 0
         printf("mbox: ");

         for (u32j = 0; u32j < MAX_NUM_REGISTERED_MBOX; u32j++)
         {
            printf("%d ", s_DataBase[u32i].mboxNotifyList[u32j].Mailbox);
         }

         printf("\n");
#endif
      }
   }
}

PRIVATE void H_DATABASE_vSetDefaults(void)
{
   U32 u32i;
   U32 u32SaveList = 0;

   for (u32i = 0; u32i < SIZE_OF_DATABASE; u32i++)
   {
      s_DataBase[u32i].Id = fcidList[u32i].fcid;
      s_DataBase[u32i].eFormat = fcidList[u32i].eFormat;

      if ((s_DataBase[u32i].eFormat != DF_ERROR) && ((U8)s_DataBase[u32i].Id == 'S'))
      {
         s_DBSave[u32SaveList].Id = fcidList[u32i].fcid;
         s_DBSave[u32SaveList].eFormat = fcidList[u32i].eFormat;
	 u32SaveList++;
      }
   }

   s_u32SavedDBItems = u32SaveList++;
   printf("[%d]%s: s_u32SavedDBItems %d each %d bytes\n", __LINE__, __FUNCTION__, s_u32SavedDBItems, sizeof(DB_SAVE));
   H_DATABASE_vPrintContents(2);
}

PRIVATE S32 H_DATABASE_s32FindDBIndex(FOURCHARID fcid)
{
   U32 u32i;

   for (u32i = 0; u32i < SIZE_OF_DATABASE; u32i++)
   {
      if (s_DataBase[u32i].Id == fcid)
      {
         return u32i;
      }
   }

   return -1;
}

PRIVATE BOOLEAN H_DATABASE_bRegisterMailBox(S32 s32Index, MBOX mbox)
{
   U32 u32i;

   L_MUTEX_LockWait(RES_DATABASE);

   for (u32i = 0; u32i < MAX_NUM_REGISTERED_MBOX; u32i++)
   {
      if (s_DataBase[s32Index].mboxNotifyList[u32i].Mailbox == mbox)
      {
         L_MUTEX_Unlock(RES_DATABASE);
         return TRUE;
      }

      if (s_DataBase[s32Index].mboxNotifyList[u32i].Mailbox == 0)
      {
         s_DataBase[s32Index].mboxNotifyList[u32i].Mailbox = mbox;
         L_MUTEX_Unlock(RES_DATABASE);
	 return TRUE;
      }
   }

   return FALSE; 
}

PRIVATE void H_DATABASE_vUpdateEntryInDatabase(S32 s32Index, DB_DATA_TYPE dbValue)
{
   if (s_DataBase[s32Index].eFormat > DF_ERROR)
   {
      switch (s_DataBase[s32Index].eFormat)
      {
         case DF_FLOAT:
            s_DataBase[s32Index].dbValue.fVal = dbValue.fVal;
	    break;
	 case DF_SIGNED32:
            s_DataBase[s32Index].dbValue.s32Val = dbValue.s32Val;
	    break;
	 case DF_UNSIGNED32:
	 default:
            s_DataBase[s32Index].dbValue.u32Val = dbValue.u32Val;
            break;
      }
   }
}

PRIVATE void H_DATABASE_vSendMsgToMailBox(S32 s32Index, MBOX Mailbox)
{
   MESSAGE_NEW_DATA *pMsg;
 
   if (s_DataBase[s32Index].eFormat > DF_ERROR)
   {
      printf("send to mailbox %d\n", Mailbox);
      pMsg = (MESSAGE_NEW_DATA *)malloc(sizeof(MESSAGE_NEW_DATA));
      pMsg->Header.Cmd = CMD_ID_NEW_DATA;
      pMsg->FourCharId = s_DataBase[s32Index].Id;

      switch (s_DataBase[s32Index].eFormat)
      {
         case DF_FLOAT:
            pMsg->dbValue.fVal = s_DataBase[s32Index].dbValue.fVal;
	    break;
         case DF_SIGNED32:
            pMsg->dbValue.s32Val = s_DataBase[s32Index].dbValue.s32Val;
	    break;
         case DF_UNSIGNED32:
         default:		 
            pMsg->dbValue.u32Val = s_DataBase[s32Index].dbValue.u32Val;
	    break;
      }

      L_MBOX_Send(Mailbox, (MSGHEADER *)pMsg, (PRIORITY)NORMAL_PRIORITY, 0);
   }
}

PRIVATE void H_DATABASE_vNotifyRegisteredMailboxes(S32 s32Index, MBOX excepThisOne)
{
   U32 u32i;

   for (u32i = 0; u32i < MAX_NUM_REGISTERED_MBOX; u32i++)
   {
      if ((s_DataBase[s32Index].mboxNotifyList[u32i].Mailbox > 0) && 
          (s_DataBase[s32Index].mboxNotifyList[u32i].Mailbox != excepThisOne))
      {
         H_DATABASE_vSendMsgToMailBox(s32Index, s_DataBase[s32Index].mboxNotifyList[u32i].Mailbox);
      }
      else
      {
         break;
      }
   }
}

PRIVATE BOOLEAN H_DATABASE_bDBExists(void)
{
   if (access(s_db_filename, R_OK | W_OK) == -1)
   {
      return FALSE;
   }
   else  //file exists - check size and entry names
   {
      FILE *fp = fopen(s_db_filename, "r");

      if (fp)
      {
	 struct stat filestat;
         int         u32i, u32Elements;
	 DB_SAVE     dbElement;
	 BOOLEAN     bValid = TRUE;

	 stat(s_db_filename, &filestat);

	 if ((filestat.st_size % sizeof(DB_SAVE)) != 0)
	 {
            printf("[%d]%s: file is corrupt, delete and start over\n", __LINE__, __FUNCTION__);
	    fclose(fp);
            unlink(s_db_filename);
            return FALSE;
	 }

	 u32Elements = filestat.st_size / sizeof(DB_SAVE);
	 printf("[%d]%s: dbfile size %ld, entries %d\n", __LINE__, __FUNCTION__, filestat.st_size, u32Elements);

	 for (u32i = 0; u32i < u32Elements; u32i++)
	 {
	    fread(&dbElement, 1, sizeof(DB_SAVE), fp);
	    
            if ((U8)dbElement.Id != 'S')
	    {
               bValid = FALSE;
	       break;
	    }
	 }

	 if (!bValid)
	 {
            printf("[%d]%s: file is corrupt, delete and start over\n", __LINE__, __FUNCTION__);
	    fclose(fp);
            unlink(s_db_filename);
            return FALSE;
	 }
      }
      else  //file exists but we can't read it, so delete it and start over
      {
         unlink(s_db_filename);
	 return FALSE;
      }

      return TRUE;
   }
}

GLOBAL void H_DATABASE_vSaveToFile(void)
{
   struct stat filestat;
   U32         u32i, u32Elements;
   FILE       *fp;

   /* lock so we don't update while we're saving */
   L_MUTEX_LockWait(RES_DATABASE);

   //double check the file before we save, it could be corrupt and need to be deleted
   stat(s_db_filename, &filestat);

   if ((filestat.st_size % sizeof(DB_SAVE)) != 0)
   {
      //bad file size, delete file
      unlink(s_db_filename);
   }
   else  //size is good, check number of entries
   {
      u32Elements = filestat.st_size / sizeof(DB_SAVE);

      if (u32Elements != s_u32SavedDBItems)
      {
         //current file either has too many or too few entries, delete it
         unlink(s_db_filename);
      }
   }

   fp = fopen(s_db_filename, "w");

   if (fp)
   {
      U32 u32i;

      for (u32i = 0; u32i < s_u32SavedDBItems; u32i++)
      {
         fwrite(&s_DBSave[u32i], sizeof(DB_SAVE), 1, fp);
      }

      fclose(fp);
   }

   /* unlock the database resource */
   L_MUTEX_Unlock(RES_DATABASE);
//   H_DATABASE_vPrintContents(1);
}

/******************************************************************************
 *
 *    Function: H_DATABASE_vInit
 *
 *    Args:    -none-
 *
 *    Return:  -none-
 *
 *    Purpose: Clear the pointer structure
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void H_DATABASE_vInit(void)
{
   memset(s_DataBase, 0, SIZE_OF_DATABASE*sizeof(DATABASE_ELEMENT));
   memset(s_DBSave, 0, SIZE_OF_DATABASE*sizeof(DB_SAVE));
   H_DATABASE_vSetDefaults();

   //see if the database file exists
   if (H_DATABASE_bDBExists())
   {
      FILE *fp = fopen(s_db_filename, "rw");

      if (fp)
      {
         struct stat filestat;
         U32         u32i, u32Elements;
         DB_SAVE    *pSavedDBItems;

         printf("[%d]%s: db exists\n", __LINE__, __FUNCTION__);
         stat(s_db_filename, &filestat);
         u32Elements = filestat.st_size / sizeof(DB_SAVE);
	 pSavedDBItems = malloc(u32Elements * sizeof(DB_SAVE));
	 fread(pSavedDBItems, sizeof(DB_SAVE), filestat.st_size, fp);

	 for (u32i = 0; u32i < u32Elements; u32i++)
	 {
#if 0
            printf("%x(%c%c%c%c): %d %d %d %f\n", pSavedDBItems[u32i].Id,
                                                  (U8)pSavedDBItems[u32i].Id,
                                                  (U8)(pSavedDBItems[u32i].Id >> 8),
                                                  (U8)(pSavedDBItems[u32i].Id >> 16),
                                                  (U8)(pSavedDBItems[u32i].Id >> 24),
                                                  pSavedDBItems[u32i].eFormat,
                                                  pSavedDBItems[u32i].dbValue.u32Val,
                                                  pSavedDBItems[u32i].dbValue.s32Val,
                                                  pSavedDBItems[u32i].dbValue.fVal);
#endif

            if (!H_DATABASE_bUpdateValue(pSavedDBItems[u32i].Id, pSavedDBItems[u32i].dbValue, 0))
	    {
               printf("[%d]%s: saved db %c%c%c%c does not exist\n", __LINE__, __FUNCTION__, 
                      (U8)pSavedDBItems[u32i].Id, (U8)(pSavedDBItems[u32i].Id >> 8),
                      (U8)(pSavedDBItems[u32i].Id >> 16), (U8)(pSavedDBItems[u32i].Id >> 24));
	    }
         }

	 free(pSavedDBItems);
	 fclose(fp);
         H_DATABASE_vPrintContents(2);
      }
   }
   else  //file does not exist, go ahead and write it with the default contents
   {
      s_DBExistedAtStartup = FALSE;
      H_DATABASE_vSaveToFile();
   }
}

GLOBAL BOOLEAN H_DATABASE_bValidDBAtStartup(void)
{
   return s_DBExistedAtStartup;
}

GLOBAL BOOLEAN H_DATABASE_bRegisterForData(FOURCHARID fcid, MBOX mbox)
{
   S32 s32Index = H_DATABASE_s32FindDBIndex(fcid);

   if (s32Index >= 0)
   {
      H_DATABASE_bRegisterMailBox(s32Index, mbox);
//      H_DATABASE_vPrintContents(2);
   }

   return FALSE;
}

GLOBAL BOOLEAN H_DATABASE_bUpdateValue(FOURCHARID fcid,
                                       DB_DATA_TYPE dbValue,
				       MBOX exceptThisOne)
{
   S32 s32Index = H_DATABASE_s32FindDBIndex(fcid);

   if (s32Index >= 0)
   {
      /* Maybe we should lock the database resource here */
      L_MUTEX_LockWait(RES_DATABASE);

      /* Update the new data into the database */
      H_DATABASE_vUpdateEntryInDatabase(s32Index, dbValue);

      /* Now tell all who registered about the change/update etc. */
      H_DATABASE_vNotifyRegisteredMailboxes(s32Index, exceptThisOne); 

      //update the saved database file
      if ((U8)fcid == 'S')
      {
         U32 u32i;

	 for (u32i = 0; u32i < s_u32SavedDBItems; u32i++)
         {
            if (s_DBSave[u32i].Id == fcid)
	    {
               if (s_DBSave[u32i].eFormat > DF_ERROR)
               {
                  switch (s_DBSave[u32i].eFormat)
                  {
                     case DF_FLOAT:
                        s_DBSave[u32i].dbValue.fVal = dbValue.fVal;
                        break;
                     case DF_SIGNED32:
                        s_DBSave[u32i].dbValue.s32Val = dbValue.s32Val;
                        break;
                     case DF_UNSIGNED32:
                     default:
                        s_DBSave[u32i].dbValue.u32Val = dbValue.u32Val;
                        break;
                  }
               }

	       break;
	    }
	 }
      }

      /* Safe to unlock the database resource now */
      L_MUTEX_Unlock(RES_DATABASE);
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

GLOBAL BOOLEAN H_DATABASE_bUpdateValueU32(FOURCHARID FourCharId, U32 Value)
{
   DB_DATA_TYPE dbData;

   dbData.u32Val = Value;
   return H_DATABASE_bUpdateValue(FourCharId, dbData, 0);
}

GLOBAL BOOLEAN H_DATABASE_bUpdateValueFloat(FOURCHARID FourCharId, float Value)
{
   DB_DATA_TYPE dbData;

   dbData.fVal = Value;
   return H_DATABASE_bUpdateValue(FourCharId, dbData, 0);
}

GLOBAL BOOLEAN H_DATABASE_bUpdateValueS32(FOURCHARID FourCharId, S32 Value)
{
   DB_DATA_TYPE dbData;

   dbData.s32Val = Value;
   return H_DATABASE_bUpdateValue(FourCharId, dbData, 0);
}

GLOBAL BOOLEAN H_DATABASE_bUpdateValueU32Except(FOURCHARID fcid, U32 u32Value, MBOX exceptThisOne)
{
   DB_DATA_TYPE dbData;

   dbData.u32Val = u32Value;
   return H_DATABASE_bUpdateValue(fcid, dbData, exceptThisOne);
}

GLOBAL BOOLEAN H_DATABASE_bFetch(const FOURCHARID fcid, DB_DATA_TYPE * const pdbDestination)
{
   S32 s32Index = H_DATABASE_s32FindDBIndex(fcid);

//   printf("[%d]%s: %x(%c%c%c%c)(%d)\n", __LINE__, __FUNCTION__, fcid, (U8)fcid, (U8)(fcid >> 8), (U8)(fcid >> 16), (U8)(fcid >> 24), s32Index);

   if (s32Index >= 0)
   {
      DATABASE_ELEMENT *pDbItem = &s_DataBase[s32Index];

      /* Lock the database so that no one changes it mid search */
      L_MUTEX_LockWait(RES_DATABASE);
      WRITE_IF_PTR_NOT_NULL(pdbDestination, pDbItem->dbValue);
   }
   else
   {
      return(FALSE);
   } 

   /* unlock after we're done */
   L_MUTEX_Unlock(RES_DATABASE);
   return(TRUE);
}

GLOBAL BOOLEAN H_DATABASE_bFetchU32(const FOURCHARID FourChardId, U32 * const pValue)
{
   return H_DATABASE_bFetch(FourChardId, (DB_DATA_TYPE *)pValue);
}   

GLOBAL BOOLEAN H_DATABASE_bFetchBoolean(const FOURCHARID FourChardId, BOOLEAN * const pValue)
{
   DB_DATA_TYPE dbData;

   if (H_DATABASE_bFetch(FourChardId, &dbData))
   {
      *pValue = (BOOLEAN)dbData.u32Val;
      return TRUE;	   
   }
   else
   {
      return FALSE;	   
   }
}   

GLOBAL BOOLEAN H_DATABASE_bFetchFloat(const FOURCHARID FourChardId, float * const pValue)
{
   return H_DATABASE_bFetch(FourChardId, (DB_DATA_TYPE *)pValue);
}   
