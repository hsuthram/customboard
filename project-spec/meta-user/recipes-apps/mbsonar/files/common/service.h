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
#ifndef SERVICE_H
   #define SERVICE_H
#ifdef __cplusplus
   extern "C" {
#endif
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include "global.h"
#include "types.h"
#include "gmsg_addr.h"
#include "slp.h"

/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define SERVICE_FIRST_PORT          49152    //beginning of ephemeral ports (allow 16384 ports)
#define SERVICE_PORTS_PER_SERVICE   64       //64 ports should be more than anyone needs

#define SERVICE_ANY_ADDR            0x00000000UL
#define SERVICE_ALL_HOSTS_GROUP     0xe0000001UL        // 224.0.0.1

#define SERVICE_HANDLE_INVALID      ((SERVICE_HANDLE)NULL)
#define SERVICE_PORT_HB_RADAR_DEVICE   (10001)

#if MODEL_SUPPORTS_CHIRP_RADAR
#define SERVICE_PORT_HB_CHIRP_RADAR_DEVICE   (5800)
#endif // MODEL_SUPPORTS_RADAR_CHIRP

/********************               MACROS                   ******************/
#define SERVICE_PORT_BASE(class_id) (SERVICE_FIRST_PORT + ((class_id) * SERVICE_PORTS_PER_SERVICE))

/********************         TYPE DEFINITIONS               ******************/

typedef SLPHandle SERVICE_HANDLE;

typedef enum
{
   SERVICE_CLASS_DEFAULT = 0,   //for general-purpose messages used by many services GMSG_STATUS e.g.
   SERVICE_CLASS_SYSTEM,        //system management service
   SERVICE_CLASS_SERVICE,     //service manangement services
   SERVICE_CLASS_DATABASE,      //general-purpose database services (any service can offer them)
   SERVICE_CLASS_SERIAL,        //serial service
   SERVICE_CLASS_SONAR,         //sonar black box service
   SERVICE_CLASS_XSONAR,        //sonar linux-side control-unit service
   SERVICE_CLASS_NMEA,          //nmea service
   SERVICE_CLASS_GPS,           //gps service
   SERVICE_CLASS_AIS,           //ais service
   SERVICE_CLASS_COMPASS,       //compass service
   SERVICE_CLASS_AUTOPILOT,     //autopilot service
   SERVICE_CLASS_CHART,         //chart service
   SERVICE_CLASS_NAV,           //navigation service
   SERVICE_CLASS_TWR,           //track, waypoint, route storage service
   SERVICE_CLASS_RADAR,         //radar service
   SERVICE_CLASS_WEATHER,       //weather service
   SERVICE_CLASS_LOG,           //log service
   SERVICE_CLASS_SOCKET_SERVER, //socket server service
   SERVICE_CLASS_VIDEO,         //video input service
   SERVICE_CLASS_PANE,          //This class is a common interface between the sonar, radar, and chart tasks
                                //dealing with panes, for example point information, georeference,
                                //cursor/boat icon change etc.
   SERVICE_CLASS_TEST,          //test harness service class

   SERVICE_CLASS_HB_NMEA,		//Humminbird NMEA Data Interface
   SERVICE_CLASS_HB_TEMP,		//Humminbird TEMP Data Interface
   SERVICE_CLASS_HB_SONAR,		//Humminbird SONAR Data Interface
   SERVICE_CLASS_HB_ALARM,		//Humminbird ALARM Data Interface
   SERVICE_CLASS_HB_NAV,      //Humminbird NAV Data Interface
   SERVICE_CLASS_HB_MULTICAST,//Humminbird Multicast

   SERVICE_CLASS_IPILOT,        // iPilot handling service
   SERVICE_CLASS_BLUETOOTH,     // Bluetooth
   SERVICE_CLASS_HB_FCID,       // FCID database handling service
   //if you add a service, be sure to add a port base and add a string to s_cServiceClass in service.c
   SERVICE_CLASS_LAST,
   SERVICE_CLASS_COUNT = (SERVICE_CLASS_LAST-SERVICE_CLASS_DEFAULT)
} SERVICE_CLASS_TYPE;

typedef enum
{
   SERVICE_PORT_DEFAULT_BASE       = SERVICE_PORT_BASE(SERVICE_CLASS_DEFAULT),  /**> not used */
   SERVICE_PORT_SYSTEM_BASE        = SERVICE_PORT_BASE(SERVICE_CLASS_SYSTEM),   /**> reserved for system services */
   SERVICE_PORT_SERVICE_BASE       = SERVICE_PORT_BASE(SERVICE_CLASS_SERVICE),  /**> service management of services */
   SERVICE_PORT_DATABASE_BASE      = SERVICE_PORT_BASE(SERVICE_CLASS_DATABASE), /**> only for global database (if used) */
   SERVICE_PORT_SERIAL_BASE        = SERVICE_PORT_BASE(SERVICE_CLASS_SERIAL),
   SERVICE_PORT_SONAR_BASE         = SERVICE_PORT_BASE(SERVICE_CLASS_SONAR),
   SERVICE_PORT_XSONAR_BASE        = SERVICE_PORT_BASE(SERVICE_CLASS_XSONAR),
   SERVICE_PORT_NMEA_BASE          = SERVICE_PORT_BASE(SERVICE_CLASS_NMEA),
   SERVICE_PORT_GPS_BASE           = SERVICE_PORT_BASE(SERVICE_CLASS_GPS),
   SERVICE_PORT_AIS_BASE           = SERVICE_PORT_BASE(SERVICE_CLASS_AIS),
   SERVICE_PORT_COMPASS_BASE       = SERVICE_PORT_BASE(SERVICE_CLASS_COMPASS),
   SERVICE_PORT_AUTOPILOT_BASE     = SERVICE_PORT_BASE(SERVICE_CLASS_AUTOPILOT),
   SERVICE_PORT_CHART_BASE         = SERVICE_PORT_BASE(SERVICE_CLASS_CHART),
   SERVICE_PORT_NAV_BASE           = SERVICE_PORT_BASE(SERVICE_CLASS_NAV),
   SERVICE_PORT_TWR_BASE           = SERVICE_PORT_BASE(SERVICE_CLASS_TWR),
   SERVICE_PORT_RADAR_BASE         = SERVICE_PORT_BASE(SERVICE_CLASS_RADAR),
   SERVICE_PORT_WEATHER_BASE       = SERVICE_PORT_BASE(SERVICE_CLASS_WEATHER),
   SERVICE_PORT_LOG_BASE           = SERVICE_PORT_BASE(SERVICE_CLASS_LOG),
   SERVICE_PORT_SOCKET_SERVER_BASE = SERVICE_PORT_BASE(SERVICE_CLASS_SOCKET_SERVER),
   SERVICE_PORT_VIDEO_BASE         = SERVICE_PORT_BASE(SERVICE_CLASS_VIDEO),
   SERVICE_PORT_PANE_BASE          = SERVICE_PORT_BASE(SERVICE_CLASS_PANE),
   SERVICE_PORT_TEST_BASE          = SERVICE_PORT_BASE(SERVICE_CLASS_TEST),
   SERVICE_PORT_HB_NMEA_BASE       = SERVICE_PORT_BASE(SERVICE_CLASS_HB_NMEA),
   SERVICE_PORT_HB_TEMP_BASE       = SERVICE_PORT_BASE(SERVICE_CLASS_HB_TEMP),
   SERVICE_PORT_HB_SONAR_BASE      = SERVICE_PORT_BASE(SERVICE_CLASS_HB_SONAR),
   SERVICE_PORT_HB_ALARM_BASE      = SERVICE_PORT_BASE(SERVICE_CLASS_HB_ALARM),
   SERVICE_PORT_HB_NAV_BASE        = SERVICE_PORT_BASE(SERVICE_CLASS_HB_NAV),
   SERVICE_PORT_HB_MULTICAST_BASE  = SERVICE_PORT_BASE(SERVICE_CLASS_HB_MULTICAST),
   SERVICE_PORT_IPILOT_BASE        = SERVICE_PORT_BASE(SERVICE_CLASS_IPILOT),
   SERVICE_PORT_BLUETOOTH_BASE     = SERVICE_PORT_BASE(SERVICE_CLASS_BLUETOOTH),
   SERVICE_PORT_HB_FCID_BASE       = SERVICE_PORT_BASE(SERVICE_CLASS_HB_FCID),
} SERVICE_PORT_NUMBER;

typedef enum
{
   SERVICE_SEARCH_LOCAL,
   SERVICE_SEARCH_ADDRESS,
   SERVICE_SEARCH_GLOBAL
} SERVICE_SEARCH_SCOPE;

typedef struct
{
   char        *pcURL;
   char        *pcService;
   char        *pcInstance;
   GMSG_ADDRESS MsgAddress;
} SERVICE_ENTRY;

typedef struct
{
   int            nCount;
   SERVICE_ENTRY *ServiceInfoArray;
} SERVICE_SEARCH_INFO;

typedef struct
{
   int      nCount;
   char   **pcServiceTypes;
} SERVICE_TYPE_INFO;

/********************        FUNCTION PROTOTYPES             ******************/
extern SERVICE_HANDLE SERVICE_hOpenServiceHandle(void);
extern void           SERVICE_vCloseServiceHandle(SERVICE_HANDLE hServiceHandle);


extern BOOLEAN SERVICE_bRegister(SERVICE_HANDLE hServiceHandle, const char *pcService, char *pcInstance,
                                 GMSG_ADDRESS *pServiceAddress, char *pcAttributes);
extern BOOLEAN SERVICE_bUnregister(SERVICE_HANDLE hServiceHandle, char *pcService, char *pcInstance,
                                   GMSG_ADDRESS *pServiceAddress);
extern BOOLEAN SERVICE_bGetServiceTypeInfo(SERVICE_HANDLE hServiceHandle, SERVICE_TYPE_INFO *pServiceTypeInfo);
extern BOOLEAN SERVICE_bFreeServiceTypeInfo(SERVICE_TYPE_INFO *pServiceTypeInfo);
extern BOOLEAN SERVICE_bSearch(SERVICE_HANDLE hServiceHandle, SERVICE_SEARCH_INFO *pSearchInfo, const char *pcService,
                               char *pcInstance, U32 u32IpAddress, char *pcAttributeFilter,
                               SERVICE_SEARCH_SCOPE eSearchScope);
extern BOOLEAN SERVICE_bGetAttributes(SERVICE_HANDLE hServiceHandle, SERVICE_ENTRY *pServiceEntry, char *pAttributeIds, char *ppAttributeList);
extern BOOLEAN SERVICE_bParseAttribute(char *pAttributeList, char *pAttribute, char **ppValue);
extern BOOLEAN SERVICE_bFreeAttribute(char *pAttributeListOrValue);
extern BOOLEAN SERVICE_bFreeSearchInfo(SERVICE_SEARCH_INFO *pServiceSerachInfo);

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/
extern const char * const s_cServiceClass[];

/********************              FUNCTIONS                 ******************/
#ifdef __cplusplus
   }
#endif
#endif //SERVICE_H

