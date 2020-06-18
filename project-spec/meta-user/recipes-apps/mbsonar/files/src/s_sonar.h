/*******************************************************************************
*           Copyright (c) 2000 - 2007  Techsonic Industries, Inc.              *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: sonar header file

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
#ifndef _S_SONAR_H
#define _S_SONAR_H

/********************           INCLUDE FILES                ******************/
#include "types.h"
//#include "model.h"
#include "service.h"

/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
//#define MATCHED_FILTER
//#define MEDIAN_FILTER /* matched filter modifier - if set uses median filter - otherwise matched */

#define SONAR_DEBUG_TOOLS            0  // (1|0) (enables|disables) sonar debug menu

#define READ_SONAR_SETTINGS_FROM_MMC 0

#define HISTORY_LENGTH (SCRN_XSIZE + 20) /* Make room for a few extra columns */

#define MAX_ZOOM_LEVEL 8 /* Defined here as same value is required in more than one place */

// speed of sound constants: 1500 m/s salt, 1463 m/s (4800 ft/s) fresh 
// updated per Dave Betts 07/06/2007
#define SOS_FRESH_WATER 1463
#define SOS_SALT_WATER  1500

// #define SOS_FRESH_WATER 1435 /* Fresh water Speed Of Sound (Metres per second) */

typedef enum
{
   WATER_FRESH         = 0,
   WATER_SALT_DEEP     = 1,  // old WATER_SALT
   WATER_SALT_SHALLOW  = 2
} WATER_TYPE;

typedef enum
{
   BEAM_NARROW         = 0,
   BEAM_MEDIUM         = 1,
   BEAM_WIDE           = 2
} BEAM_WIDTH;

typedef enum
{
   TVG_TYPE1 = 0,   //Gen1
   TVG_TYPE2,       //Gen2
   TVG_TYPE_HYBRID  //Gen2 -10 to -1, Gen1 0 to 20
} TVG_TYPE;
#define IS_WATER_FRESH(x)   ((x) == WATER_FRESH) // TRUE = FRESH water

#define BASIC_SAMPLE_PERIOD 26 /* microseconds per sample: 26 = 0.75" resolution */

#define MIN_MANUAL_SIDE_FEET_SETTING      6.0f
#define MIN_MANUAL_SIDE_METRES_SETTING    2.0f
#define MIN_MANUAL_SIDE_FATHOMS_SETTING   1.0f

#define MIN_MANUAL_FEET_SETTING      2.0f
#define MIN_MANUAL_METRES_SETTING    1.0f
#define MIN_MANUAL_FATHOMS_SETTING   1.0f

#define MIN_MANUAL_FEET_IN_METERS    dConvertToInternal(MIN_MANUAL_FEET_SETTING, D_DEPTH, U_FEET)

/* Periods for selected frequencies.  */
#define PERIOD_50KHZ_MICROSECONDS  20.0f
#define PERIOD_83KHZ_MICROSECONDS  12.0f
#define PERIOD_200KHZ_MICROSECONDS  5.0f
#define PERIOD_455KHZ_MICROSECONDS  2.2f

#define BANDWIDTH_MUTE     0x00
#define BANDWIDTH_NARROW   0x01
#define BANDWIDTH_MEDIUM   0x03
#define BANDWIDTH_WIDE     0x02
#define BANDWIDTH_AUTO     0x00 /* retains compatability with previous units that may SSBW set to 0 */

#define BANDWIDTH_DEFAULT_83       BANDWIDTH_NARROW // change from MEDIUM per D.Betts 11/25/2008    
#define BANDWIDTH_DEFAULT_50       BANDWIDTH_MEDIUM    
#define BANDWIDTH_DEFAULT_200      BANDWIDTH_MEDIUM    
#define BANDWIDTH_DEFAULT_SI       BANDWIDTH_WIDE    
#define BANDWIDTH_DEFAULT_QUAD     BANDWIDTH_MEDIUM    
#define BANDWIDTH_DEFAULT_3D       BANDWIDTH_WIDE  // (change from MEDIUM per D.Betts 2/27/07)  

#define BANDWIDTH_DEFAULT_DI_455   BANDWIDTH_NARROW
#define BANDWIDTH_DEFAULT_DI_800   BANDWIDTH_MEDIUM

#define BANDWIDTH_83_ICE_MODE      BANDWIDTH_WIDE // (change from DEFAULT per D.Betts 5/24/10)

#define BANDWIDTH_50_DEEP_MODE     BANDWIDTH_WIDE // (change from DEFAULT per D.Betts 5/24/10)

#define CHART_SPEED_MANUAL 0
#define CHART_SPEED_AUTO   1

/* These values can be used as indexes - so start at zero! */
/* The order of the beams is also critical for blending of sonar data and fish lists */
/* Wider beams should be listed first. */
#define BEAM_LF            0
#define BEAM_200KHZ        1
#define BEAM_455KHZ_L      2
#define BEAM_455KHZ_R      3
#define BEAM_TRUE_DI       4
#define NUMBER_OF_BEAMS    5

/* LF/200 and composite DI, these are just for local presentation
   and can vary values based on new beam addition over time */
#define NUMBER_OF_PSEUDO_BEAMS 2
#define BEAM_LF_200_BLENDED      (NUMBER_OF_BEAMS + 0)
#define BEAM_DOWN_IMAGING        (NUMBER_OF_BEAMS + 1)

/* NETWORK translated beam for limiting old untis from receiving MegaImaging frequency */
#define NETWORK_TRANSLATED_BEAM_START 127
#define MEGAIMAGING_NET_DI_BEAM           (NETWORK_TRANSLATED_BEAM_START+BEAM_TRUE_DI)
#define MEGAIMAGING_NET_SI_LEFT_BEAM      (NETWORK_TRANSLATED_BEAM_START+BEAM_455KHZ_L)
#define MEGAIMAGING_NET_SI_RIGHT_BEAM     (NETWORK_TRANSLATED_BEAM_START+BEAM_455KHZ_R)

/* 2D chirp type */
#define SONAR_NO_CHIRP        10
#define SONAR_LOW_CHIRP       11
#define SONAR_MED_CHIRP       12
#define SONAR_HIGH_CHIRP      13
#define SONAR_LOW_MED_CHIRP   14
#define SONAR_LOW_HIGH_CHIRP  15
#define SONAR_MED_HIGH_CHIRP  16
#define SONAR_CHIRP_455KHZ    17
#define SONAR_CHIRP_MODE_NUM  18

#define SONAR_IMAG_NO_CHIRP   0
#define SONAR_IMAG_262_CHIRP  1
#define SONAR_IMAG_455_CHIRP  2
#define SONAR_IMAG_800_CHIRP  3
#define SONAR_IMAG_1200_CHIRP 4

#define DI_BEAM_CHIRP_SEL_BITMASK              (0x8)
#define SI_BEAM_CHIRP_SEL_BITMASK              (0x1)

#define DI_BEAM_CHIRP_MODE_BITMASK             (0x7)
#define SI_BEAM_CHIRP_MODE_BITMASK             (0xE)

/* sonar multicast ports, (SERVICE_PORT_HB_MULTICAST_BASE+1) is used by radar*/
#define SONAR_2D_MULTICAST_PORT             (SERVICE_PORT_HB_MULTICAST_BASE)    //50880
#define SONAR_SI_DI_MULTICAST_PORT          (SERVICE_PORT_HB_MULTICAST_BASE+2)  //50882
#define SONAR_360_MULTICAST_PORT            (SERVICE_PORT_HB_MULTICAST_BASE+3)  //50883
#define SONAR_CHIRP_2D_MULTICAST_PORT       (SERVICE_PORT_HB_MULTICAST_BASE+4)  //50884
#define SONAR_CHIRP_DI_MULTICAST_PORT       (SERVICE_PORT_HB_MULTICAST_BASE+5)  //50885
#define SONAR_CHIRP_SI_MULTICAST_PORT       (SERVICE_PORT_HB_MULTICAST_BASE+6)  //50886
#define SONAR_MEGAIMAGING_DI_MULTICAST_PORT (SERVICE_PORT_HB_MULTICAST_BASE+7)  //50887
#define SONAR_MB_MULTICAST_PORT             (SERVICE_PORT_HB_MULTICAST_BASE + SONAR_MB_MULTICAST_GROUP_OCTET)  //50890

/* multicast group octet */
/**
 *            Source Type   Multicast IP
 *            -----------   ---------------
 *            Sonar 2D      239.0.ccc.ddd
 *            Radar         239.1.ccc.ddd
 *            Sonar DI      239.2.ccc.ddd
 *            Sonar SI      239.2.ccc.ddd // 3 is unused, initially used for SI
 *            Sonar 360     239.4.ccc.ddd
 *            CHIRP 2D      239.5.ccc.ddd
 *            CHIRP DI      239.6.ccc.ddd
 *            CHIRP SI      239.6.ccc.ddd
*/ 
#define SONAR_2D_MULTICAST_GROUP_OCTET             0
#define SONAR_SI_DI_MULTICAST_GROUP_OCTET          2
#define SONAR_MEGAIMAGING_DI_MULTICAST_GROUP_OCTET 8
#define SONAR_360_MULTICAST_GROUP_OCTET            4
#define SONAR_CHIRP_2D_MULTICAST_GROUP_OCTET       5
#define SONAR_CHIRP_DI_MULTICAST_GROUP_OCTET       6
#define SONAR_CHIRP_SI_MULTICAST_GROUP_OCTET       7
#define SONAR_MB_MULTICAST_GROUP_OCTET             10

typedef struct
{
   U32     u32Imaging262kHzLowLimit;
   U32     u32Imaging262kHzUpperLimit;
   U32     u32Imaging455kHzLowLimit;
   U32     u32Imaging455kHzUpperLimit;
   U32     u32Imaging800kHzLowLimit;
   U32     u32Imaging800kHzUpperLimit;
   U32     u32Imaging1200kHzLowLimit;
   U32     u32Imaging1200kHzUpperLimit;
   BOOLEAN bLocalChirpOn;
   BOOLEAN bSourceChirpOn;
} ImagingInfo;

typedef struct
{
   U32         u32ChirpType;
   U32         u32LowFreqLowLimit;
   U32         u32LowFreqUpperLimit;
   U32         u32MedFreqLowLimit;
   U32         u32MedFreqUpperLimit;
   U32         u32HighFreqLowLimit;
   U32         u32HighFreqUpperLimit;
   U32         u32High455FreqLowLimit;
   U32         u32High455FreqUpperLimit;
   U32         u32ChirpGain;
   BOOLEAN     bLocalChirpOn;
   BOOLEAN     bSourceChirpOn;
   ImagingInfo ImagingDI;
   ImagingInfo ImagingSI;
   ImagingInfo Imaging360;
} ChirpInfo;

/* chirp limit */
#define SONAR_CHIRP_LOW_FREQ_LOW_LIMIT     40
#define SONAR_CHIRP_LOW_FREQ_UPPER_LIMIT   60
#define SONAR_CHIRP_MED_FREQ_LOW_LIMIT     75
#define SONAR_CHIRP_MED_FREQ_UPPER_LIMIT   95

#define SONAR_CHIRP_HIGH_FREQ_200_50_XDUCER_LOW_LIMIT   180
#define SONAR_CHIRP_HIGH_FREQ_200_50_XDUCER_UPPER_LIMIT 220
#define SONAR_CHIRP_HIGH_FREQ_200_83_XDUCER_LOW_LIMIT   175
#define SONAR_CHIRP_HIGH_FREQ_200_83_XDUCER_UPPER_LIMIT 225
#define SONAR_CHIRP_HIGH_FREQ_200_455_XDUCER_LOW_LIMIT   185
#define SONAR_CHIRP_HIGH_FREQ_200_455_XDUCER_UPPER_LIMIT 225

#define SONAR_CHIRP_ICE_XDUCER_15_DEGREE_LOW_LIMIT         170
#define SONAR_CHIRP_ICE_XDUCER_15_DEGREE_UPPER_LIMIT       250
#define SONAR_CHIRP_ICE_XDUCER_15_DEGREE_LOW_DEFAULT       180
#define SONAR_CHIRP_ICE_XDUCER_15_DEGREE_UPPER_DEFAULT     240
#define SONAR_CHIRP_ICE_XDUCER_21_DEGREE_LOW_LIMIT         130
#define SONAR_CHIRP_ICE_XDUCER_21_DEGREE_UPPER_LIMIT       210
#define SONAR_CHIRP_ICE_XDUCER_21_DEGREE_LOW_DEFAULT       140
#define SONAR_CHIRP_ICE_XDUCER_21_DEGREE_UPPER_DEFAULT     200
#define SONAR_CHIRP_ICE_XDUCER_FULL_SPECTRUM_LOW_LIMIT     140
#define SONAR_CHIRP_ICE_XDUCER_FULL_SPECTRUM_UPPER_LIMIT   240
#define SONAR_CHIRP_ICE_XDUCER_FULL_SPECTRUM_LOW_DEFAULT   150
#define SONAR_CHIRP_ICE_XDUCER_FULL_SPECTRUM_UPPER_DEFAULT 220

#define SONAR_CONE_ANGLE_15_DEGREE_NOMINAL_FREQUENCY       210
#define SONAR_CONE_ANGLE_21_DEGREE_NOMINAL_FREQUENCY       170
#define SONAR_CONE_ANGLE_15_21_DEGREE_NOMINAL_FREQUENCY    210

#define SONAR_CHIRP_ICE_XDUCER_IR1_LOW_LIMIT   210
#define SONAR_CHIRP_ICE_XDUCER_IR1_UPPER_LIMIT 240
#define SONAR_CHIRP_ICE_XDUCER_IR2_LOW_LIMIT   135
#define SONAR_CHIRP_ICE_XDUCER_IR2_UPPER_LIMIT 175
#define SONAR_CHIRP_ICE_XDUCER_IR3_LOW_LIMIT   220
#define SONAR_CHIRP_ICE_XDUCER_IR3_UPPER_LIMIT 250
#define SONAR_CHIRP_ICE_XDUCER_IR4_LOW_LIMIT   140
#define SONAR_CHIRP_ICE_XDUCER_IR4_UPPER_LIMIT 180
#define SONAR_CHIRP_ICE_XDUCER_IR5_LOW_LIMIT   215
#define SONAR_CHIRP_ICE_XDUCER_IR5_UPPER_LIMIT 245
#define SONAR_CHIRP_ICE_XDUCER_IR6_LOW_LIMIT   130
#define SONAR_CHIRP_ICE_XDUCER_IR6_UPPER_LIMIT 170

#define SONAR_CHIRP_HIGH_455_FREQ_LOW_LIMIT   440
#define SONAR_CHIRP_HIGH_455_FREQ_UPPER_LIMIT 490

#define GENERIC_CHIRP_LOW_FREQ_LOW_LIMIT     28
#define GENERIC_CHIRP_LOW_FREQ_UPPER_LIMIT   75
#define GENERIC_CHIRP_MED_FREQ_LOW_LIMIT     75
#define GENERIC_CHIRP_MED_FREQ_UPPER_LIMIT  155
#define GENERIC_CHIRP_HIGH_FREQ_LOW_LIMIT   130
#define GENERIC_CHIRP_HIGH_FREQ_UPPER_LIMIT 250
#define GENERIC_CHIRP_HIGH_455_FREQ_LOW_LIMIT   410
#define GENERIC_CHIRP_HIGH_455_FREQ_UPPER_LIMIT 500

#define SONAR_CHIRP_IMAG_262KHZ_LOW_LIMIT     210
#define SONAR_CHIRP_IMAG_262KHZ_UPPER_LIMIT   300
#define SONAR_CHIRP_IMAG_455KHZ_LOW_LIMIT     405 //420
#define SONAR_CHIRP_IMAG_455KHZ_UPPER_LIMIT   535 //520
#define SONAR_CHIRP_IMAG_800KHZ_LOW_LIMIT     780 //790
#define SONAR_CHIRP_IMAG_800KHZ_UPPER_LIMIT   860 //850
#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT    1000
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT  1300

#define SONAR_CHIRP_IMAG_455KHZ_OLD_LOW       420
#define SONAR_CHIRP_IMAG_455KHZ_OLD_UPPER     520
#define SONAR_CHIRP_IMAG_800KHZ_OLD_LOW       790
#define SONAR_CHIRP_IMAG_800KHZ_OLD_UPPER     850

#define SONAR_CHIRP_IMAG_455KHZ_LOW_LIMIT_SUPER     400
#define SONAR_CHIRP_IMAG_455KHZ_UPPER_LIMIT_SUPER   600
#define SONAR_CHIRP_IMAG_800KHZ_LOW_LIMIT_SUPER     700
#define SONAR_CHIRP_IMAG_800KHZ_UPPER_LIMIT_SUPER   900
#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT_SUPER    800
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT_SUPER  2000

#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT_DEFAULT    1150
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT_DEFAULT  1275

#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT_XR_SI_DEFAULT    1050
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT_XR_SI_DEFAULT  1175

#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT_XR_DI_DEFAULT    1100
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT_XR_DI_DEFAULT  1200

#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT_XD_DEFAULT    1100
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT_XD_DEFAULT  1200

#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT_DI_75_DEFAULT    1075  
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT_DI_75_DEFAULT  1150 //1175

#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT_SI_150_DEFAULT    1075  
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT_SI_150_DEFAULT  1150 //1175

#define SONAR_CHIRP_IMAG_455KHZ_LOW_LIMIT_SI_DI_COMPACT_DEFAULT    440
#define SONAR_CHIRP_IMAG_455KHZ_HIGH_LIMIT_SI_DI_COMPACT_DEFAULT   500

#define SONAR_CHIRP_IMAG_455KHZ_LOW_LIMIT_XR_SI_DEFAULT    405
#define SONAR_CHIRP_IMAG_455KHZ_HIGH_LIMIT_XR_SI_DEFAULT   505

#define SONAR_CHIRP_IMAG_455KHZ_LOW_LIMIT_XR_DI_DEFAULT    435
#define SONAR_CHIRP_IMAG_455KHZ_HIGH_LIMIT_XR_DI_DEFAULT   535

#define SONAR_CHIRP_IMAG_800KHZ_LOW_LIMIT_XR_SI_DEFAULT    780
#define SONAR_CHIRP_IMAG_800KHZ_HIGH_LIMIT_XR_SI_DEFAULT   840

#define SONAR_CHIRP_IMAG_800KHZ_LOW_LIMIT_XR_DI_DEFAULT    800
#define SONAR_CHIRP_IMAG_800KHZ_HIGH_LIMIT_XR_DI_DEFAULT   860

#define SONAR_CHIRP_IMAG_455KHZ_LOW_LIMIT_INC_RANGE2_DI_DEFAULT 420
#define SONAR_CHIRP_IMAG_455KHZ_UPPER_LIMIT_INC_RANGE2_DI_DEFAULT 520

#define SONAR_CHIRP_IMAG_455KHZ_LOW_LIMIT_INC_RANGE2_SI_DEFAULT 400
#define SONAR_CHIRP_IMAG_455KHZ_UPPER_LIMIT_INC_RANGE2_SI_DEFAULT 500

#define SONAR_CHIRP_IMAG_800KHZ_LOW_LIMIT_INC_RANGE2_DI_DEFAULT 790
#define SONAR_CHIRP_IMAG_800KHZ_UPPER_LIMIT_INC_RANGE2_DI_DEFAULT 850

#define SONAR_CHIRP_IMAG_800KHZ_LOW_LIMIT_INC_RANGE2_SI_DEFAULT 790
#define SONAR_CHIRP_IMAG_800KHZ_UPPER_LIMIT_INC_RANGE2_SI_DEFAULT 850

#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT_INC_RANGE2_DI_DEFAULT 1075
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT_INC_RANGE2_DI_DEFAULT 1175

#define SONAR_CHIRP_IMAG_1200KHZ_LOW_LIMIT_INC_RANGE2_SI_DEFAULT 1025
#define SONAR_CHIRP_IMAG_1200KHZ_UPPER_LIMIT_INC_RANGE2_SI_DEFAULT 1125



#if MODEL_HAS_3D
   #define BEAM_3D_BASE (NUMBER_OF_BEAMS + NUMBER_OF_PSEUDO_BEAMS)
   #define BEAM_3D_1            (BEAM_3D_BASE + 0)
   #define BEAM_3D_PSEUDO_12    (BEAM_3D_BASE + 1)
   #define BEAM_3D_2            (BEAM_3D_BASE + 2)
   #define BEAM_3D_PSEUDO_23    (BEAM_3D_BASE + 3)
   #define BEAM_3D_3            (BEAM_3D_BASE + 4)
   #define BEAM_3D_PSEUDO_34    (BEAM_3D_BASE + 5)
   #define BEAM_3D_4            (BEAM_3D_BASE + 6)
   #define BEAM_3D_PSEUDO_45    (BEAM_3D_BASE + 7)
   #define BEAM_3D_5            (BEAM_3D_BASE + 8)
   #define BEAM_3D_PSEUDO_56    (BEAM_3D_BASE + 9)
   #define BEAM_3D_6            (BEAM_3D_BASE + 10)
   #define BEAM_3D_COMPOSITE_34 (BEAM_3D_BASE + 11)
#endif // MODEL_HAS_3D

// default TVG level for TVG_OFF display setting (not really off, just mostly off)
#define DEFAULT_LF_TVG_OFF_SETTING  (3)   // 3 = 30.0% of nominal TVG curve
#define DEFAULT_HF_TVG_OFF_SETTING  (0)   // no TVG curve
#define DEFAULT_IMAGING_TVG_OFF_SETTING  (3)   // no TVG curve

#define MEASURE_NOISE_FLAG  128  /* Bitwise OR any of the above beams to indicate a noise measurement */
#define MEASURE_NOISE_MASK  127  /* Bitwise AND to get the actual beam */

#define INVALID_BEAM         255
#define INVALID_TEMPERATURE -300.0f;

#define ZOOM_NARROW    (4*(SCRN_XSIZE-ACTIVE_X_ORIGIN)/8)
#define ZOOM_MEDIUM    (3*(SCRN_XSIZE-ACTIVE_X_ORIGIN)/8)
/* On very small screens - if lower depth is manually set - the "M" collides with the zoom lower depth scale - hence the +5 */
#define ZOOM_WIDE      ((SCRN_XSIZE>160) ? (2*(SCRN_XSIZE-ACTIVE_X_ORIGIN)/8) :  5 + (2*(SCRN_XSIZE-ACTIVE_X_ORIGIN)/8)) 

/* SI orientation */
#define SONAR_SI_ORIENTATION_NORMAL 0
#define SONAR_SI_ORIENTATION_REVERSE 1

/* SI offset threshold to show offset */
#define SI_OFFSET_MENU_TO_SHOW_AFTER_RANGE_METERS 16.0f

/* Imaging display freq. */
#define IMAGING_DISPLAY_455KHZ 1
#define IMAGING_DISPLAY_MEGA   2
#define IMAGING_DISPLAY_AUTO 3

/********************               MACROS                   ******************/
#define METRES_TO_MICROSECONDS(x) ((U32)((x)*1000000*2/g_u32SpeedOfSoundInWater))
#define INCHES_TO_MICROSECONDS(x) ((U32)((x)*50800/g_u32SpeedOfSoundInWater)) /* 50800 = 0.0254*1000000*2 */
#define MICROSECONDS_TO_METRES(x) ((float)(x)*g_u32SpeedOfSoundInWater/(2.0f*1000000.0f))

#define FEET_TO_MICROSECONDS(x) ((U32)(INCHES_TO_MICROSECONDS((x)*12)))
#define MICROSECONDS_TO_SAMPLES(x,period) (((x) + (period)/2) / (period))
#define METRES_TO_SAMPLES(x,period) MICROSECONDS_TO_SAMPLES(METRES_TO_MICROSECONDS(x), period)
#define INCHES_TO_SAMPLES(x,period) MICROSECONDS_TO_SAMPLES(INCHES_TO_MICROSECONDS(x), period)
#define FEET_TO_SAMPLES(x,period)   MICROSECONDS_TO_SAMPLES(FEET_TO_MICROSECONDS(x), period)
#define SAMPLES_TO_METRES(x,period) (MICROSECONDS_TO_METRES((x)*(period)))
#define DEFAULT_BOOST_VOLTAGE_SETTING 0xEC
/********************         TYPE DEFINITIONS               ******************/
typedef U8 BEAM;

enum 
{
   SIDESCAN_INVERSE = 1,
   SIDESCAN_GRAY,
   SIDESCAN_GREEN,
   SIDESCAN_BROWN,
   SIDESCAN_BLUE,
   SIDESCAN_AMBER1,
   SIDESCAN_AMBER2,
   SIDESCAN_GREEN_RED,
   SIDESCAN_GREEN_BLUE,
   SIDESCAN_ORANGE_RED_PURPLE,
   SIDESCAN_RED_YELLOW_BLUE,
   SIDESCAN_BLUE_YELLOW,


   /* CUSTOM DEBUG PALETTE */
   SIDESCAN_CUSTOM
};

enum 
{
   DOWNIMAGING_INVERSE = 1,
   DOWNIMAGING_LIGHT_GRAY,
   DOWNIMAGING_DARK_GRAY,
   DOWNIMAGING_NOT_SET
};

enum 
{
   SONAR_2D_PALETTE_1 = 1,       /* Yellow, Red, Blue */
   SONAR_2D_PALETTE_2,           /* Yellow, Green, Blue */
   SONAR_2D_PALETTE_3,           /* Red, Orange, Yellow, Green, Blue */
   SONAR_2D_PALETTE_4,           /* Orginal Spectrum */
   SONAR_2D_GRAY,
   SONAR_2D_GRAY_INVERSE,
   SONAR_2D_GREEN,
   SONAR_2D_PALETTE_1_BLACK,
   SONAR_2D_PALETTE_2_BLACK,
   SONAR_2D_PALETTE_3_BLACK,
   SONAR_2D_PALETTE_4_BLACK,     /*Orginal*/
   SONAR_2D_GRAY_BLACK,
   SONAR_2D_GRAY_INVERSE_BLACK,
   SONAR_2D_GREEN_BLACK,
   SONAR_2D_AMBER1,
   SONAR_2D_AMBER2,
   SONAR_2D_RED_GREEN_YELLOW,
   SONAR_2D_BLUE_GREEN_YELLOW_WHITE,
   SONAR_2D_BLUE_GREEN_YELLOW_BLACK,
   SONAR_ICE_PALETTE_1,          /* ICE MODE ONLY */
   SONAR_ICE_PALETTE_2,          /* ICE MODE ONLY */
   SONAR_ICE_PALETTE_3           /* ICE MODE ONLY */
};
/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/
extern U32 g_u32SpeedOfSoundInWater;
/********************              FUNCTIONS                 ******************/
#endif /* _S_SONAR_H */
