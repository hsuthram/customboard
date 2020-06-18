/*******************************************************************************
*           Copyright (c) 2000 - 2008 Techsonic Industries, Inc.               *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: gsocket.h - Geonav socket handling code

      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      + This software is the proprietary property of:                   +
      +                                                                 +
      +  Techsonic Industries, Inc.                                     +
      +                                                                 +
      +  1220 Old Alpharetta Rd Ste 340   1 Humminbird Lane             +
      +  Alpharetta, GA  30005            Eufaula, AL  36027            +
      +                                                                 +
      + Use, duplication, or disclosure of this material, in any form,  +
      + is forbidden without permission from Techsonic Industries, Inc. +
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

*******************************************************************************/

#ifndef GSOCKET_H
   #define GSOCKET_H
#ifdef __cplusplus
   extern "C" {
#endif
/********************           COMPILE FLAGS                ******************/
/********************           INCLUDE FILES                ******************/
#include "gmsg_addr.h"
#ifdef linux
   #include <netinet/in.h>
#else
   #ifndef USE_LWIP_STACK
      #define USE_LWIP_STACK 0
   #endif
   #if USE_LWIP_STACK
      #include "lwip/sockets.h"
   #else
      #include "fns_in.h"
   #endif
#endif
/********************               ENUMS                    ******************/
/********************              DEFINES                   ******************/
#ifndef HOST_NAME_MAX
   #define HOST_NAME_MAX 255
#endif


#if USE_LWIP_STACK  // send()'s larger than this will fail with LWIP
#define GSOCKET_MAX_PACKET   TCP_SND_BUF
#else
#define GSOCKET_MAX_PACKET   32768
#endif
/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef struct
{
   int               nSocket;
   GSOCKET_ADDRESS   Address;
   int               nBacklog;
   BOOLEAN           bListen;
   BOOLEAN           bNonBlocking;
} GSOCKET;

/********************        FUNCTION PROTOTYPES             ******************/

extern BOOLEAN GSOCKET_bCreate(GSOCKET *pSocket, U32 u32Address, U16 u16Port,
                               BOOLEAN bListen, int nBacklog, BOOLEAN bNonBlocking);
extern BOOLEAN GSOCKET_bCreateUdp(GSOCKET *pSocket, U32 u32Address, U16 u16Port,
                                  BOOLEAN bListen, int nBacklog, BOOLEAN bNonBlocking);
extern BOOLEAN GSOCKET_bSetNonBlocking(GSOCKET *pSocket, BOOLEAN bNonBlocking);
extern BOOLEAN GSOCKET_bAccept(GSOCKET *pListenSocket, GSOCKET *pConnectSocket, BOOLEAN bNonBlocking);
extern BOOLEAN GSOCKET_bConnect(GSOCKET *pSocket);
extern int     GSOCKET_nSend(GSOCKET *pSocket, char *pData, int nDataLength);
extern int     GSOCKET_nReceive(GSOCKET *pSocket, char *pData, int nMaxData);
extern int     GSOCKET_nReceiveFrom(GSOCKET *pSocket, char *pData, int nMaxData);
extern BOOLEAN GSOCKET_bClose(GSOCKET *pSocket);
extern int     GSOCKET_nDataAvailable(GSOCKET *pSocket, char *pData, int nMaxData);
extern int     GSOCKET_nDataAvailableUdp(GSOCKET *pSocket, char *pData, int nMaxData);

extern BOOLEAN GSOCKET_bGetLocalAddress(U32 *pu32Address, char *pDevice);
extern BOOLEAN GSOCKET_bGetMacAddress(unsigned char *pcMacAddress, char *pDevice);

extern BOOLEAN GSOCKET_bOpenSocket( int *pSocket, struct sockaddr_in *pSockAddr,
                   char *hostname, U32 u32IPaddress, U16 u16PortNumber, BOOLEAN bSetLinger, BOOLEAN bUdp );
extern BOOLEAN GSOCKET_bSetBroadcastOptions(GSOCKET *pSocket);
extern BOOLEAN GSOCKET_bSetMulticastOptions(GSOCKET *pSocket, U32 u32MulticastAddress, BOOLEAN bAdd);

#ifdef __cplusplus
   }
#endif
#endif // #ifndef GSOCKET_H
