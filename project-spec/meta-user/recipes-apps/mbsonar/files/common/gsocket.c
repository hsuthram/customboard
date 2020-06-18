/*******************************************************************************
*           Copyright (c) 2000 - 2008 Techsonic Industries, Inc.               *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: gsocket.c - Geonav socket handling code

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

/********************           COMPILE FLAGS                ******************/
//#include "compile_flags.h"

/********************           INCLUDE FILES                ******************/
#ifdef __RTXC__
//   #include "matrixOS.h"
//   #include "nomalloc.h"
//   #include "model.h"
   #include "gmsg.h"
#else
   #include "u_socket.h"
   #include "event.h"
#endif
#include "gsocket.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#ifndef linux
   #ifndef USE_LWIP_STACK
      #define USE_LWIP_STACK 0
   #endif
   #if USE_LWIP_STACK
      #include "lwip/sockets.h"
   #else
      #include "fns_in.h"
      #include "fns_netapi.h"
      #include "fns_bsdsockapi.h"
   #endif
   #include "h_ethernet.h"
#else
   #include <unistd.h>
   #include <sys/socket.h> // socket includes
   #include <netinet/in.h>
   #include <netdb.h>
   #include <fcntl.h>
   #include <sys/ioctl.h>
   #include <net/if.h>
#endif
//#if MODEL_HAS_NETWORKING
/********************               ENUMS                    ******************/

enum {
   QUERY = 1,  // Query data sources, then register, then done
   REGISTER,
   DONE,
   ERROR
};

/********************              DEFINES                   ******************/

#define DEBUG_GSOCKET   (0)
#define QR_SOCKET_BUFSIZE     (256)
#ifndef linux
   #define close           closesocket
#endif

#ifndef MSG_DONTWAIT
   #define MSG_DONTWAIT MSG_NONBLOCKING
#endif

/********************               MACROS                   ******************/
/********************         TYPE DEFINITIONS               ******************/
#ifndef linux
#ifndef socklen_t
   typedef int             socklen_t;
#endif
   typedef unsigned long   in_addr_t;
   typedef unsigned short  in_port_t;
#endif
/********************        FUNCTION PROTOTYPES             ******************/

#ifndef __RTXC__
PRIVATE BOOLEAN GSOCKET_bParseSonarResp( SOCKET_DATA_SOURCES *pQueryResp, U8 u8BeamType,
                   SOCKET_BEAM *pFoundBeam );
#if DEBUG_GSOCKET
PRIVATE void    GSOCKET_vPrintSonarData( SOCKET_DATA_SOURCES *pQueryResp );
#endif
#endif
/********************          LOCAL VARIABLES               ******************/
/********************             FUNCTIONS                  ******************/

/******************************************************************************/
/**
 * Create a socket
 *
 * @param pSocket      Socket Object
 * @param u32Address   Address to create socket on
 * @param u16Port      Port to open at/listen on
 * @param bListen      is it a listening socket
 * @param nBacklog     socket listen queue length
 * @param bNonBlocking setup socket as non-blocking
 *
 * @return BOOLEAN - TRUE on success
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bCreate(GSOCKET *pSocket, U32 u32Address, U16 u16Port,
                               BOOLEAN bListen, int nBacklog, BOOLEAN bNonBlocking)
{
   struct sockaddr_in addr;

   pSocket->Address.u32Address   = u32Address;
   pSocket->Address.u32Port      = (U32)u16Port;
   pSocket->nBacklog             = nBacklog;
   pSocket->bListen              = bListen;
   pSocket->bNonBlocking         = bNonBlocking;

   if (!GSOCKET_bOpenSocket(&pSocket->nSocket, &addr, NULL, u32Address, u16Port, FALSE, FALSE))
   {
      printf("GSOCKET_bCreate: Error creating socket");
      return(FALSE);
   }

   if (bListen)
   {
      if (bind(pSocket->nSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
      {
         printf("GSOCKET_bCreate: Error binding socket");
         close( pSocket->nSocket );
         return(FALSE);
      }
      listen(pSocket->nSocket, nBacklog);
   }

   GSOCKET_bSetNonBlocking(pSocket, bNonBlocking);

   return(TRUE);
}

/******************************************************************************
 *
 *    Function: GSOCKET_bCreateUdp
 *
 *    Args:    pSocket        - Socket Object
 *             u16Port        - Port to open at/listen on
 *             bListen        - is it a listening socket
 *             nBacklog       - socket listen queue length
 *             bNonBlocking   - setup socket as non-blocking
 *
 *    Return:  BOOLEAN - TRUE on success
 *
 *    Purpose: Create a UDP socket
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bCreateUdp(GSOCKET *pSocket, U32 u32Address, U16 u16Port,
                               BOOLEAN bListen, int nBacklog, BOOLEAN bNonBlocking)
{
   struct sockaddr_in addr;

   pSocket->Address.u32Address   = u32Address;
   pSocket->Address.u32Port      = (U32)u16Port;
   pSocket->nBacklog             = nBacklog;
   pSocket->bListen              = bListen;
   pSocket->bNonBlocking         = bNonBlocking;

   if (!GSOCKET_bOpenSocket(&pSocket->nSocket, &addr, NULL, u32Address, u16Port, FALSE, TRUE))
   {
      printf("GSOCKET_bCreateUdp: Error creating socket");
      return(FALSE);
   }

   if (bListen)
   {
      if (bind(pSocket->nSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
      {
         printf("GSOCKET_bCreateUdp: Error binding socket");
          GSOCKET_bClose(pSocket);
         return(FALSE);
      }
      //listen(pSocket->nSocket, nBacklog);  mar 9 11
   }

   GSOCKET_bSetNonBlocking(pSocket, bNonBlocking);

   return(TRUE);
}

/******************************************************************************/
/**
 * Set a socket as non-blocking
 *
 * @param pSocket      ptr to GSOCKET
 * @param bNonBlocking set to non-blocking?
 *
 * @return BOOLEAN - TRUE on success
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bSetNonBlocking(GSOCKET *pSocket, BOOLEAN bNonBlocking)
{
#ifdef linux
   if (-1 == fcntl(pSocket->nSocket, F_SETFL, bNonBlocking ? O_NONBLOCK : 0))
   {
      printf("GSOCKET_bSetNonBlocking: error setting O_NONBLOCK");
      return(FALSE);
   }
   return(TRUE);
#elif USE_LWIP_STACK
   int fdflags;

   fdflags = 1;
   ioctlsocket(pSocket->nSocket, FIONBIO, &fdflags);

   return TRUE;
#else
   int nErr;

   if (bNonBlocking)
   {
      if (FNS_ENOERR != fnsNonBlocking(pSocket->nSocket, &nErr))
      {
         printf("GSOCKET_bSetNonBlocking: error setting O_NONBLOCK");
         return(FALSE);
      }
   }
   else
   {
      if (FNS_ENOERR != fnsBlocking(pSocket->nSocket, &nErr))
      {
         printf("GSOCKET_bSetNonBlocking: error clearing O_NONBLOCK");
         return(FALSE);
      }
   }
   return(TRUE);
#endif
}

/******************************************************************************/
/**
 * Set a socket as Broadcast
 *
 * @param pSocket  ptr to GSOCKET
 *
 * @return BOOLEAN - TRUE on success
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bSetBroadcastOptions(GSOCKET *pSocket)
{
   int optval = 1;
   
   setsockopt(pSocket->nSocket, SOL_SOCKET, SO_BROADCAST, (char *)&optval, sizeof(optval));
   return(TRUE);
}

/******************************************************************************/
/**
 * Set a socket as Multicast
 *
 * @param pSocket  ptr to GSOCKET
 *
 * @return BOOLEAN - TRUE on success
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bSetMulticastOptions(GSOCKET *pSocket, U32 u32McAddress, BOOLEAN bAdd)
{
   int optval = 1;
   struct ip_mreq     mreq;
   char sAddressMcast[16];
   U32 u32Address = 0;

   if(!(GSOCKET_bGetLocalAddress(&u32Address, "eth0") || GSOCKET_bGetLocalAddress(&u32Address, "eth0:avahi")))
   {
      printf("GSOCKET_bSetMulticastOptions: error getting locaL address");
      return(FALSE);
   }

   setsockopt(pSocket->nSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

   sprintf(sAddressMcast, "%d.%d.%d.%d", (u32McAddress>>24)&0x000000ff,
                                         (u32McAddress>>16)&0x000000ff,
                                         (u32McAddress>>8)&0x000000ff,
                                          u32McAddress&0x000000ff );
   printf("ngd [%d] %s Multicast address=%s\n", __LINE__, __FUNCTION__, sAddressMcast);
   inet_pton(AF_INET, sAddressMcast, &mreq.imr_multiaddr);
   mreq.imr_interface.s_addr = u32Address;

   if (bAdd)
   {
      return (setsockopt (pSocket->nSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq, sizeof(mreq)));
   }
   else
   {
      return (setsockopt (pSocket->nSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &mreq, sizeof(mreq)));
   }
}

#ifndef __RTXC__
/******************************************************************************/
/**
 * Create an event from a socket
 *
 * @param pSocketEvent     socket event object
 * @param pSocket          socket object to watch event
 * @param pfEventHandler   socket event handler
 * @param pEventParam      socket event parameter
 * @param eEventMode       socket event mode (read, write, read/write)
 * @param u32EventPriority relative event priority
 *
 * @return BOOLEAN - TRUE on success
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bCreateEvent(GSOCKET_EVENT *pSocketEvent, GSOCKET *pSocket,
                                    EVENT_HANDLER_FP pfEventHandler, void *pEventParam,
                                    EVENT_MODE eEventMode, U32 u32EventPriority)
{
   pSocketEvent->EventHeader.fd             = pSocket->nSocket;
   pSocketEvent->EventHeader.eEventMode     = eEventMode;
   pSocketEvent->EventHeader.pfEventHandler = pfEventHandler;
   pSocketEvent->EventHeader.pEventParam    = pEventParam;
   pSocketEvent->EventHeader.u32Priority    = u32EventPriority;
   pSocketEvent->pSocket                    = pSocket;
   return(TRUE);
}

/******************************************************************************/
/**
 * Add a socket event to an event list
 *
 * @param pEventList   ptr to Event List
 * @param pSocketEvent ptr to Socket event to add to event list
 *
 * @return BOOLEAN - TRUE on success
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bAddEvent(EVENT_LIST *pEventList, GSOCKET_EVENT *pSocketEvent)
{
   return(EVENT_bAddEventContext(pEventList, (EVENT_CONTEXT*)pSocketEvent));
}
#endif //#ifndef __RTXC__

/******************************************************************************/
/**
 * accept a listening socket connection
 *
 * @param pListenSocket  ptr to Listening Socket
 * @param pConnectSocket ptr to Conntect Socket
 *
 * @return BOOLEAN - TRUE on success
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bAccept(GSOCKET *pListenSocket, GSOCKET *pConnectSocket, BOOLEAN bNonBlocking)
{
   struct sockaddr_in addr;
   socklen_t size = sizeof(struct sockaddr_in);
   int optval = 1;

   if (!pListenSocket->bListen)
   {
      printf("GSOCKET_bAccept: accept on non-listening socket");
      return(FALSE);
   }

   if (-1 == (pConnectSocket->nSocket = accept(pListenSocket->nSocket, (struct sockaddr*)&addr, &size)))
   {
      printf("GSOCKET_bAccept: socket accept failure\n");
      return(FALSE);
   }

   GSOCKET_bSetNonBlocking(pConnectSocket, bNonBlocking);

   // set keep alive option
   setsockopt( pConnectSocket->nSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval));

   pConnectSocket->Address.u32Address  = (U32)addr.sin_addr.s_addr;
   pConnectSocket->Address.u32Port     = (U32)ntohs(addr.sin_port);
   pConnectSocket->bListen             = FALSE;
   pConnectSocket->bNonBlocking        = bNonBlocking;
   pConnectSocket->nBacklog            = 0;

   return(TRUE);
}

/******************************************************************************/
/**
 * Connect a socket (non-listening socket)
 *
 * @param pSocket ptr to GSOCKET
 *
 * @return BOOLEAN - TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bConnect(GSOCKET *pSocket)
{
   struct sockaddr_in addr;
   BOOLEAN bSuccess = TRUE;
   int ret;

   if (pSocket->bListen)
   {
      printf("GSOCKET_bConnect: connect on listening socket");
      return(FALSE);
   }

   memset(&addr, 0, sizeof(addr));
   addr.sin_addr.s_addr = (in_addr_t)pSocket->Address.u32Address;
   addr.sin_port        = (in_port_t)htons((U16)pSocket->Address.u32Port);
   addr.sin_family      = AF_INET;
   if (0 != (ret=connect(pSocket->nSocket,(struct sockaddr *)&addr, sizeof(addr))) )
   {
      if (errno == EINPROGRESS)
      {
         fd_set writeFds;
         fd_set excFds;
         struct timeval timeout = {0, 250000};

         FD_ZERO(&writeFds);
         FD_SET(pSocket->nSocket, &writeFds);

         FD_ZERO(&excFds);
         FD_SET(pSocket->nSocket, &excFds);

         if (0 >= (ret=select(pSocket->nSocket + 1, NULL, &writeFds, &excFds, &timeout)) )
         {
            printf("GSOCKET_bConnect: select returns ret=%d (errno=%d)\r\n", ret, errno);
            close( pSocket->nSocket );
            bSuccess = FALSE;
         }

         if ( bSuccess && FD_ISSET( pSocket->nSocket, &excFds ) )
         {
            close( pSocket->nSocket );
            bSuccess = FALSE;
         }
      }
      else
      {
         bSuccess = FALSE;
      }
   }
   if (!bSuccess)
   {
      printf("GSOCKET_bConnect: socket connect failure");
   }
   return(bSuccess);
}

/******************************************************************************/
/**
 * Send data to a socket
 *
 * @param pSocket     ptr to GSOCKET
 * @param pData       data buffer to send
 * @param nDataLength length of data
 *
 * @return BOOLEAN - TRUE if successful
 */
/******************************************************************************/
GLOBAL int GSOCKET_nSend(GSOCKET *pSocket, char *pData, int nDataLength)
{
   int nSent;

   if (-1 == (nSent = send(pSocket->nSocket, pData, nDataLength, 0)))
   {
      int error = errno;

      if (
#ifdef EAGAIN
            error == EAGAIN ||
#endif
            error == EWOULDBLOCK)
      {
         nSent = 0;
      }
      else
      {
         printf("GSOCKET_bSend: socket send failure");
      }
   }

   return(nSent);
}

/******************************************************************************/
/**
 * Receive Data from a socket
 *
 * @param pSocket  ptr to GSOCKET
 * @param pData    data buffer
 * @param nMaxData Length of data buffer
 *
 * @return int - no. of bytes received, -1 on error
 */
/******************************************************************************/
GLOBAL int GSOCKET_nReceive(GSOCKET *pSocket, char *pData, int nMaxData)
{
   int nReceived;

   if (-1 == (nReceived = recv(pSocket->nSocket, pData, nMaxData, 0)))
   {
      int error = errno;
      if (
#ifdef EAGAIN
            error == EAGAIN ||
#endif
            error == EWOULDBLOCK)
      {
         nReceived = 0;
      }
      else
      {
         printf("GSOCKET_nReceive: socket receive failure");
      }
   }
   return(nReceived);
}

/******************************************************************************
 *
 *    Function: GSOCKET_nReceiveFrom
 *
 *    Args:    pSocket  - ptr to GSOCKET_DGRAM
 *             pData    - data buffer
 *             nMaxData - Length of data buffer
 *
 *    Return:  int - no. of bytes received, -1 on error
 *
 *    Purpose: Receive Data from a socket
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL int GSOCKET_nReceiveFrom(GSOCKET *pSocket, char *pData, int nMaxData)
{
   int nReceived;

   if (-1 == (nReceived = recvfrom(pSocket->nSocket, pData, nMaxData, 0, NULL, NULL)))
   {
      printf("GSOCKET_nReceiveFrom: socket receive failure");
   }
   return(nReceived);
}

/******************************************************************************/
/**
 * Check if data available on the socket
 * 
 * @param pSocket 
 * 
 * @return BOOELAN 
 */
/******************************************************************************/
GLOBAL int GSOCKET_nDataAvailable(GSOCKET *pSocket, char *pData, int nMaxData)
{
   return(recv(pSocket->nSocket, pData, nMaxData, MSG_PEEK | MSG_DONTWAIT));
}

/******************************************************************************/
/**
 * Check if data available on the socket
 * 
 * @param pSocket 
 * 
 * @return BOOELAN 
 */
/******************************************************************************/
GLOBAL int GSOCKET_nDataAvailableUdp(GSOCKET *pSocket, char *pData, int nMaxData)
{
   return(recvfrom(pSocket->nSocket, pData, nMaxData, MSG_PEEK | MSG_DONTWAIT, NULL, NULL));
}

/******************************************************************************/
/**
 * Close a socket
 *
 * @param pSocket ptr to GSOCKET
 *
 * @return BOOLEAN - TRUE if successful
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bClose(GSOCKET *pSocket)
{
   if (-1 == close(pSocket->nSocket))
   {
      printf("GSOCKET_bClose: error closing socket");
      return(FALSE);
   }
   return(TRUE);
}

#ifndef __RTXC__
/******************************************************************************/
/**
 * Creates receiving socket and reads data 
 *
 * notes: Called when app gets alert that something is connecting to
 *              listening socket
 *
 * @param pListenSock pointer to listening socket
 * @param pAcceptSock pointer to returned connected socket
 * @param dataSize    size of data buffer
 * @param data        buffer to hold socket data (may be NULL)
 * 
 * @return S32 - count of bytes returned from socket, or -1 on FAIL
 */
/******************************************************************************/
GLOBAL S32 GSOCKET_s32ReceiveSocketData( int *pListenSock, int *pAcceptSock,
   int dataSize, char *data )
{
   int    size;
   char   buf[XWIN_SOCKET_BUFSIZE];
   struct sockaddr_in addr;
   U32    addr_size = sizeof(addr);

   if (( *pAcceptSock = accept( *pListenSock, (struct sockaddr *)&addr, &addr_size)) < 0 )
   {
      perror("accept()");
      return( -1 );
   }

#if DEBUG_GSOCKET
   printf ("Accepted connection from %s on port %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
#endif

   if ((size = recv( *pAcceptSock, buf, XWIN_SOCKET_BUFSIZE, 0 )) >= 0 )
   {
#if DEBUG_GSOCKET
      printf( "read %d bytes from socket\n", size );
#endif
      if ( data != NULL )
      {
         memset( (void *)data, 0, dataSize );
         memcpy( (void *)data, (void *)buf, MIN(dataSize,size));
      }
      return( size );
   }
   else
   {
      perror( "recv()" );
      return( -1 );
   }
}
#endif

/******************************************************************************/
/**
 * Creates socket to hostname (default) or IP address
 *
 * Note: IP address must already be in network byte order (if specified)
 *
 * @param pSocket       ptr to returned socket
 * @param pSockAddr     ptr returned struct sockaddr_in
 * @param hostname      host to create socket to
 * @param u32IPaddress  ip address to create socket to
 * @param u16PortNumber port number to create socket
 * @param bSetLinger    use linger flag and discard data on close?
 * @param bUdp          UDP socket
 *
 * @return BOOLEAN - TRUE if success, FALSE if fail
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bOpenSocket( int *pSocket, struct sockaddr_in *pSockAddr,
   char *hostname, U32 u32IPaddress, U16 u16PortNumber, BOOLEAN bSetLinger, BOOLEAN bUdp )
{
   int             optval = 1;
   struct hostent *hostInfo = NULL;
   struct linger   linger;

   if ( bSetLinger )
   {
      linger.l_onoff  = 1; // enable linger flag for consideration
      linger.l_linger = 0; // set socket to discard data on close
   }

   if ( hostname != NULL )
   {
      if ((hostInfo = gethostbyname( hostname )) == NULL )
      {
         perror("gethostbyname()");
         close( *pSocket );
         return( FALSE );
      }
   }

   if (bUdp)
   {
      if ((*pSocket = socket( PF_INET, SOCK_CLOEXEC|SOCK_DGRAM, 0)) < 0)
      {
         return( FALSE );
      }
   }
   else
   {
      if ((*pSocket = socket( PF_INET, SOCK_CLOEXEC|SOCK_STREAM, 0)) < 0)
      {
         return( FALSE );
      }     
   }

   // set socket options
   if (!bUdp)
      setsockopt( *pSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

   // set keep alive option
   setsockopt( *pSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval));

   if ( bSetLinger )
   {
      setsockopt( *pSocket, SOL_SOCKET, SO_LINGER,    (char *)&linger, sizeof(linger));
   }

   if ( pSockAddr != NULL )
   {
      memset((void *)pSockAddr, 0, sizeof(struct sockaddr_in));

      if ( hostInfo != NULL )
      {
         pSockAddr->sin_family = hostInfo->h_addrtype;
         memcpy((char *)&pSockAddr->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
      }
      else
      {
         pSockAddr->sin_family      = AF_INET;
         pSockAddr->sin_addr.s_addr = u32IPaddress;  // arg in network byte order
      }
      pSockAddr->sin_port = htons(u16PortNumber);
   }
   return( TRUE );
}

/******************************************************************************/
/**
 * Get the Local IP address
 *
 * @param pu32Address pu32Address - ptr to address
 *
 * @return BOOLEAN - TRUE if success, FALSE if fail
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bGetLocalAddress( U32 *pu32Address, char *pDevice  )
{
#ifdef linux
   int fd;
   struct ifreq ifr;

   if (0 > (fd = socket(AF_INET, SOCK_CLOEXEC|SOCK_DGRAM, 0)))
   {
      return( FALSE );
   }
   ifr.ifr_addr.sa_family = AF_INET;
   strncpy(ifr.ifr_name, pDevice, IFNAMSIZ-1);
   if (0 > (ioctl(fd, SIOCGIFADDR, &ifr)))
   {
      close(fd);
      return( FALSE );
   }
   *pu32Address = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
   close(fd);

   return( TRUE );

#elif USE_LWIP_STACK
   extern void DD_ETHERNET_vGetIpAddress(U32 *pu32Address);

   DD_ETHERNET_vGetIpAddress(pu32Address);

   return TRUE;

#else // RTXC FUSION

   int nListLen = 1;
   fnsIfAddress_t LocalAddress;
   struct in_sockaddr *pIfAddr;

   if (FNS_ENOERR != fnsIfGetIpAddressList(pDevice, &LocalAddress, &nListLen) || !nListLen )
   {
      return(FALSE);
   }
   pIfAddr = (struct in_sockaddr*)LocalAddress.ifAddr;
   *pu32Address = (U32)pIfAddr->sin_addr.s_addr;
   fnsIpFreeAddressList(&LocalAddress, nListLen);
   return(TRUE);

#endif //#ifdef linux
}

/******************************************************************************/
/**
 * Get Mac address for our default ethernet 
 *  
 * @param pcMacAddress char buffer of at least 6 bytes 
 * @param pDevice      device name such as "eth0" 
 * 
 * @return BOOLEAN - TRUE if success, FALSE if fail
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bGetMacAddress( unsigned char *pcMacAddress, char *pDevice )
{
#ifdef linux
  int fd;
  struct ifreq ifr;

  if (0 > (fd = socket(AF_INET, SOCK_CLOEXEC|SOCK_DGRAM, 0)))
  {
     return( FALSE );
  }
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, pDevice, IFNAMSIZ-1);
  if (0 > (ioctl(fd, SIOCGIFHWADDR, &ifr)))
  {
     close(fd);
     return( FALSE );
  }
  memcpy(pcMacAddress, ifr.ifr_hwaddr.sa_data, 6);
  close(fd);

  return( TRUE );
#else
   H_ETHERNET_eGetMacAddress((U8*)pcMacAddress);
   return(TRUE);
#endif //#ifdef linux
}

#ifndef __RTXC__
/******************************************************************************/
/**
 * Queries and registers for desired beam at sonar source hostname
 *
 * @param hostname   hostname of sonar source
 * @param u8BeamType beam id
 *
 * @return BOOLEAN - TRUE if success, FALSE if fail
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bRegisterForSonarData( char *hostname, U8 u8BeamType )
{
   int   QRsocket;  // query-register socket
   int   bytes;
   char  sockBuf[QR_SOCKET_BUFSIZE];
   int   state = QUERY;

   SOCKET_BEAM        beamInfo;
   struct sockaddr_in QRaddr;

   memset( (void *)&beamInfo, 0, sizeof( SOCKET_BEAM ));

   // start simple state machine to request beam from 'hostname'

   while ( state < DONE )
   {
      if ( !GSOCKET_bOpenSocket( &QRsocket, &QRaddr, hostname, 0, SONAR_PORT, FALSE, FALSE ))
      {
         printf( "CreateControlThread: error creating socket\n" );
         return( FALSE );
      }

#if DEBUG_GSOCKET
      printf( "CreateControlThread: Attempting to connect to %s at port %d\n", hostname, SONAR_PORT );
#endif
      if (connect( QRsocket, (struct sockaddr *)&QRaddr, sizeof(QRaddr)) < 0 )
      {
         perror("CreateControlThread: connect()");
         return( FALSE );
      }

      if ( state == QUERY )
      {
         sprintf( sockBuf, "%s", QUERY_MSG );
      }
      else if ( state == REGISTER )
      {
         sprintf( sockBuf, "%s%02d", REGISTER_BEAM, beamInfo.u8BeamNumber );
      }
      else
      {
         printf( "CreateControlThread ERROR: Bad state = %d\n", state );
         break;
      }

      if (( bytes = send(QRsocket, sockBuf, strlen(sockBuf)+1,0)) != strlen(sockBuf)+1 )
      {
         perror("CreateControlThread: send()");
         close(QRsocket);
         return( FALSE );
      }
#if DEBUG_GSOCKET
      printf( "CreateControlThread: Sent '%s' to %s at port %d\n", sockBuf, hostname, SONAR_PORT );
#endif

      memset( sockBuf, 0, QR_SOCKET_BUFSIZE );

      if (( bytes = recv(QRsocket, sockBuf, QR_SOCKET_BUFSIZE, 0)) < 0 )
      {
         perror("CreateControlThread: recv()");
         close(QRsocket);
         return( FALSE );
      }
      else
      {
         SOCKET_MSG *pMsg = (SOCKET_MSG *)sockBuf;

         if ( bytes == 0 )
         {
#if DEBUG_GSOCKET
            printf( "CreateControlThread: Received zero bytes from socket.\n" );
#endif
         }
         else if ( pMsg->u16Size != bytes )
         {
#if DEBUG_GSOCKET
            printf( "Received partial message! (FIXME!) recv=%d, msgSize=%d\n", bytes, pMsg->u16Size );
#endif
         }
         else
         {
            switch( pMsg->u16Type )
            {
               case DATA_SOURCES:
                  if ( state == QUERY )
                  {
                     if ( !GSOCKET_bParseSonarResp( (SOCKET_DATA_SOURCES *)sockBuf, u8BeamType, &beamInfo ))
                     {
                        printf( "CreateControlThread: desired beam not found in query\n" );
                        state = ERROR;
                     }
#if DEBUG_GSOCKET
                     GSOCKET_vPrintSonarData( (SOCKET_DATA_SOURCES *)sockBuf );
#endif
                  }
                  else
                  {
                     printf( "CreateControlThread: incorrect query sequence\n" );
                     state = ERROR;
                  }
                  break;

               case SOCKET_ERROR:
                  printf( "CreateControlThread: Received SOCKET_ERROR\n" );
                  state = ERROR;
                  break;

               case REGISTER_ACK:
                  if ( state != REGISTER )
                  {
                     printf( "CreateControlThread: Received REGISTER_ACK (incorrect response)\n" );
                  }
#if DEBUG_GSOCKET
                  else
                  {
                     printf( "CreateControlThread: Received REGISTER_ACK\n" );
                  }
#endif
                  break;

               default:
                  printf( "CreateControlThread: Received unknown message type: %d\n", pMsg->u16Type );
                  state = ERROR;
                  break;
            }
         }
      }
      close(QRsocket);  // close the socket after each msg send
      state++;
   }

   if ( state == DONE )
   {
      return( TRUE );
   }
   else
   {
      return( FALSE );
   }
}

/******************************************************************************/
/**
 * Creates socket for display thread and initializes it
 * 
 * @param sockPtr pointer to socket
 * @param u32Port pointer to port
 * 
 * @return BOOLEAN - TRUE if success, FALSE if fail
 */
/******************************************************************************/
GLOBAL BOOLEAN GSOCKET_bInitializeDisplaySocket( int *sockPtr, U32* u32Port )
{
   struct sockaddr_in listenAddr;

   if ( !GSOCKET_bOpenSocket( sockPtr, &listenAddr, NULL, htonl(INADDR_ANY), (U16)*u32Port, FALSE, FALSE ))
   {
      return( FALSE );
   }
   if ( bind( *sockPtr, (struct sockaddr *)&listenAddr, sizeof( listenAddr )) < 0 )
   {
      perror( "bind()" );
      return( FALSE );
   }
   listen( *sockPtr, 4 );  // start listening on the created socket

#if DEBUG_GSOCKET
   printf( "\nCreated socket, listening on port %d...\n", *u32Port );
#endif
   return( TRUE );
}

/******************************************************************************/
/**
 * Looks at the query resp to find the desired beam.  If found, copy the
 * SOCKET_BEAM info to pFoundBeam
 *
 * @param pQueryResp Pointer to query response
 * @param u8BeamType desired beam id
 * @param pFoundBeam pointer to found beam
 *
 * @return BOOLEAN - TRUE if successful
 */
/******************************************************************************/
PRIVATE BOOLEAN GSOCKET_bParseSonarResp( SOCKET_DATA_SOURCES *pQueryResp, U8 u8BeamType,
   SOCKET_BEAM *pFoundBeam )
{
   int i;
   SOCKET_BEAM *pBeam = pQueryResp->Beam;

   for (i=0; i < pQueryResp->u8NumberOfBeams; i++ )
   {
      if ( pBeam->u8Type == u8BeamType ) // found desired beam
      {
         *pFoundBeam = *pBeam; // struct copy (not pointer copy)
         return( TRUE );
      }
      pBeam++;
   }
   return( FALSE );
}

#if DEBUG_GSOCKET
/******************************************************************************/
/**
 * Print query response message for debug
 *
 * @param pQueryResp query response message
 */
/******************************************************************************/
PRIVATE void GSOCKET_vPrintSonarData( SOCKET_DATA_SOURCES *pQueryResp )
{
   int i;
   SOCKET_BEAM *pBeam = pQueryResp->Beam;

   printf( "Received sonar data message:\n\n" );
   printf( "\tMessage type %d, size %d\n", pQueryResp->Msg.u16Type, pQueryResp->Msg.u16Size );
   printf( "\tNumber of Beams = %d\n", pQueryResp->u8NumberOfBeams );

   for (i=0; i < pQueryResp->u8NumberOfBeams; i++ )
   {
      printf( "\t\tBeam %d, type %d, properties 0x%02X, frequency %d KHz\n",
         pBeam->u8BeamNumber, pBeam->u8Type, pBeam->u8Properties, pBeam->u32BeamFreq );
      pBeam++;
   }
   printf( "\n" );
}
#endif // DEBUG_GSOCKET
#endif //#ifndef __RTXC__
// end of socket.c
//#endif //MODEL_HAS_NETWORKING

