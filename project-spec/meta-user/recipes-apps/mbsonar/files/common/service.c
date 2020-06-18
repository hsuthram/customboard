/*******************************************************************************
*           Copyright (c) 2008  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: Functions to advertise and search for system services.
                        Also defines classes of services and the socket port
                        ranges for services.

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
#ifdef __RTXC__
//   #include "matrixOS.h"
//   #include "nomalloc.h"
//   #include "model.h"
//   #include "u_utils.h"
#else
   #include "global.h"
   #include "types.h"
#endif
#include "mutex.h"
#include "service.h"
#include "gsocket.h"
#include <slp.h>
#ifdef linux
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netdb.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>
#else
   #ifndef USE_LWIP_STACK
      #define USE_LWIP_STACK 0
   #endif
   #if USE_LWIP_STACK
      #include "lwip/sockets.h"
   #else
      #include "fns_bsdsockapi.h"
      #include "u_fusion_port.h"
   #endif
#endif
//#include "t_slpd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "error.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define SERVICE_INFO_ARRAY_GROWTH      4
#define NAMING_AUTHORITY               "techsonic"
#ifndef linux
   #define  strcasecmp                    stricmp
#endif

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
   SERVICE_SEARCH_INFO *pSearchInfo;
   SLPError             err;
   int                  nAllocated;
}SERVICE_CALLBACK_INFO;

typedef struct
{
   char     *pcAttributes;
   SLPError  err;
}SERVICE_ATTR_CALLBACK_INFO;

typedef struct
{
   char       *pcServiceTypes;
   SLPError    err;
}SERVICE_TYPE_CALLBACK_INFO;

/********************        FUNCTION PROTOTYPES             ******************/

PRIVATE BOOLEAN    SERVICE_bMakeURL(char **ppcURL, const char *pcService, const char *pcPath,
                                    const char *pcAddr, U16 u16Port);
PRIVATE void       SERVICE_vRegCallback(SLPHandle hSLP, SLPError errCode, void *pvCookie);
PRIVATE SLPBoolean SERVICE_bTypeCallback(SLPHandle hSLP, const char* pSrvTypes, SLPError errCode, void* pvCookie);
PRIVATE SLPBoolean SERVICE_bSearchCallback(SLPHandle hSLP, const char* pcSrvURL,
                                           unsigned short sLifetime, SLPError errCode, void *pvCookie);
PRIVATE SLPBoolean SERVICE_bAttributeCallback(SLPHandle hSLP, const char* pcAttrList,
                                              SLPError errCode, void *pvCookie);

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

//service names used for debugging services/messaging and in service registration
GLOBAL const char * const s_cServiceClass[SERVICE_CLASS_COUNT] =
{
   "Default",
   "System",
   "Service",
   "Database",
   "Serial",
   "Sonar",
   "Xsonar",
   "Nmea",
   "Gps",
   "Ais",
   "Compass",
   "Autopilot",
   "Chart",
   "Nav",
   "Twr",
   "Radar",
   "Weather",
   "Log",
   "Socketserver",
   "Video",
   "Pane",
   "Test",
   "HBNmea",
   "HBTemp",
   "HBSonar",
   "HBAlarm",
   "HBNav",
   "HBMCast",
   "iPilot",
   "Bluetooth",
   "FCID_Server",
};

/********************              FUNCTIONS                 ******************/
/******************************************************************************
 *
 *    Function: SERVICE_bOpenServiceHandle
 *
 *    Args:    none
 *
 *    Return:  SERVICE_HANDLE - SERVICE_HANDLE_INVALID on error
 *
 *    Purpose: Open a Service handle
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL SERVICE_HANDLE SERVICE_hOpenServiceHandle(void)
{
   SERVICE_HANDLE hSLP;

//   if (T_SLPD_bReady())
   {
      if (SLP_OK == SLPOpen("en", SLP_FALSE, &hSLP))
      {
         return(hSLP);
      }
   }
   return(SERVICE_HANDLE_INVALID);
}

/******************************************************************************
 *
 *    Function: SERVICE_vCloseServiceHandle
 *
 *    Args:    hServiceHandle - previously opened service handle
 *
 *    Return:  SERVICE_HANDLE - SERVICE_HANDLE_INVALID on error
 *
 *    Purpose: Close a Service handle
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void SERVICE_vCloseServiceHandle(SERVICE_HANDLE hServiceHandle)
{
//   if (T_SLPD_bReady())
   {
      SLPClose(hServiceHandle);
   }
}

/******************************************************************************
 *
 *    Function: SERVICE_bRegister
 *
 *    Args:    pcService       - name of service
 *             pcInstance      - instance of service (can be NULL if N/A i.e. singleton)
 *             pServiceAddress - address of service
 *             pcAttributes    - Comma seperated service attributes
 *                               Example...
 *                               "(attr1=valA),(attr2=valB)"
 *                               (instance=<pcInstance>) and
 *                               (queue=<QueueName>) are reserved
 *
 *    Return:  BOOLEAN - TRUE if successful
 *
 *    Purpose: Register a service with SLP
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN SERVICE_bRegister(SERVICE_HANDLE hServiceHandle, const char *pcService, char *pcInstance,
                                 GMSG_ADDRESS *pServiceAddress, char *pcAttributes)
{
   SLPError       errReg;
   struct in_addr inAddr;
   char           pcAddr[16];
   char          *pcURL;
   char          *pcExtraAttributes;
   char           pcEmpty[]  = "";
   char           pcNullInstance[] = "NULL";
   char           pcFormat[] = "(service=%s),(instance=%s),(address=%s),(%s=%s),%s";
   char           pcQueueLabel[]  = "queue";
   char           pcPortLabel[]   = "port";
   char          *pcLabel;
   char           pcValue[16];
   U16            u16Port;

//   if (!T_SLPD_bReady())
//   {
//      return(FALSE);
//   }

   if (pcAttributes == NULL)
   {
      pcAttributes = pcEmpty;
   }

   if (pcInstance == NULL || *pcInstance == '\0')
   {
      pcInstance = pcNullInstance;
   }

   if (pServiceAddress->MsgAddressType == GMSG_ADDRESS_SOCKET)
   {
      inAddr.s_addr = pServiceAddress->uAddr.Socket.u32Address;
      u16Port       = (U16)pServiceAddress->uAddr.Socket.u32Port;
      pcLabel       = pcPortLabel;
      sprintf(pcValue, "%d", u16Port);
   }
   else if (pServiceAddress->MsgAddressType == GMSG_ADDRESS_QUEUE)
   {
      if (!(GSOCKET_bGetLocalAddress((U32*)&inAddr.s_addr, "eth0") || GSOCKET_bGetLocalAddress((U32*)&inAddr.s_addr, "eth0:avahi")))
      {
         return(FALSE);
      }
      u16Port = 0;
      pcLabel = pcQueueLabel;
      strcpy(pcValue, pServiceAddress->uAddr.Queue);
   }
   else
   {
      return(FALSE);
   }

   if (NULL == inet_ntop(AF_INET, &inAddr, pcAddr, sizeof(pcAddr)))
   {
      return(FALSE);
   }

   pcExtraAttributes = malloc(strlen(pcFormat) - 12 + strlen(pcService) + strlen(pcInstance) +
                                         strlen(pcAddr) + strlen(pcLabel) + strlen(pcValue) + strlen(pcAttributes) + 1);
   sprintf(pcExtraAttributes, pcFormat, pcService, pcInstance, pcAddr, pcLabel, pcValue, pcAttributes);

   if(!SERVICE_bMakeURL(&pcURL, pcService, pcInstance, pcAddr, u16Port))
   {
      free(pcExtraAttributes);
      return(FALSE);
   }

   if (SLP_OK != SLPReg(hServiceHandle, pcURL, SLP_LIFETIME_MAXIMUM, NULL, pcExtraAttributes, SLP_TRUE, SERVICE_vRegCallback, &errReg) ||
       SLP_OK != errReg)
   {
      free(pcURL);
      free(pcExtraAttributes);
      return(FALSE);
   }
   free(pcURL);
   free(pcExtraAttributes);
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: SERVICE_bUnregister
 *
 *    Args:    pcService       - name of service
 *             pcInstance      - instance of service
 *             pServiceAddress - address of service
 *
 *    Return:  BOOLEAN - TRUE on Success
 *
 *    Purpose: Unregister a service
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN SERVICE_bUnregister(SERVICE_HANDLE hServiceHandle, char *pcService, char *pcInstance,
                                   GMSG_ADDRESS *pServiceAddress)
{
   SLPError       errReg;
   char           *pcURL;
   struct in_addr inAddr;
   char           pcAddr[16];
   U16            u16Port;
   char           pcNullInstance[] = "NULL";

//   if (!T_SLPD_bReady())
//   {
//      return(FALSE);
//   }

   if (pcInstance == NULL || *pcInstance == '\0')
   {
      pcInstance = pcNullInstance;
   }

   if (pServiceAddress->MsgAddressType == GMSG_ADDRESS_SOCKET)
   {
      inAddr.s_addr = pServiceAddress->uAddr.Socket.u32Address;
      u16Port       = (U16)pServiceAddress->uAddr.Socket.u32Port;
   }
   else if (pServiceAddress->MsgAddressType == GMSG_ADDRESS_QUEUE)
   {
      if (!(GSOCKET_bGetLocalAddress((U32*)&inAddr.s_addr, "eth0") || GSOCKET_bGetLocalAddress((U32*)&inAddr.s_addr, "eth0:avahi")))
      {
         return(FALSE);
      }
      u16Port = 0;
   }
   else
   {
      return(FALSE);
   }

   if (NULL == inet_ntop(AF_INET, &inAddr, pcAddr, sizeof(pcAddr)))
   {
      return(FALSE);
   }

   if(!SERVICE_bMakeURL(&pcURL, pcService, pcInstance, pcAddr, u16Port))
   {
      return(FALSE);
   }

   if (SLP_OK != SLPDereg(hServiceHandle, pcURL, SERVICE_vRegCallback, &errReg) ||
       SLP_OK != errReg)
   {
      free(pcURL);
      return(FALSE);
   }
   free(pcURL);
   return(TRUE);
}

/******************************************************************************/
/**
 *
 *
 * @param pServiceTypeInfo
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN SERVICE_bGetServiceTypeInfo(SERVICE_HANDLE hServiceHandle, SERVICE_TYPE_INFO *pServiceTypeInfo)
{
   SERVICE_TYPE_CALLBACK_INFO    ServiceTypeCallbackInfo;
   int                           nAllocated = 0;

//   if (!T_SLPD_bReady())
//   {
//      return(FALSE);
//   }

   pServiceTypeInfo->nCount = 0;
   pServiceTypeInfo->pcServiceTypes = NULL;

   ServiceTypeCallbackInfo.err = SLP_OK;
   ServiceTypeCallbackInfo.pcServiceTypes = NULL;

   if (SLP_OK != SLPFindSrvTypes(hServiceHandle, NAMING_AUTHORITY, "", SERVICE_bTypeCallback, (void*)&ServiceTypeCallbackInfo))
   {
      if (ServiceTypeCallbackInfo.pcServiceTypes != NULL)
      {
         free(ServiceTypeCallbackInfo.pcServiceTypes);
      }
      return(FALSE);
   }
   if (ServiceTypeCallbackInfo.pcServiceTypes != NULL)
   {
      char *pPtr = NULL;
      char *pService;

      pService = strtok_r(ServiceTypeCallbackInfo.pcServiceTypes, ",", &pPtr);
      while (pService != NULL)
      {
         char *pcDotNamingAuthority;

         if (NULL != (pcDotNamingAuthority = strrchr(pService, '.')))
         {
            *pcDotNamingAuthority = '\0'; //end the string before naming authority
            if ((pServiceTypeInfo->nCount+1) > nAllocated)
            {
               nAllocated += SERVICE_INFO_ARRAY_GROWTH;
               pServiceTypeInfo->pcServiceTypes = realloc(pServiceTypeInfo->pcServiceTypes, sizeof(char*) * nAllocated);
            }
            pServiceTypeInfo->pcServiceTypes[pServiceTypeInfo->nCount] = strdup(pService);
            pServiceTypeInfo->nCount++;
         }
         pService = strtok_r(NULL, ",", &pPtr);
      }
      free(ServiceTypeCallbackInfo.pcServiceTypes);
   }
   return(TRUE);
}

/******************************************************************************/
/**
 *
 *
 * @param pServiceTypeInfo
 *
 * @return BOOLEAN
 */
/******************************************************************************/
GLOBAL BOOLEAN SERVICE_bFreeServiceTypeInfo(SERVICE_TYPE_INFO *pServiceTypeInfo)
{
   int i;
   if (pServiceTypeInfo->pcServiceTypes != NULL)
   {
      for (i = 0; i < pServiceTypeInfo->nCount; i++)
      {
         if (pServiceTypeInfo->pcServiceTypes[i] != NULL)
         {
            free(pServiceTypeInfo->pcServiceTypes[i]);
         }
      }
      free(pServiceTypeInfo->pcServiceTypes);
   }
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: SERVICE_bSearch
 *
 *    Args:    pSearchInfo - ptr to SERVICE_SEARCH_INFO structure
 *             pcService   - Service name to search for
 *             pcInstance  - Instance to search for (NULL = get all instances)
 *             u32IpAddress- IP Address to search at (applicable if eSearchScope == SERVICE_SEARCH_ADDRESS)
 *             pcAttributeFilter - LDAPv3 Search Filter (NULL/empty string if no filter)
 *                                 anded with a filter list of the address and/or instance (if available)
 *                                 http://www.openslp.org/doc/rfc/rfc2254.txt
 *             eSearchScope - SERVICE_SEARCH_LOCAL, SERVICE_SEARCH_ADDRESS, SERVICE_SEARCH_GLOBAL
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Search for services using SLP
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN SERVICE_bSearch(SERVICE_HANDLE hServiceHandle, SERVICE_SEARCH_INFO *pSearchInfo,
                               const char *pcService, char *pcInstance, U32 u32IpAddress,
                               char *pcAttributeFilter, SERVICE_SEARCH_SCOPE eSearchScope)
{
   SLPSrvURL                 *pParsedURL;
   SERVICE_CALLBACK_INFO      ServiceCallbackInfo;
   SERVICE_ATTR_CALLBACK_INFO ServiceAttrCallbackInfo;
   struct in_addr             inAddr;
   char                       pcAddr[16];
   char                      *pcTotalFilter;
   char                      *pcTemp;
   char                      *pcFullServiceName;
   char                       pcFormatService[]   = "%s.%s";
   char                       pcStartFilter[]     = "(&";
   char                       pcEndFilter[]       = ")";
   char                       pcFormatInstance[]  = "(instance=%s)";
   char                       pcFormatAddress[]   = "(address=%s)";
   int                        i;

//   if (!T_SLPD_bReady())
//   {
//      return(FALSE);
//   }

   if (pcService == NULL)
   {
      return(FALSE);
   }
   else
   {
      pcFullServiceName = malloc(strlen(pcFormatService) - 4 + strlen(pcService) + strlen(NAMING_AUTHORITY) + 1);
      sprintf(pcFullServiceName, pcFormatService, pcService, NAMING_AUTHORITY);
   }

   //decide to use unicast (to a specific address, or default multicast)
   switch(eSearchScope)
   {
   case SERVICE_SEARCH_LOCAL:
      if (!(GSOCKET_bGetLocalAddress(&u32IpAddress, "eth0") || GSOCKET_bGetLocalAddress(&u32IpAddress, "eth0:avahi")))
      {
         free(pcFullServiceName);
         return(FALSE);
      }
      //fall through on purpose here
   case SERVICE_SEARCH_ADDRESS:
      inAddr.s_addr = u32IpAddress;
      if (NULL == inet_ntop(AF_INET, &inAddr, pcAddr, sizeof(pcAddr)))
      {
         free(pcFullServiceName);
         return(FALSE);
      }
      if(SLP_OK != SLPAssociateIP(hServiceHandle, pcAddr))
      {
         free(pcFullServiceName);
         return(FALSE);
      }
      break;
   case SERVICE_SEARCH_GLOBAL:
      pcAddr[0] = '\0';
      break;
   default:
      //unknown search scope
      free(pcFullServiceName);
      return(FALSE);
   }

   //allocate memory for attribute filter
   pcTotalFilter = malloc(strlen(pcStartFilter) +
                                     ((pcInstance && *pcInstance) ? (strlen(pcFormatInstance) - 2 + strlen(pcInstance)) : 0) +
                                     (*pcAddr ? (strlen(pcFormatAddress) - 2 + strlen(pcAddr)) : 0) +
                                     ((pcAttributeFilter && *pcAttributeFilter) ? strlen(pcAttributeFilter) : 0) +
                                     strlen(pcEndFilter) + 1);

   //create total attribute filter (add address or instance if applicable)
   if ((pcInstance && *pcInstance) || *pcAddr || (pcAttributeFilter && *pcAttributeFilter))
   {
      pcTemp = pcTotalFilter;
      strcpy(pcTemp, pcStartFilter);
      pcTemp += strlen(pcTemp);
      if (pcInstance && *pcInstance)
      {
         sprintf(pcTemp, pcFormatInstance, pcInstance);
         pcTemp += strlen(pcTemp);
      }
      if (*pcAddr)
      {
         sprintf(pcTemp, pcFormatAddress, pcAddr);
         pcTemp += strlen(pcTemp);
      }
      if (pcAttributeFilter && *pcAttributeFilter)
      {
         strcpy(pcTemp, pcAttributeFilter);
         pcTemp += strlen(pcTemp);
      }
      strcpy(pcTemp, pcEndFilter);
   }
   else
   {
      pcTotalFilter[0] = '\0'; //null terminate empty string case
   }

   //prepare the callback cookie
   pSearchInfo->nCount             = 0;
   pSearchInfo->ServiceInfoArray   = NULL;
   ServiceCallbackInfo.pSearchInfo = pSearchInfo;
   ServiceCallbackInfo.err         = SLP_OK;
   ServiceCallbackInfo.nAllocated  = 0;

   //search for services matching pcService, filtered by pcTotalFilter
   if(SLP_OK != SLPFindSrvs(hServiceHandle, pcFullServiceName, NULL, pcTotalFilter, SERVICE_bSearchCallback, (void*)&ServiceCallbackInfo))
   {
      free(pcTotalFilter);
      free(pcFullServiceName);
      return(FALSE);
   }

   //check callback cookie for errors
   if (ServiceCallbackInfo.err != SLP_OK)
   {
      if (pSearchInfo->ServiceInfoArray != NULL)
      {
         free(pSearchInfo->ServiceInfoArray);
      }
      free(pcTotalFilter);
      free(pcFullServiceName);
      return(FALSE);
   }
   free(pcTotalFilter);
   free(pcFullServiceName);

   //process each service URL returned
   for (i = 0; i < pSearchInfo->nCount; i++)
   {
      char *pSavePtr;

      //parse the URL, break it into primary components
      if(SLP_OK != SLPParseSrvURL(pSearchInfo->ServiceInfoArray[i].pcURL, &pParsedURL))
      {
         SLPFree(pParsedURL);
         return(FALSE);
      }

      //get the service and instance
      (void)strtok_r(pParsedURL->s_pcSrvType, ":.", &pSavePtr);
      pSearchInfo->ServiceInfoArray[i].pcService  = strdup(strtok_r(NULL, ":.", &pSavePtr));

      if (strcasecmp(&pParsedURL->s_pcSrvPart[1], "NULL") != 0)
      {
         pSearchInfo->ServiceInfoArray[i].pcInstance = strdup(&pParsedURL->s_pcSrvPart[1]);
      }

      //is it a queue?
      if (pParsedURL->s_iPort == 0)
      {
         char *pQueueName;

         //the queue name is in the "queue" attribute
         ServiceAttrCallbackInfo.err = SLP_OK;
         ServiceAttrCallbackInfo.pcAttributes = NULL;
         if (SLP_OK != SLPFindAttrs(hServiceHandle, pSearchInfo->ServiceInfoArray[i].pcURL, NULL, "queue", SERVICE_bAttributeCallback,
                                   (void*)&ServiceAttrCallbackInfo))
         {
            SLPFree(pParsedURL);
            return(FALSE);
         }

         //parse the "queue" attribute
         if (SLP_OK != SLPParseAttrs(ServiceAttrCallbackInfo.pcAttributes, "queue", &pQueueName))
         {
            free(ServiceAttrCallbackInfo.pcAttributes);
            SLPFree(pParsedURL);
            return(FALSE);
         }

         pSearchInfo->ServiceInfoArray[i].MsgAddress.MsgAddressType = GMSG_ADDRESS_QUEUE;
         strcpy(pSearchInfo->ServiceInfoArray[i].MsgAddress.uAddr.Queue, pQueueName);
         free(ServiceAttrCallbackInfo.pcAttributes);
         SLPFree(pQueueName);
      }
      else //its a socket
      {
         struct in_addr addr;

         if (1 != inet_pton(AF_INET, pParsedURL->s_pcHost, (void*)&addr))
         {
            SLPFree(pParsedURL);
            return(FALSE);
         }

         pSearchInfo->ServiceInfoArray[i].MsgAddress.MsgAddressType = GMSG_ADDRESS_SOCKET;
         pSearchInfo->ServiceInfoArray[i].MsgAddress.uAddr.Socket.u32Address = (U32)addr.s_addr;
         pSearchInfo->ServiceInfoArray[i].MsgAddress.uAddr.Socket.u32Port    = (U32)pParsedURL->s_iPort;
      }

      SLPFree(pParsedURL);
   }

   return(TRUE);
}

/******************************************************************************
 *
 *    Function: SERVICE_bGetAttributes
 *
 *    Args:    pServiceEntry   - Service Entry to find attribute
 *             pAttributeIds   - Comma delimited list of attributes we are
 *                               interested in, NULL for all attributes
 *             ppAttributeList - ptr to location to store attribute list
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Get the attributes associated with a service
 *
 *    Notes:   Attribute List must be freeed with SERVICE_bFreeAttribute()
 *
 ******************************************************************************/
GLOBAL BOOLEAN SERVICE_bGetAttributes(SERVICE_HANDLE hServiceHandle, SERVICE_ENTRY *pServiceEntry,
                                      char *pAttributeIds, char *ppAttributeList)
{
//   if (!T_SLPD_bReady())
//   {
//      return(FALSE);
//   }

   SERVICE_ATTR_CALLBACK_INFO ServiceAttrCallbackInfo;
   int                        i;

   ServiceAttrCallbackInfo.err = SLP_OK;
   ServiceAttrCallbackInfo.pcAttributes = NULL;

   if (SLP_OK != SLPFindAttrs(hServiceHandle, pServiceEntry->pcURL, NULL, pAttributeIds, SERVICE_bAttributeCallback,
                             (void*)&ServiceAttrCallbackInfo) || SLP_OK != ServiceAttrCallbackInfo.err)
   {
      return(FALSE);
   }

   if ((ServiceAttrCallbackInfo.pcAttributes == NULL) || (strlen(ServiceAttrCallbackInfo.pcAttributes) == 0))
   {
      return(FALSE);
   }

   strcpy(ppAttributeList, ServiceAttrCallbackInfo.pcAttributes);
   free(ServiceAttrCallbackInfo.pcAttributes);
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: SERVICE_bParseAttribute
 *
 *    Args:    pAtributeList - list of service attributes
 *             pAttribute    - desired attribute name
 *             ppValue       - ptr location where value is set
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Parse the value of an attribute in an attribute list
 *
 *    Notes:   Value must be freed with SERVICE_bFreeAttribute()
 *
 ******************************************************************************/
GLOBAL BOOLEAN SERVICE_bParseAttribute(char *pAttributeList, char *pAttribute, char **ppValue)
{
   char *pTempVal;

   if (SLP_OK != SLPParseAttrs(pAttributeList, pAttribute, &pTempVal))
   {
      return(FALSE);
   }
   *ppValue = strdup(pTempVal);
   SLPFree(pTempVal);
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: SERVICE_bFreeAttribute
 *
 *    Args:    pServiceAttr - list of service attributes
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Free list of service attributes
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN SERVICE_bFreeAttribute(char *pAttributeListOrValue)
{
   free(pAttributeListOrValue);
   return(TRUE);
}


/******************************************************************************
 *
 *    Function: SERVICE_bFreeSearchInfo
 *
 *    Args:    pServiceSearchInfo - Service Search Information
 *
 *    Return:  BOOLEAN - TRUE on Success
 *
 *    Purpose: Free Service Search Information
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN SERVICE_bFreeSearchInfo(SERVICE_SEARCH_INFO *pServiceSearchInfo)
{
   int i;

   if (pServiceSearchInfo->ServiceInfoArray != NULL)
   {
      for (i = 0; i < pServiceSearchInfo->nCount; i++)
      {
         if (pServiceSearchInfo->ServiceInfoArray[i].pcURL != NULL)
         {
            free(pServiceSearchInfo->ServiceInfoArray[i].pcURL);
         }
         if (pServiceSearchInfo->ServiceInfoArray[i].pcService != NULL)
         {
            free(pServiceSearchInfo->ServiceInfoArray[i].pcService);
         }
         if (pServiceSearchInfo->ServiceInfoArray[i].pcInstance != NULL)
         {
            free(pServiceSearchInfo->ServiceInfoArray[i].pcInstance);
         }
      }
      free(pServiceSearchInfo->ServiceInfoArray);
   }
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: SERVICE_bMakeURL
 *
 *    Args:    ppcURL      - URL to allocate
 *             pcService   - service name
 *             pcPath      - service path
 *             pcAddr      - service ip address dotted decimal string
 *             u16Port     - serivce port
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Create a URL from base service information
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE BOOLEAN SERVICE_bMakeURL(char **ppcURL, const char *pcService, const char *pcPath, const char *pcAddr, U16 u16Port)
{
   int                  nLength;
   char                 pcFormat[] = "service:%s.%s://%s:%d/%s";

   nLength = strlen(pcFormat) - 10 /* format conversion */ + strlen(pcService) + strlen(NAMING_AUTHORITY)
           + strlen(pcAddr) + 5 /* port no. */ + strlen(pcPath) + 1 /* null */;
   *ppcURL = (char*)malloc(nLength);
   sprintf(*ppcURL, pcFormat, pcService, NAMING_AUTHORITY, pcAddr, u16Port, pcPath);
   return(TRUE);
}

/******************************************************************************
 *
 *    Function: SERVICE_vRegCallback
 *
 *    Args:    hSLP - SLP handle
 *             errCode - SLP Error Code
 *             pvCookie - parameter from Registration/Deregistration call
 *
 *    Return:  -none-
 *
 *    Purpose: Callback function for SLP Registration/Deregistration
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE void SERVICE_vRegCallback(SLPHandle hSLP, SLPError errCode, void *pvCookie)
{
   *(SLPError*)pvCookie = errCode;
}

/******************************************************************************/
/**
 *
 *
 * @param hSLP
 * @param pSrvTypes
 * @param errCode
 * @param pvCookie
 *
 * @return SLPBoolean
 */
/******************************************************************************/
PRIVATE SLPBoolean SERVICE_bTypeCallback(SLPHandle hSLP, const char* pSrvTypes, SLPError errCode, void* pvCookie)
{
   SERVICE_TYPE_CALLBACK_INFO *pServiceTypeCallbackInfo = (SERVICE_TYPE_CALLBACK_INFO*)pvCookie;

   switch(errCode)
   {
   case SLP_OK:
      pServiceTypeCallbackInfo->err = SLP_OK;
      if (pServiceTypeCallbackInfo->pcServiceTypes == NULL)
      {
         pServiceTypeCallbackInfo->pcServiceTypes = strdup(pSrvTypes);
      }
      return(SLP_TRUE);
   case SLP_LAST_CALL:
      pServiceTypeCallbackInfo->err = SLP_OK;
      return(SLP_FALSE);
   default:
      pServiceTypeCallbackInfo->err = errCode;
      return(SLP_FALSE);
   }
}

/******************************************************************************
 *
 *    Function: SERVICE_vSearchCallback
 *
 *    Args:    hSLP        - SLP Handle
 *             pcSrvURL    - Service URL found
 *             sLifetime   - lifetime of service
 *             errCode     - error code
 *             pvCookie    - cookie (SERVICE_CALLBACK INFO *)
 *
 *    Return:  SLPBoolean - TRUE on want more data
 *
 *    Purpose: Callback function for SLPFindSrvs
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE SLPBoolean SERVICE_bSearchCallback(SLPHandle hSLP, const char* pcSrvURL, unsigned short sLifetime,
                                           SLPError errCode, void *pvCookie)
{
   SERVICE_CALLBACK_INFO *pServiceCallbackInfo = (SERVICE_CALLBACK_INFO *)pvCookie;
   SERVICE_SEARCH_INFO   *pSearchInfo          = pServiceCallbackInfo->pSearchInfo;

   switch(errCode)
   {
   case SLP_OK:
      if ((pSearchInfo->nCount+1) > pServiceCallbackInfo->nAllocated)
      {
         pServiceCallbackInfo->nAllocated += SERVICE_INFO_ARRAY_GROWTH;
         pSearchInfo->ServiceInfoArray = realloc(pSearchInfo->ServiceInfoArray,
                                                 sizeof(SERVICE_ENTRY)*pServiceCallbackInfo->nAllocated);
      }
      pSearchInfo->ServiceInfoArray[pSearchInfo->nCount].pcURL = strdup(pcSrvURL);
      pSearchInfo->ServiceInfoArray[pSearchInfo->nCount].pcService = NULL;
      pSearchInfo->ServiceInfoArray[pSearchInfo->nCount].pcInstance = NULL;
      pSearchInfo->nCount++;
      pServiceCallbackInfo->err = SLP_OK;
      return(SLP_TRUE);
   case SLP_LAST_CALL:
      pServiceCallbackInfo->err = SLP_OK;
      return(SLP_FALSE);
   default:
      pServiceCallbackInfo->err = errCode;
      return(SLP_FALSE);
   }
}

/******************************************************************************
 *
 *    Function: SERVICE_bAttributeCallback
 *
 *    Args:    hSLP        - SLP Handle
 *             pcAttrList  - List of attributes found
 *             errCode     - error code
 *             pvCookie    - cookie (SERVICE_ATTR_CALLBACK_INFO *)
 *
 *    Return:  SLPBoolean - TRUE on want more data
 *
 *    Purpose: Callback function for SLPFindAttrs
 *
 *    Notes:
 *
 ******************************************************************************/
PRIVATE SLPBoolean SERVICE_bAttributeCallback(SLPHandle hSLP, const char* pcAttrList,
                                              SLPError errCode, void *pvCookie)
{
   SERVICE_ATTR_CALLBACK_INFO *pServiceAttrCallbackInfo = (SERVICE_ATTR_CALLBACK_INFO *)pvCookie;

   switch(errCode)
   {
      case SLP_OK:
         if (pServiceAttrCallbackInfo->pcAttributes == NULL)
         {
            pServiceAttrCallbackInfo->pcAttributes = strdup(pcAttrList);
         }

         pServiceAttrCallbackInfo->err = SLP_OK;
         break;
      case SLP_LAST_CALL:
         pServiceAttrCallbackInfo->err = SLP_OK;
         break;
      default:
         pServiceAttrCallbackInfo->err = errCode;
         break;
   }

   //Since discovery of attributes by service-type is not supported,
   //there is really no reason to return anything but SLP_FALSE from
   //the SLPAttrCallback().
   return(SLP_FALSE);
}

