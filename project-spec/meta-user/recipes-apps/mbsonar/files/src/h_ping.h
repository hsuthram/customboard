/*******************************************************************************
*           Copyright (c) 2000 - 2006  Techsonic Industries, Inc.              *
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
#ifndef _H_PING_H
   #define _H_PING_H
/********************           INCLUDE FILES                ******************/
#include "message.h"
//#include "model.h"
//#include "Option.h"
//#include "s_fishhistory.h"
#include "s_sonar.h"
//#if MODEL_HAS_NETWORKING
#include "datatype.h"
//#endif
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define QUEUE_ENTRIES 12
//#define DEBUG_DEPTH_ALGORITHM         // enable depth alorithm debugging code

#if IN_HOUSE_FEATURES
// #define DEBUG_DEPTH_ALGORITHM         // enable depth alorithm debugging code
#endif

#define USE_ENERGY_ABOVE_THRESHOLD    // candidate search uses energy above threshold
                                      // instead of total energy for calculations
#define  MAX_BOTTOM_CANDIDATES     10


#if MODEL_HAS_MCASP_SONAR
#define  SIDE_IMAGING_800KHZ_FREQUENCY_IN_KHZ      812
#define  SIDE_IMAGING_455KHZ_FREQUENCY_IN_KHZ      462
#else
#define  SIDE_IMAGING_800KHZ_FREQUENCY_IN_KHZ      800
#define  SIDE_IMAGING_455KHZ_FREQUENCY_IN_KHZ      455
#endif
#define  SIDE_IMAGING_262KHZ_FREQUENCY_IN_KHZ      262

#define  SIDE_IMAGING_1200KHZ_FREQUENCY_IN_KHZ      1210

#define  MAX_200KHZ_RANGE_METERS                   350 // limit 200KHz pings to 300 meters

#define MAX_HFBYTES_T 8192
#define MAX_HFBYTES_S 4096
#define COLS_8_dyn 256
#define ROWS_8_dyn 752

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
//typedef struct
//{
//   BOOLEAN  bPositionValid;
//   S32      s32LongitudeMM;
//   S32      s32LatitudeMM;
//} SONAR_PING_LOCATION;

//typedef struct
//{
//   U32  u32StartIndex;             // target start position index value
//   U32  u32StopIndex;              // target stop position index value
//   U32  u32PeakIndex;              // peak position index
//   U8   u8TVGPeak;                 // a/d reading of max return in bottom (TVG)
//   U8   u8RawPeak;                 // a/d reading of max return in bottom (not TVG)
//   U32  u32ElongationMicroSeconds; // vertical elongation of bottom
//   U64  u64Energy;                 // Sum of return between -12dB points antilogged (may be energy above thresh)
//#ifdef USE_ENERGY_ABOVE_THRESHOLD
//   U64  u64EnergyTotal;            // Total energy of the target peak
//#endif
//} TARGETCANDIDATE;

//#ifdef DEBUG_DEPTH_ALGORITHM
//typedef struct
//{
//   TARGETCANDIDATE  BottomCandidates[MAX_BOTTOM_CANDIDATES];
//   U32              u32BottomCandidates;
//   U32              u32BestBottomCandidate;
//   BOOLEAN          bConstantThreshold;
//   U8               u8BottomThreshold[SCRN_ACTUAL_YSIZE]; // downsampled threshold curve for display on RTS
//   U32              u32SRAChoice;            // indicate this is the second return algorithm choice for bottom
//   U32              u32DepthWindowHalfSize;  // half the depth window size (0 if unused)
//   U32              u32DepthWindowCenter;    // center of the depth window (0 if unused)
//   U64              u64DepthPeakEnergy;
//} DEPTH_CALC_IN_HOUSE;
//#endif

typedef struct
{
   BEAM                 Beam;                            /* Frequency/Beam selection */
   U8                   u8AdcChannel;                    /* 0 to 7 on the Samsung processor */
   U8                   u8TransmitCycles;                /* 0 = no transmit burst, i.e noise measure */
   U8                   u8TransmitPower;                 /* Low, medium or high */
   U32                  u32NumberOfSamples;              /* Number of samples to record */
   U8                  *pu8RawSonarData;                 /* DMA will copy data to this location */
   U8                  *pu8TvgdData;                     /* Filtered TVG data */
   BOOLEAN              bSimulated;                      /* TRUE if this return is simulated */
   U32                  u32SamplePeriodMicroSeconds;     /* Delay between samples in microseconds */
   U16                  u16MilliSecondsBetweenPings;     /* Minimum period between pings of same frequency */
   U16                  u16MilliSecondsBeforeNextPing;   /* Minimum period between this ping and next ping */
//   FISH_TARGET_LIST     SimulatedFishList;               /* Filled in by synthetic sonar generator */
//   SONAR_PING_LOCATION  Location;                        /* Gps location at point of ping */
   float                fDepthMetres;                    /* Measured depth - not necessarily derived from this beam */
   float                fDepthOffsetFeet;                /* Depth offset - only positive value is stored */
   U8                   u8NoiseLevel;
   U8                   u8PeakNoiseLevel;                /* Filled in by the measure noise function */
   U8                   u8Transducer;                    /* The transducer used to acquire this ping */
   U32                  u32TransmitFrequencyHertz;       /* The frequency of the transmission */
   U32                  u32TimeStampMilliSeconds;        /* Taken from the free running timer */
   U8                   u8BandwidthControl;
   U32                  u32PingIndex;                    // ping index number from sonar recording file
   BOOLEAN              bReversePing;                    // ping for reverse scroll sonar display (placed at other end of history)
   U32                  u32PPIPositionInfo;              // composite position info of the ping for PPI display
//#ifdef DEBUG_DEPTH_ALGORITHM
//   DEPTH_CALC_IN_HOUSE  DepthParms;                      // debug data for depth calculation
//#endif
//#if MODEL_HAS_NETWORKING
   BOOLEAN              bIsLocal;                        // TRUE if the ping is from a local source.  FALSE if from a network source
//#endif //MODEL_HAS_NETWORKING
} PING_DESCRIPTOR;

typedef struct
{
   BEAM Beam;                            /* Frequency/Beam selection */
   S32  s32nhf_t;            /* no clue - ask Per */
   S32  s32nhf_s;            /* no clue - ask Per */  
   U32  u32Seed;             /* used to decode the data */
   U32  u32NumberOfSamples;
   U32  u32PingNumber;
   U8  *pu8RawSonarData;    /* DMA will copy data to this location */
} MB_PING_DESCRIPTOR;

typedef struct
{
   GENERIC_MESSAGE      Header;     /* This will always contain an element called Cmd */
   PING_DESCRIPTOR     *pReturnDescriptor;
//   MB_PING_DESCRIPTOR  *pReturnDescriptor;
} PING_MESSAGE;

//typedef struct
//{
//   GENERIC_MESSAGE      Header;     /* This will always contain an element called Cmd */
//   PING_DESCRIPTOR     *apReturnDescriptor[3];
//#if MODEL_HAS_SONAR_RECORD
//   BOOLEAN              bBlankPing;
//#endif
//} TRIPLE_PING_MESSAGE;

/********************        FUNCTION PROTOTYPES             ******************/
//extern U32        H_PING_u32Generate(PING_DESCRIPTOR *pPing);
//extern void       H_PING_vInitialize(void);
//extern BOOLEAN    H_PING_bSimulate(BOOLEAN bState);
//extern U32        H_PING_u32GetPingCount(const BEAM Beam);
//extern void       H_PING_vSetPingDetectMode(BOOLEAN bDetectMode);
/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/
//#if MODEL_HAS_NETWORKING
   DT_EXTERN_TYPE(DT_BEAM);
   DT_EXTERN_TYPE(DT_RAW_SONAR_DATA);
   DT_EXTERN_TYPE(DT_EXCLUDE_TVGD);
   DT_EXTERN_TYPE(DT_EXLUDE_FISH_TARGET_LIST);
   DT_EXTERN_TYPE(DT_SONAR_PING_LOCATION);
   DT_EXTERN_TYPE(DT_PING_DESCRIPTOR);
   DT_EXTERN_TYPE(DT_MB_PING_DESCRIPTOR);
//#endif
/********************              FUNCTIONS                 ******************/

#endif /* _H_PING_H */
