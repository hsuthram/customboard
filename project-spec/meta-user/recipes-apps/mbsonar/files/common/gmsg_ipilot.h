/*****************************************************************************
*         Copyright (c) 2011  Johnson Outdoors Marine Electronics, Inc.      *
*       - Contains CONFIDENTIAL and TRADE SECRET information -               *
******************************************************************************

   Description:

      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      + This software is the proprietary property of:                   +
      +                                                                 +
      +  Johnson Outdoors Marine Electronics, Inc.                      +
      +                                                                 +
      +  1220 Old Alpharetta Rd           1 Humminbird Lane             +
      +  Suite 340                        Eufaula, AL  36027            +
      +  Alpharetta, GA  30005                                          +
      +                                                                 +
      +        Use, duplication, or disclosure of this material,        +
      +          in any form, is forbidden without permission           +
      +         from Johnson Outdoors Marine Electronics, Inc.          +
      +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

*****************************************************************************/


/********************           COMPILE FLAGS                ****************/
#ifndef __GMSG_IPILOT_H
#define __GMSG_IPILOT_H

/********************           INCLUDE FILES                ****************/

#define MATRIX_BUILD    // Comment out for UNIFIED

#ifdef MATRIX_BUILD
#include "u_nav_twr.h"
#include "datatype.h"
#else // UNIFIED
#include "ipilot_defs.h"
#endif

#include "u_ipilot_common.h"
#include "u_ipcrypt.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************               ENUMS                    ****************/

/********************              DEFINES                   ****************/

#define GMSG_IPILOT_FILEID_CTLR        ('C')
#define GMSG_IPILOT_FILEID_REMOTE      ('R')

#ifdef MATRIX_BUILD

   #define DT_DYNAMIC_ARRAY               DYNAMIC_ARRAY

#else

   #define DT_UNSIGNED64                  D_UNSIGNED64
   #define DT_SIGNED32                    D_SIGNED32
   #define DT_UNSIGNED32                  D_UNSIGNED32
   #define DT_UNSIGNED16                  D_UNSIGNED16
   #define DT_UNSIGNED8                   D_UNSIGNED8

#endif

/********************               MACROS                   ****************/

/********************         TYPE DEFINITIONS               ****************/

typedef struct tag_GMSG_IPILOT_REGISTER
{
   U32 u32version;
   U32 u32tcpPort;
} GMSG_IPILOT_REGISTER;

#ifdef SUPPORT_MATRIX_FF
typedef struct tag_GMSG_IPILOT_WAYPOINT
{
   U32  u32commFlags;
   char sName[MAX_LENGTH_WAYPOINT_NAME];
   U32  u32DateTime;
   U16  u16__index;
   U16  u16depth;
   U8   u8icon;
   U8   u8status;
   S32  s32coordX;
   S32  s32coordY;
   U16  u16distance;
   U16  u16bearing;
   U32  u32UID_H;
   U32  u32UID_L;
   S32  s32OriginPtX;
   S32  s32OriginPtY;
} GMSG_IPILOT_WAYPOINT;
#endif // SUPPORT_MATRIX_FF


typedef struct tag_GMSG_IPLT_U_WAYPOINT
{
   U32      u32commFlags;
   UCHAR16  uName[MAX_LENGTH_WAYPOINT_NAME];
   U32      u32DateTime;
   U16      u16depth;
   U8       u8icon;
   U8       u8status;
   S32      s32coordX;
   S32      s32coordY;
   U32      u32UID_H;
   U32      u32UID_L;
   S32      s32OriginPtX;
   S32      s32OriginPtY;
} GMSG_IPLT_U_WAYPOINT;

// Itrack data GMSG structure, has multiple usees, transferring:
//    Itrack data (daPoints.nMembers > 0)
//    data  ack  (daPoints.nMembers == 0, Status set)
//    member of Itrack list (daPoints.nMembers == 0)
#ifdef SUPPORT_MATRIX_FF
typedef struct tag_GMSG_IPILOT_ITRACK
{
   U32  u32commFlags;
   char sName[MAX_LENGTH_ITRACK_NAME];
   U32  u32UID_L;
   U32  u32UID_H;
   U32  u32DateTime;
   U16  u16Segment;
   U8   u8Type;
   U8   u8Status;
   U16  u16distance;
   U16  u16bearing;
   U32  u32TotalLength;
   U32  u32LastUsed;
   U32  u32Style;
   U32  u32Color;
   U32  u32ClosestPt;
   S32  s32OriginPtX;
   S32  s32OriginPtY;
   DT_DYNAMIC_ARRAY daPoints;
} GMSG_IPILOT_ITRACK;
#endif // SUPPORT_MATRIX_FF

typedef struct tag_GMSG_IPLT_U_ITRACK
{
   U32      u32commFlags;
   UCHAR16  uName[MAX_LENGTH_ITRACK_NAME];
   U32      u32UID_L;
   U32      u32UID_H;
   U32      u32DateTime;
   U16      u16Segment;
   U8       u8Type;
   U8       u8Status;
   U16      u16distance;
   U16      u16bearing;
   U32      u32TotalLength;
   U32      u32LastUsed;
   U32      u32Style;
   U32      u32Color;
   U32      u32ClosestPt;
   S32      s32OriginPtX;
   S32      s32OriginPtY;
   DT_DYNAMIC_ARRAY daPoints;
} GMSG_IPLT_U_ITRACK;

#ifdef SUPPORT_MATRIX_FF
typedef struct tag_GMSG_IPILOT_ITRACK_HDR
{
   U32      u32commFlags;
   char     sName[MAX_LENGTH_ITRACK_NAME];
   U32      u32DateTime;
   U32      u32TotalLength;
   U32      u32LastUsed;
   S32      s32Lat;
   S32      s32Lon;
} GMSG_IPILOT_ITRACK_HDR;
#endif // SUPPORT_MATRIX_FF

typedef struct tag_GMSG_IPLT_U_ITRACK_HDR
{
   U32      u32commFlags;
   UCHAR16  uName[MAX_LENGTH_ITRACK_NAME];
   U32      u32DateTime;
   U32      u32TotalLength;
   U32      u32LastUsed;
   S32      s32Lat;
   S32      s32Lon;
} GMSG_IPLT_U_ITRACK_HDR;

#ifdef SUPPORT_MATRIX_FF
typedef struct tag_GMSG_IPILOT_ITRACK_FULLHDR
{
   U32            u32commFlags;
   ITRACK_HEADER  tHeader;
} GMSG_IPILOT_ITRACK_FULLHDR;
#endif // SUPPORT_MATRIX_FF

typedef struct tag_GMSG_IPLT_U_ITRACK_FULLHDR
{
   U32            u32commFlags;
   ITRACK_HEADER  tHeader;
} GMSG_IPLT_U_ITRACK_FULLHDR;

#ifdef SUPPORT_MATRIX_FF
typedef struct tag_GMSG_IPILOT_ROUTE
{
   U32      u32commFlags;
   char     sName[MAX_LENGTH_ROUTE_NAME];
   U32      u32Count;
   U32      u32DateTime;
   U32      u32TargetPt;
   U32      u32TotalLengthFt;
   U32      u32LastUsed;
   S32      s32OriginPtX;
   S32      s32OriginPtY;
   DT_DYNAMIC_ARRAY daWaypts;
} GMSG_IPILOT_ROUTE;
#endif // SUPPORT_MATRIX_FF

typedef struct tag_GMSG_IPLT_U_ROUTE
{
   U32      u32commFlags;
   UCHAR16  uName[MAX_LENGTH_ROUTE_NAME];
   U32      u32Count;
   U32      u32DateTime;
   U32      u32TargetPt;
   U32      u32TotalLengthFt;
   U32      u32LastUsed;
   S32      s32OriginPtX;
   S32      s32OriginPtY;

   U32      u32UID_L;
   U32      u32UID_H;

   DT_DYNAMIC_ARRAY daWaypts;
} GMSG_IPLT_U_ROUTE;

typedef struct tag_GMSG_IPILOT_QUERY
{
   U16      u16range;     // max range to search (in mercator meters?)
   U8       u8SortOrder;  //
   U8       u8typeMask;   // bitmap of types to search

   S32      s32coordX;    // X and Y coordinate location to start search
   S32      s32coordY;    //
} GMSG_IPILOT_QUERY;

typedef struct tag_MSG_IPILOT_STATUS
{
   U8       u8CurentMode;
   U8       u8CcFlags;
   U8       u8ApFlags;
   U8       u8PropStatus;
   U8       u8PropThrust;
   U16      u16MotorHeading;
} GMSG_IPILOT_STATUS;

typedef struct tag_MSG_IPILOT_OPSTATUS
{
   U32      u32Command;
   S32      s32Lat;
   S32      s32Lon;
   U32      bNewTarget;
   U32      u32Distance1;
   U32      u32Distance2;
   U32      u32Bearing;
} GMSG_IPILOT_OPSTATUS;

typedef struct tag_MSG_IPILOT_CMD
{
   U32 u32Command;
   U32 u32CmdParam1;
   U32 u32CmdParam2;
} GMSG_IPILOT_CMD;

typedef struct tag_MSG_IPILOT_CFGITEM
{
   U32            u32TimeStamp;     /* Config Timestamp */
   FOURCHARID     tConfigId;        /* 4 char identifier of the databse item */
   U32            u32ConfigValu;    /* The latest value */
} GMSG_IPILOT_CFGITEM;

typedef struct tag_MSG_IPILOT_DBCONFIG
{
   FOURCHARID     tConfigId;        /* 4 char identifier of the databse item */
   U32            u32ConfigValu;    /* The latest value */
} GMSG_IPILOT_DBCONFIG;

typedef struct tag_MSG_IPILOT_CFGLIST
{
   U32                  u32TimeStamp;     /* Config Timestamp */
   U32                  nNumberConfig;
   DT_DYNAMIC_ARRAY     daConfigItems;
} GMSG_IPILOT_CFGLIST;

typedef struct tag_MSG_IPILOT_SWUPD
{
   U8  u8SubCmd;
   U8  u8FileId;
   U16 u16Version;
} GMSG_IPILOT_SWUPD;

typedef enum tag_IPILOT_SWUPD_CMD
{
   IPILOT_SWUPD_HAVE_VERSION,
   IPILOT_SWUPD_LOAD_FILE,
   IPILOT_SWUPD_ACK,
   IPILOT_SWUPD_NAK,
   IPILOT_SWUPD_ERROR,
} IPILOT_SWUPD_CMD;

typedef struct tag_MSG_IPILOT_SCREENSHOT
{
   U32 u32SubCmd;
   U32 u32Offset;
   DT_DYNAMIC_ARRAY daData;
} GMSG_IPILOT_SCREENSHOT;


// meta-data sent in the 64-bit Token with GMSGs
// must be 64 bits long
typedef union _tag_IPILOT_GMSG_TOKEN
{
   U64 u64Token;     // forces aligment and size
   struct {
      U8  u8RemoteId;
      U8  u8Action;
      U16 u16Index;
      U8  u8Flag;
   } s;
} IPILOT_GMSG_TOKEN;

#ifdef SUPPORT_MATRIX_FF
typedef struct tag_GMSG_IPILOT_NAME_CHANGE
{
   U32  u32commFlags;
   U32  u32Command;
   char sName[MAX_LENGTH_ITRACK_NAME];
} GMSG_IPILOT_NAME_CHANGE;
#endif // SUPPORT_MATRIX_FF

typedef struct tag_GMSG_IPLT_U_NAME_CHANGE
{
   U32      u32commFlags;
   U32      u32Command;
   UCHAR16  uName[MAX_LENGTH_ITRACK_NAME];
} GMSG_IPLT_U_NAME_CHANGE;

typedef struct tag_GMSG_IPLT_GPS_DATA
{
   U32      u32commFlags;
   U32      u32FixFlags;
   U32      u32UTCtime;
   S32      s32Longitude;
   S32      s32Latitude;
   S32      s32Height;
   S32      s32NEDnorth;
   S32      s32NEDeast;
   S32      s32SOG;
   S32      s32COG;
   U32      u32EPE;
   U32      u32HDOP;
} GMSG_IPLT_GPS_DATA;

typedef struct tag_GMSG_IPLT_HDG_DATA
{
   U32      u32commFlags;
   S32      s32Heading;
   S32      s32Pitch;
   S32      s32Roll;
} GMSG_IPLT_HDG_DATA;

/********************        FUNCTION PROTOTYPES             ****************/

/********************          LOCAL VARIABLES               ****************/

/********************          GLOBAL VARIABLES              ****************/

#ifdef IPILOT_DECLARE_GLOBALS

// iPilot Register GMSG data definition
// used for
//    GMSG_ID_IPILOT_REGISTER
//
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_REGISTER, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_REGISTER), NULL,
      DT_UNSIGNED32, DT_UNSIGNED32);

// WAYPOINT name array
//
#ifdef SUPPORT_MATRIX_FF
DT_ARRAY_TYPE(GLOBAL, DT_IPILOT_WAYPOINT_NAME,
      MAX_LENGTH_WAYPOINT_NAME, 1, DT_UNSIGNED8);
#endif // SUPPORT_MATRIX_FF

DT_ARRAY_TYPE(GLOBAL, DT_IPLT_U_WAYPOINT_NAME,
      MAX_LENGTH_WAYPOINT_NAME, 2, DT_UNSIGNED16);

// iPilot Waypoint GMSG data definition
// used for
//    GMSG_ID_IPILOT_STORE_WAYPOINT
//    GMSG_ID_IPILOT_STORE_WAYPOINT_ACK
//    GMSG_ID_IPILOT_DISPLAY_GOTO
//
#ifdef SUPPORT_MATRIX_FF
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_WAYPOINT, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_WAYPOINT), NULL,
      DT_UNSIGNED32, DT_IPILOT_WAYPOINT_NAME,
      DT_UNSIGNED32, DT_UNSIGNED16, DT_UNSIGNED16,
      DT_UNSIGNED8, DT_UNSIGNED8,
      DT_SIGNED32, DT_SIGNED32,
      DT_UNSIGNED16, DT_UNSIGNED16,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_SIGNED32, DT_SIGNED32 );
#endif // SUPPORT_MATRIX_FF

DT_STRUCT_TYPE(GLOBAL, DT_IPLT_U_WAYPOINT, 1, NULL, NULL,
      sizeof(GMSG_IPLT_U_WAYPOINT), NULL,
      DT_UNSIGNED32, DT_IPLT_U_WAYPOINT_NAME,
      DT_UNSIGNED32, DT_UNSIGNED16, DT_UNSIGNED8, DT_UNSIGNED8,
      DT_SIGNED32, DT_SIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_SIGNED32, DT_SIGNED32 );

// iPilot Waypoint LIST GMSG data definition
//
// DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_IPILOT_WAYPOINT_LIST, DT_IPILOT_WAYPOINT);

// ITRACK name array
//
#ifdef SUPPORT_MATRIX_FF
DT_ARRAY_TYPE(GLOBAL, DT_IPILOT_ITRACK_NAME,
      MAX_LENGTH_ITRACK_NAME, 1, DT_UNSIGNED8);
#endif // SUPPORT_MATRIX_FF

DT_ARRAY_TYPE(GLOBAL, DT_IPLT_U_ITRACK_NAME,
      MAX_LENGTH_ITRACK_NAME, 2, DT_UNSIGNED16);

// Coordinate pair structure
//
#ifdef SUPPORT_MATRIX_FF
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_POINT, 1, NULL, NULL,
      sizeof(POINT), NULL,
      DT_SIGNED32, DT_SIGNED32);

// Coordinate list array
//
DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_IPILOT_COORDINATE_LIST, DT_IPILOT_POINT);
#endif // SUPPORT_MATRIX_FF

DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_IPLT_U_COORDINATE_LIST, DT_SIGNED32);

// iPilot ITrack Header GMSG data definition
//
#ifdef SUPPORT_MATRIX_FF
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_ITRACK_HDR, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_ITRACK_HDR), NULL,
      DT_UNSIGNED32, DT_IPILOT_ITRACK_NAME,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32, DT_UNSIGNED32);
#endif // SUPPORT_MATRIX_FF

DT_STRUCT_TYPE(GLOBAL, DT_IPLT_U_ITRACK_HDR, 1, NULL, NULL,
      sizeof(GMSG_IPLT_U_ITRACK_HDR), NULL,
      DT_UNSIGNED32, DT_IPLT_U_ITRACK_NAME,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32, DT_UNSIGNED32);

#ifdef SUPPORT_MATRIX_FF
DT_ARRAY_TYPE(GLOBAL, DT_IPILOT_ITRACK_THEADER,
      MAX_ITRACK_HEADER_SIZE, 1, DT_UNSIGNED8);
#endif // SUPPORT_MATRIX_FF

DT_ARRAY_TYPE(GLOBAL, DT_IPLT_U_ITRACK_THEADER,
      MAX_ITRACK_HEADER_SIZE, 1, DT_UNSIGNED8);

#ifdef SUPPORT_MATRIX_FF
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_ITRACK_FULLHDR, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_ITRACK_FULLHDR), NULL,
      DT_UNSIGNED32, DT_IPILOT_ITRACK_THEADER);
#endif // SUPPORT_MATRIX_FF

DT_STRUCT_TYPE(GLOBAL, DT_IPLT_U_ITRACK_FULLHDR, 1, NULL, NULL,
      sizeof(GMSG_IPLT_U_ITRACK_FULLHDR), NULL,
      DT_UNSIGNED32, DT_IPLT_U_ITRACK_THEADER);

// iPilot ITrack GMSG data definition (unencrypted)
//
#ifdef SUPPORT_MATRIX_FF
DT_STRUCT_TYPE(GLOBAL, uDT_IPILOT_ITRACK, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_ITRACK), NULL,
      DT_UNSIGNED32, DT_IPILOT_ITRACK_NAME,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED16, DT_UNSIGNED8, DT_UNSIGNED8,
      DT_UNSIGNED16, DT_UNSIGNED16,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_IPILOT_COORDINATE_LIST);
#endif // SUPPORT_MATRIX_FF

DT_STRUCT_TYPE(GLOBAL, uDT_IPLT_U_ITRACK, 1, NULL, NULL,
      sizeof(GMSG_IPLT_U_ITRACK), NULL,
      DT_UNSIGNED32, DT_IPLT_U_ITRACK_NAME,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED16, DT_UNSIGNED8, DT_UNSIGNED8,
      DT_UNSIGNED16, DT_UNSIGNED16,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_IPLT_U_COORDINATE_LIST);

// Encryption wrapper for iPilot ITrack GMSG
//
#ifdef SUPPORT_MATRIX_FF
DT_IPCRYPT_WRAPPER(GLOBAL, DT_IPILOT_ITRACK, uDT_IPILOT_ITRACK);
#endif // SUPPORT_MATRIX_FF

DT_IPCRYPT_WRAPPER(GLOBAL, DT_IPLT_U_ITRACK, uDT_IPLT_U_ITRACK);

// ROUTE name array
//
#ifdef SUPPORT_MATRIX_FF
DT_ARRAY_TYPE(GLOBAL, DT_IPILOT_ROUTE_NAME,
      MAX_LENGTH_ROUTE_NAME, 1, DT_UNSIGNED8);
#endif // SUPPORT_MATRIX_FF

DT_ARRAY_TYPE(GLOBAL, DT_IPLT_U_ROUTE_NAME,
      MAX_LENGTH_ROUTE_NAME, 2, DT_UNSIGNED16);

// Route list array
//
#ifdef SUPPORT_MATRIX_FF
DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_IPILOT_ROUTE_LIST, DT_IPILOT_WAYPOINT);
#endif // SUPPORT_MATRIX_FF

DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_IPLT_U_ROUTE_LIST, DT_IPLT_U_WAYPOINT);

// iPilot ROUTE GMSG data definition
//
#ifdef SUPPORT_MATRIX_FF
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_ROUTE, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_ROUTE), NULL,
      DT_UNSIGNED32, DT_IPILOT_ROUTE_NAME,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32,
      DT_SIGNED32,   DT_SIGNED32,
      DT_IPILOT_ROUTE_LIST);
#endif // SUPPORT_MATRIX_FF

// iPilot Unified ROUTE GMSG data definition
//
DT_STRUCT_TYPE(GLOBAL, DT_IPLT_U_ROUTE, 1, NULL, NULL,
      sizeof(GMSG_IPLT_U_ROUTE), NULL,
      DT_UNSIGNED32, DT_IPLT_U_ROUTE_NAME,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32,
      DT_SIGNED32,   DT_SIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_IPLT_U_ROUTE_LIST);

// iPilot ITRACK LIST GMSG data definition
//
// DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_IPILOT_ITRACK_LIST, DT_IPILOT_ITRACK);

// iPilot Query (waypoints or itracks)
//
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_QUERY, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_QUERY), NULL,
      DT_UNSIGNED16, DT_UNSIGNED8, DT_UNSIGNED8, DT_SIGNED32, DT_SIGNED32);

DT_ARRAY_TYPE(GLOBAL, DT_IPILOT_QUERY_NAME,
      QUERY_NAME_LENGTH, 1, DT_UNSIGNED8);

DT_ARRAY_TYPE(GLOBAL, DT_IPLT_U_QUERY_NAME,
      QUERY_NAME_LENGTH, 2, DT_UNSIGNED16);

// QUERY_LIST_DATA from u_ipilot_common.h
//
#ifdef SUPPORT_MATRIX_FF
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_QUERY_DATA, 1, NULL, NULL,
      sizeof(QUERY_LIST_DATA), NULL,
      DT_IPILOT_QUERY_NAME,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED16, DT_UNSIGNED16, DT_UNSIGNED16,
      DT_UNSIGNED8, DT_UNSIGNED8);

DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_IPILOT_QUERY_LIST, DT_IPILOT_QUERY_DATA);
#endif // SUPPORT_MATRIX_FF

DT_STRUCT_TYPE(GLOBAL, DT_IPLT_U_QUERY_DATA, 1, NULL, NULL,
      sizeof(QUERY_LIST_DATA), NULL,
      DT_IPLT_U_QUERY_NAME,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_UNSIGNED16, DT_UNSIGNED16, DT_UNSIGNED16,
      DT_UNSIGNED8, DT_UNSIGNED8);

DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_IPLT_U_QUERY_LIST, DT_IPLT_U_QUERY_DATA);


// Status Reports message data
//
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_STATUS, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_STATUS), NULL,
      DT_UNSIGNED8, DT_UNSIGNED8, DT_UNSIGNED8, DT_UNSIGNED8, DT_UNSIGNED8, DT_UNSIGNED16 );

// Operational Status Update
//
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_OPSTATUS, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_OPSTATUS), NULL,
      DT_UNSIGNED32, DT_SIGNED32, DT_SIGNED32, DT_UNSIGNED32, DT_UNSIGNED32, DT_UNSIGNED32, DT_UNSIGNED32);

// Generic Command message (unencrypted)
//
DT_STRUCT_TYPE(GLOBAL, uDT_IPILOT_COMMAND, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_CMD), NULL,
      DT_UNSIGNED32, DT_UNSIGNED32, DT_UNSIGNED32 );

// Encryption wrapper for Generic Command GMSGs
//
DT_IPCRYPT_WRAPPER(GLOBAL, DT_IPILOT_COMMAND, uDT_IPILOT_COMMAND);

// Config Item data
//
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_CFGITEM, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_CFGITEM), NULL,
      DT_UNSIGNED32, DT_UNSIGNED32, DT_UNSIGNED32 );

DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_DBCONFIG, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_DBCONFIG), NULL,
      DT_UNSIGNED32, DT_UNSIGNED32 );

//  Config LIST GMSG data definition
//
DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_IPILOT_CONFIG_ITEMLIST, DT_IPILOT_DBCONFIG);

// Generic Config message
//
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_CFGLIST, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_CFGLIST), NULL,
      DT_UNSIGNED32, DT_UNSIGNED32, DT_IPILOT_CONFIG_ITEMLIST);


// Software update messages
//
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_SWUPD, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_SWUPD), NULL,
      DT_UNSIGNED8, DT_UNSIGNED8, DT_UNSIGNED16);

// Screnshot data
//
DT_DYNAMIC_ARRAY_TYPE(GLOBAL, DT_ARRAY_U8, DT_UNSIGNED8);

DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_SCREENSHOT, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_SCREENSHOT), NULL,
      DT_UNSIGNED32, DT_UNSIGNED32, DT_ARRAY_U8);

// Authentication Gmsgs
//
DT_OPAQUE_TYPE(GLOBAL, DT_IPILOT_AUTH, sizeof(IPCRYPT_AUTH), sizeof(U32));

// ITrack Name change GMSG data definition
//
#ifdef SUPPORT_MATRIX_FF
DT_STRUCT_TYPE(GLOBAL, DT_IPILOT_NAME_CHANGE, 1, NULL, NULL,
      sizeof(GMSG_IPILOT_NAME_CHANGE), NULL,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_IPILOT_ITRACK_NAME );
#endif // SUPPORT_MATRIX_FF

DT_STRUCT_TYPE(GLOBAL, DT_IPLT_U_NAME_CHANGE, 1, NULL, NULL,
      sizeof(GMSG_IPLT_U_NAME_CHANGE), NULL,
      DT_UNSIGNED32, DT_UNSIGNED32,
      DT_IPLT_U_ITRACK_NAME );


DT_STRUCT_TYPE(GLOBAL, DT_IPLT_PATTERN_MODE, 1, NULL, NULL,
               sizeof(IPLT_PATTERN_MODE_DATA), NULL,
               DT_UNSIGNED32, DT_UNSIGNED32,
               DT_SIGNED32, DT_SIGNED32,
               DT_SIGNED32, DT_SIGNED32,
               DT_SIGNED32);

// Ethernet Data Sharing GMSG data definition
//
DT_STRUCT_TYPE(GLOBAL, DT_IPLT_GPS_DATA, 1, NULL, NULL,
               sizeof(GMSG_IPLT_GPS_DATA), NULL,
               DT_UNSIGNED32,DT_UNSIGNED32,
               DT_UNSIGNED32,
               DT_SIGNED32, DT_SIGNED32,
               DT_SIGNED32, DT_SIGNED32,
               DT_SIGNED32, DT_SIGNED32,
               DT_SIGNED32,
               DT_UNSIGNED32, DT_UNSIGNED32);

DT_STRUCT_TYPE(GLOBAL, DT_IPLT_HDG_DATA, 1, NULL, NULL,
               sizeof(GMSG_IPLT_HDG_DATA), NULL,
               DT_UNSIGNED32,
               DT_SIGNED32, DT_SIGNED32,
               DT_SIGNED32 );

#else // ! IPILOT_DECLARE_GLOBALS

DT_EXTERN_TYPE( DT_IPILOT_REGISTER );

DT_EXTERN_TYPE( DT_IPILOT_QUERY );
DT_EXTERN_TYPE( DT_IPILOT_STATUS );
DT_EXTERN_TYPE( DT_IPILOT_OPSTATUS );
DT_EXTERN_TYPE( DT_IPILOT_COMMAND );
DT_EXTERN_TYPE( DT_IPILOT_CFGITEM );
DT_EXTERN_TYPE( DT_IPILOT_DBCONFIG );
DT_EXTERN_TYPE( DT_IPILOT_CONFIG_ITEMLIST );
DT_EXTERN_TYPE( DT_IPILOT_CFGLIST );
DT_EXTERN_TYPE( DT_IPILOT_SWUPD );
DT_EXTERN_TYPE( DT_ARRAY_U8 );
DT_EXTERN_TYPE( DT_IPILOT_SCREENSHOT );
DT_EXTERN_TYPE( DT_IPILOT_AUTH );


#ifdef SUPPORT_MATRIX_FF
DT_EXTERN_TYPE( DT_IPILOT_WAYPOINT_NAME );
DT_EXTERN_TYPE( DT_IPILOT_WAYPOINT );
DT_EXTERN_TYPE( DT_IPILOT_ITRACK_NAME );
DT_EXTERN_TYPE( DT_IPILOT_POINT );
DT_EXTERN_TYPE( DT_IPILOT_COORDINATE_LIST );
DT_EXTERN_TYPE( DT_IPILOT_ITRACK_HDR );
DT_EXTERN_TYPE( DT_IPILOT_ITRACK_THEADER );
DT_EXTERN_TYPE( DT_IPILOT_ITRACK_FULLHDR );
DT_EXTERN_TYPE( DT_IPILOT_ITRACK );
DT_EXTERN_TYPE( DT_IPILOT_ROUTE_NAME );
DT_EXTERN_TYPE( DT_IPILOT_ROUTE_LIST );
DT_EXTERN_TYPE( DT_IPILOT_ROUTE );
DT_EXTERN_TYPE( DT_IPILOT_QUERY_NAME );
DT_EXTERN_TYPE( DT_IPILOT_QUERY_DATA );
DT_EXTERN_TYPE( DT_IPILOT_QUERY_LIST );
DT_EXTERN_TYPE( DT_IPILOT_NAME_CHANGE );
#endif // SUPPORT_MATRIX_FF


DT_EXTERN_TYPE( DT_IPLT_U_WAYPOINT_NAME );
DT_EXTERN_TYPE( DT_IPLT_U_WAYPOINT );
DT_EXTERN_TYPE( DT_IPLT_U_ITRACK_NAME );
DT_EXTERN_TYPE( DT_IPLT_U_POINT );
DT_EXTERN_TYPE( DT_IPLT_U_COORDINATE_LIST );
DT_EXTERN_TYPE( DT_IPLT_U_ITRACK_HDR );
DT_EXTERN_TYPE( DT_IPLT_U_ITRACK_THEADER );
DT_EXTERN_TYPE( DT_IPLT_U_ITRACK_FULLHDR );
DT_EXTERN_TYPE( DT_IPLT_U_ITRACK );
DT_EXTERN_TYPE( DT_IPLT_U_ROUTE_NAME );
DT_EXTERN_TYPE( DT_IPLT_U_ROUTE_LIST );
DT_EXTERN_TYPE( DT_IPLT_U_ROUTE );
DT_EXTERN_TYPE( DT_IPLT_U_QUERY_NAME );
DT_EXTERN_TYPE( DT_IPLT_U_QUERY_DATA );
DT_EXTERN_TYPE( DT_IPLT_U_QUERY_LIST );
DT_EXTERN_TYPE( DT_IPLT_U_NAME_CHANGE );
DT_EXTERN_TYPE( DT_IPLT_PATTERN_MODE );
DT_EXTERN_TYPE( DT_IPLT_GPS_DATA );
DT_EXTERN_TYPE( DT_IPLT_HDG_DATA );


#endif// ! IPILOT_DECLARE_GLOBALS

#ifdef __cplusplus
}
#endif


#endif//__GMSG_IPILOT_H
