/*******************************************************************************
*           Copyright (c) 2002 - 2010 Techsonic Industries, Inc.               *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description:

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
#ifndef MESSAGE_H
   #define MESSAGE_H
/********************           INCLUDE FILES                ******************/
#include "global.h"
//#include "matrixOS.h"
//#include "model_features.h"
#include "l_mbox.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
typedef enum  /* Add new message commands here */
{
   CMD_ID_INVALID_ID = 0,     /* This should never be used by anything!!! */
   CMD_ID_SHUTDOWN   = 1,     /* Global message to tell tasks that shutdown is occurring */
   CMD_ID_NEW_DATA,           /* Sent by the database manager (h_database.c), whenever new data is available */
   CMD_ID_RESET_POS,          /* Sent by the keyboard manager when in POS mode */

   /* T_ADC messages */
   CMD_ID_SONAR_PROCESS_PING,          /* Process this ping data */
   CMD_ID_SONAR_PROCESS_TRIPLE_PING,   /* Process these three ping returns */
   CMD_ID_BLANK_SCROLL_PING,           /* Scroll the sonar display by adding a "blank" ping */
   CMD_ID_REVERSE_SCROLL_PING,         /* Read the Sonar file for the correct (2D)Ping in Reverse Scroll Mode */
   CMD_ID_NEW_PLAYBACK_STATE,          /* Send a new playback state to the t_n_process task */

   /* t_display messages */
   CMD_ID_DISPLAY_SCREEN,        /* A request to the display task to display a new screen */
   CMD_ID_SET_COLOR_PALETTE,     /* A request to the display task to change the color palette */
   CMD_ID_TRIGGER_SCREEN_DUMP,   /* A request to the display task to trigger a screen dump to the MMC */
   CMD_ID_START_VIDEO_OUT_CHECK, /* A request to the display task to start checking for video out connectivity */

   /* t_view messages */
   CMD_ID_REGISTER_LAYER,      /* A request to register a layer */
   CMD_ID_UNREGISTER_LAYER,    /* A request to unregister a layer */
   CMD_ID_PING_UPDATE,         /* Notification of new playback/recording status for S&R view */
   CMD_ID_PING_START,          /* Notification of start of recording for S&R view */

   /* t_menu messages */
   CMD_ID_REGISTER_MENU_TAB,
   CMD_ID_REGISTER_MENU_ITEM,
   CMD_ID_REGISTER_SUB_MENU,
   CMD_ID_REGISTER_HOT_MENU,
   CMD_ID_REGISTER_INFO_MENU,

   CMD_ID_REGISTER_FLOATING_MENU,
   CMD_ID_UNREGISTER_FLOATING_MENU,
   CMD_ID_REGISTER_FLOATING_MENU_ITEM,
   CMD_ID_UNREGISTER_FLOATING_MENU_ITEM,
   CMD_ID_ACTIVATE_FLOATING_MENU,
   //CMD_ID_REGISTER_SINGLE_CENTERED_MENU,
   CMD_ID_UNREGISTER_SINGLE_CENTERED_MENU,
   CMD_ID_REGISTER_SINGLE_CENTERED_MENU_ITEM,
   CMD_ID_ACTIVATE_SINGLE_CENTERED_MENU,

   CMD_ID_UNREGISTER_MENU_TAB,
   CMD_ID_UNREGISTER_MENU_ITEM,
   CMD_ID_ACTIVATE_INFO_MENU,
   CMD_ID_ACTIVATE_HOT_MENU,
   CMD_ID_CLEAR_MENU,
   CMD_ID_HIDE_MENU,
   CMD_ID_RESTORE_MENU,

   /* t_keyboard messages */
   CMD_ID_REGISTER_FOR_KEYS,   /* A request to register for one or more keys */
   CMD_ID_UNREGISTER_FOR_KEYS, /* A request to unregister for one or more keys */
   CMD_ID_KEY_CHANGE,          /* A message indicating that a key has changed */
   CMD_ID_INITIATE_SHUTDOWN,   /* A message commanding a clean system shutdown  */

   /* t_startup messages */
   CMD_ID_RUN_SYSTEM,          /* Tell the view task to go to the saved view and start using the view key */
                               /* Tell the menu task to go to start using the menu key */
   /* t_nvram messages */
   CMD_ID_WRITE_TO_HEADER,     /* A special case telling the NVR to save a database parameter in the NVR unit header */
   CMD_ID_WRITE_TO_NVR,        /* Force an immediate write to NVR */
   CMD_ID_SCRATCH_NVR,         /* a very special and dangerous command that erases all NVR */
   CMD_ID_READ_SONAR_DATA,     /* Read the data that is to be used by sonar testing */
   CMD_ID_WRITE_SONAR_DATA,    /* Write the data that is to be used by sonar testing */
   CMD_ID_READ_NMEA2K_DATA,     /* Read the data that is to be used by sonar testing */
   CMD_ID_WRITE_NMEA2K_DATA,    /* Write the data that is to be used by sonar testing */
   CMD_ID_READ_PERSONALIZED,   /* Read the data block that contains the personalization data */
   CMD_ID_WRITE_PERSONALIZED,  /* Write the data block that contains the personalization data */

   /* t_tone messages */
   CMD_ID_PLAY_TONE,           /* Instruct the tone task to play a tone */

   /* t_uart messages */
   CMD_ID_CHANGE_BAUD_RATE,    /* Change a uart's baud rate */
   CMD_ID_DISCONNECT,          /* Close an open channel */
   CMD_ID_CAPTURE_CHANNEL,     /* Grab or setup a conditional grab for a uart channel */

   /* t_inline_test messages */
   CMD_ID_USER_TEST,           /* Start User Test */

   /* t_acc messages */
   CMD_ID_UNIT_FOUND,          /* Sent to an accessory task when a unit is discovered */
   CMD_ID_ATTENTION,           /* Accessory needs attention */
   CMD_ID_UNIT_RESET,          /* Sent from an accessory task when a unit is reset */

   /* t_acc_comm messages */
   CMD_ID_SEND_2_ACCESSORY,    /* Send a message to an accessory */

   /* t_alarm messages */
   CMD_ID_SET_ALARM,           /* Indicate an alarm condition has been satisfied */
   CMD_ID_SET_EXT_ALARM,       /* Same as CMD_ID_SET_ALARM, with extended display and Cbs */
   CMD_ID_CLEAR_ALARM,

   /* t_mmc messages */
   CMD_ID_MMC_CHANGE_STATE,
   CMD_ID_EXPORT_NAV_DATA,     /* Tell the MMC task to save all nav data */
   CMD_ID_SCREEN_DUMP,         /* A request to the MMC task to get a screen dump */
   CMD_ID_LOAD_BMP,            /* A request to the MMC task to retrieve a bitmap from the MMC */
   CMD_ID_DELETE_IMAGE,        /* A request to the MMC task to delete a bitmap and associated .dat file from the MMC */
   CMD_ID_LOAD_THUMBNAIL,      /* A request to the MMC task to load a specified thumbnail .dat file */
   CMD_ID_LOAD_RECORDING,      /* A request to the MMC task to load a specified recording .dat file */
   CMD_ID_LOAD_XMWEATHER_IMAGE,/* A request to the MMC task to load a specified .h image */
   CMD_ID_LOAD_LAKEMASTER_IMAGE,/* A request to the MMC task to load a specified .h image */
   CMD_ID_EXPORT_SELECTED_NAV_DATA, /* A request to the MMC task to export selected nav data */

   /* t_chart_info messages */
   CMD_ID_CHART_INFO_MENU,     /* Indicate that a Chart Info Menu selection was made */
   CMD_ID_CHART_INFO_REQUEST,  /* A request for Chart Info for a specified position */
   CMD_ID_CHART_WAKE_UP,       /* Indicate that chart rendering is required */
   CMD_ID_PORT_INFO_MENU,

   /* t_nema messages */
   CMD_ID_NMEA_TOGGLE_ECHO,     /* Toggle the setting of the NMEA echo mode */
   CMD_ID_NETWORK_NMEA_MESSAGE,  /* A messages has been redeivced from the network */
   CMD_ID_NMEA2K_MESSAGE,
   CMD_ID_NMEA_GPS_DETECT,
   CMD_ID_NMEA_GPS_RATE,
   CMD_ID_NMEA_GPS_RESET,
   CMD_ID_PROGRAM_EFUSE,
   CMD_ID_COMPASS_HDG_RATE,
   CMD_ID_COMPASS_BAUD_RATE,
   CMD_ID_COMPASS_PNR_RATE,
   CMD_ID_COMPASS_TEST_CMD,
   CMD_ID_COMPASS_MODE_CMD,

   /* t_bathymetry messages */
   CMD_ID_BATHYMETRY_WRITE_PING_DATA,

   /* t_html messages */
   CMD_ID_HTML_LOAD_URL,       /* Load a new URL into the indicated browser */

   /* t_gateway messages */
   CMD_ID_GATEWAY_DATA,        /* Post/Receive new data to/from Gateway task */

   /* t_nav messages */
   CMD_ID_NAV_BULK_TRANSFER,   /* Post Bulk Transfer to Nav task */
   CMD_ID_NAV_IPILOT,          /* Commands for IPILOT navigation */
   CMD_ID_NAV_WIPE_FLASH,      /* Command to delete TRW and reboot*/

   /* t_sound messages */
   CMD_ID_REGISTER_SOUND,      /* Instruct the sound task to register a sound */
   CMD_ID_PLAY_SOUND,          /* Instruct the sound task to play a sound */

   /* t_ethernet messages */
   CMD_ID_ETHERNET_EVENT,      /* An event from an ethernet stack callback */
   CMD_ID_SAVE_IP_SETUP,       /* Tells the ethernet task that the IP address was modified */

   /* t_chart_3d messages */
   CMD_ID_FREEZE_3D_LAYER,     /* Instructs the chart 3D task to freeze frame */

   /* t_xmweather messages */
   CMD_ID_XMWEATHER_INFO_MENU,              /* Info Menu Request */
   CMD_ID_XMWEATHER_INFO_MENU_RESPONSE,
   CMD_ID_XMWEATHER_PLACE_DATA,             /* Send Selected Place Data from ACc to Data task */
   CMD_ID_XMWEATHER_INFO_REQUEST,           /* A request for a specific location from the INFO key */
   CMD_ID_XMWEATHER_INFO_RESPONSE,
   CMD_ID_XMWEATHER_OVERLAY_NEW_DATA,         /* Data->Weather task to notify of new data arrival */
   CMD_ID_XMWEATHER_OVERLAY_NEW_DATA_REQUEST, /* Request data from data task after notified that new data has arrived */
   CMD_ID_XMWEATHER_OVERLAY_DATA_REQUEST,   /* Request data from the WeatherACC task */
   CMD_ID_XMALERT_MSG,                      /* from Data task - XM Device Alert message */
   CMD_ID_XMWEATHER_WARNING_ALERT,          /* Warning and Alert message */
   CMD_ID_XMWEATHER_DATA_REQUEST,           /* Request data from the WeatherData task */
   CMD_ID_XMWEATHER_OVERLAY_TILE_RESPONSE,  /* Data from Data to weather task */
   CMD_ID_XMWEATHER_DIAGNOSTIC_DEBUG,       /* Toggle XM CBM Debug Mode 0-Off/1-On */
   CMD_ID_XMWEATHER_DIAGNOSTIC_CLEAR_NVM,    /* Clear XM Chipset NVM */

   /* t_xducerid messages */
   CMD_ID_XID_READ_VERSION,          /* request version information */
   CMD_ID_XID_READ_BLOCK,            /* request transducer information */
   CMD_ID_XID_READ_TEMP,             /* request temperature information */

   /* t_gmsg messages */
   CMD_ID_GMSG_REGISTER,            /* Register a Task's Mailbox for GMSG services */
   CMD_ID_GMSG_UNREGISTER,          /* Unregister a Task's Mailbox for GMSG services */
   CMD_ID_GMSG_NEW_MESSAGE,         /* A message is available on a socket (internal GMSG) */
   CMD_ID_GMSG_NEW_MESSAGE_EXT,     /* A message is available from an external source */
   CMD_ID_GMSG_RECEIVE,             /* In incoming GMSG message (Sent to a Task's Mailbox) */
   CMD_ID_GMSG_SEND,                /* Send a GMSG message to a target address/port */
   CMD_ID_GMSG_DISCONNECT,          /* Inform Task If a Socket has disconnected */

   CMD_ID_NETWORK_SERVER_CONNECT_MESSAGE,             /* Server connected */
   CMD_ID_NETWORK_SERVER_CONNECTION_LOST_MESSAGE,     /* Server connection was lost */
   CMD_ID_NETWORK_REGISTER_4DATA,   /* Register with network peer for data. */
   CMD_ID_ALARM_ADD_PEER,            /* Add (Remove) network alarm peer */
   CMD_ID_SERVER_MONITOR_TIMEOUT,    /* The server monitor timed out */
   CMD_ID_NETWORK_SERVER_CONNECTION_SUCCESS_MESSAGE,  /* success after a 'lost' message */
   CMD_ID_CLIENT_MONITOR_TIMEOUT,    /* a unit on the network has stopped responding */

   CMD_ID_SUDP_OUT_SEND,                 /* Simple UDP Out send message */
   CMD_ID_SUDP_IN_REGISTER,
   CMD_ID_SUDP_IN_RECEIVE,
   CMD_ID_SUDP_IN_MULTICAST_RESET,

   /* t_radar messages */
   CMD_ID_RADAR_DATA,
   CMD_ID_RADAR_DATA_CHIRP,
   CMD_ID_SECTOR_BLANK_DATA,
   CMD_ID_RADAR_SETTING_CHANGE,
   CMD_ID_SEND_PING_DESCRIPTOR,
   CMD_ID_SET_VIEW,

   CMD_ID_IPILOT,                // Message to iPilot task

   // Talon networking Messages
   CMD_ID_TALON_NETWORKING_SETTING_CHANGE,
   CMD_ID_TALON_NETWORKING_DEVICE_INFO,
   CMD_ID_TALON_NETWORKING_FORGET_DEVICE,

   CMD_ID_IPILOT_SCREENSHOT,
} CMD_ID;

typedef struct
{
   MSGHEADER msghdr;    /* Do not touch this! It's for RTXC use only! */
   CMD_ID    Cmd;       /* All messages will contain and ID so that you know how to interpret the rest */
} GENERIC_MESSAGE;

typedef struct
{
   GENERIC_MESSAGE   msg;
} MSG_SHUTDOWN;

typedef struct tag_MSG_WIPE_FLASH
{
   GENERIC_MESSAGE   msg;
} MSG_WIPE_FLASH;

typedef struct
{
   GENERIC_MESSAGE   msg;
} MSG_POS_RESET;

// The following enum should be used to set mail message priorities.  NORMAL_PRIORITY
// should be used in all but extreme cases.
typedef enum
{
   EXTREMELY_HIGH_PRIORITY = 1,
   HIGH_PRIORITY,
   NORMAL_PRIORITY,                          // Use Me!
   DATABASE_PRIORITY = NORMAL_PRIORITY,
   SHUTDOWN_PRIORITY = NORMAL_PRIORITY,
   LOW_PRIORITY,
   EXTREMELY_LOW_PRIORITY
} MESSAGE_PRIORITY;
/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/

#endif      // #ifndef MESSAGE_H
