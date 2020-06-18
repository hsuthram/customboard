/*******************************************************************************
*           Copyright (c) 2006  Techsonic Industries, Inc.                     *
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
#ifndef GLOBAL_H
   #define GLOBAL_H
/********************           INCLUDE FILES                ******************/
//#include "cproject.h"
//#include "dd_utils.h"
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#ifndef NULL
   #define  NULL        ((void *)0)
#endif

#define  INVALID_MAILBOX   -1

#define  BITS_PER_BYTE      8

#define EARTH_RADIUS 6370000  //in meters

// This is used in frequency calculations ...
#define  MHz             1000000                // number of Hz in 1 MHz

#define  PI              3.1415926535897932384626
#define  HALF_PI         1.5707963267948966192313
#define  TWO_PI          (2.0 * PI)
       //scaled constants for those routines abhorring floating point
#define  KPI             ((PI)*1024)  

#define  DO_FOREVER      for(;;)

// Unit Conversion Constants
#define  FT2M        0.304800610       // number of meters per foot
#define  M2FT        (1.0/FT2M)        // number of feet per meter

#define  FT2SLICES   4                 // number of slices per foot
#define  M2SLICES    (4.0 * M2FT)      // number of slices per meter
#define  SLICES2M    (1.0/M2SLICES)    // number of meters per slice

#define  MPH2KPH     1.609347220       // number of km in a sm
#define  MPH2KTS     0.868976311       // number of nm in a sm

#define  KTS2MPH     (1.0/MPH2KTS)     // number of sm in a nm
#define  KTS2KPH     (MPH2KPH/MPH2KTS) // number of km in a nm

#define  KPH2MPH     (1.0/MPH2KPH)     // number of sm in a km
#define  KPH2KTS     (MPH2KTS/MPH2KPH) // number of nm in a km

#define M2FATHOM 0.546806649
#define FATHOM2M (1/M2FATHOM)

#define ONEKM2M   0.621    // 1km = 0.539 nautical mile
#define ONEKM2NM   0.539    // 1km = 0.539 mile

#define ONENM2KM   1.852    // 1 nm = 1.852 km
#define ONENM2M   1.151    // 1nm = 1.151 mile

#define ONEM2KM   1.609    // 1mile = 1.609 km
#define ONEM2NM   0.869    // 1km = 0.869 mile

#define ONEKTS2KPH  1.85   // 1 knot = 1.85 km/h
#define ONEMS2KM  3.6   // 1 meter/s = 1.3.6 km/h

#define SECPERYEAR 31556925.9747
#define SECPERDAY 86400
#define CLKTICK 1
#define  THREE_MILLISECONDS (3/CLKTICK)
#define  TEN_MILLISECONDS  (10/CLKTICK)
#define  TENTH_SEC         (100/CLKTICK)
#define  QUARTER_SEC       (250/CLKTICK)
#define  HALF_SEC          (500/CLKTICK)
#define  ONE_SEC           (1000/CLKTICK)
#define  TWO_SEC           (2000/CLKTICK)
#define  THREE_SEC         (3000/CLKTICK)
#define  FIVE_SEC          (5000/CLKTICK)
#define  TEN_SEC           (10000/CLKTICK)
#define  TWENTY_SEC        (20000/CLKTICK)
#define  THIRTY_SEC        (30000/CLKTICK)
#define  ONE_MIN           (60000/CLKTICK)
#define  TWO_MIN           (120000/CLKTICK)
#define  THREE_MIN         (180000/CLKTICK)
#define  FOUR_MIN          (240000/CLKTICK)
#define  FIVE_MIN          (300000/CLKTICK)

#define  BREAKPOINT        __asm{NOP}

#define  U32_FLAG_ERROR          ((U32)-1)
#define  U32_FLAG_END_OF_LIST    ((U32)-1)

#define  MILLI_PER_SECOND        1000
#define  MICRO_PER_SECOND        1000000
#define  NANO_PER_SECOND         1000000000

#define  NANO_PER_MILLI          (NANO_PER_SECOND/MILLI_PER_SECOND)

/********************               MACROS                   ******************/
#define  ROUND(a)          ((int)((a)+0.5))
#define  NINT(a)           (((a) >= 0) ? (int)((a)+0.5) : (int)((a)-0.5))

#define  SIGN(x)           ((x) > 0 ? 1:  ((x) == 0 ? 0:  (-1)))
#define  POS(x)            ((x) < 0 ? 0 : 1)
#define  NEG(x)            ((x) < 0 ? 1 : 0) 
#define  SWAP(a,b)         {(a)^=(b); (b)^=(a); (a)^=(b);}
#define  ABS( x )          (( (x) >= 0 ) ? (x) : -(x))

#define  ARRAY_LENGTH(x)   (sizeof(x)/sizeof(x[0]))

#define  RADIANS(x)        ((x)/(2*PI))
#define  KRADIANS(x)       (RADIANS(x)*1024)
#define  DEGREES2RADIANS(x) ((x)*PI/180.0f)
#define  RADIANS2DEGREES(x) ((x)*180.0f/PI)

#define  FLT_EQUAL(flt1, flt2, eps)  (((((flt1) + (eps)) > (flt2)) && (((flt1) - (eps)) < (flt2))) ? TRUE : FALSE)
#define  FLT_ROUND_IF_ZERO(flt, eps) (FLT_EQUAL(flt, 0.0f, eps) ? 0.0f : (flt))

#if !defined(linux) && defined(__BIG_ENDIAN)
   #define  HOST_TO_LITTLE_ENDIAN_64(x)      (U64)( ( (U64)DD_UTIL_EndianSwap( (U32)(   ( x ) & 0x00000000FFFFFFFFULL         ) ) << 32 ) + \
                                                    ( (U64)DD_UTIL_EndianSwap( (U32)( ( ( x ) & 0xFFFFFFFF00000000ULL ) >> 32 ) )       ) )
   #define  HOST_TO_LITTLE_ENDIAN_32(x)      DD_UTIL_EndianSwap( x )
   #define  HOST_TO_LITTLE_ENDIAN_16(x)      (U16)(DD_UTIL_EndianSwap( x ) >> 16)
   #define  HOST_TO_BIG_ENDIAN_64(x)         (U64)(x)
   #define  HOST_TO_BIG_ENDIAN_32(x)         (x)
   #define  HOST_TO_BIG_ENDIAN_16(x)         (U16)(x)
   #define  LITTLE_ENDIAN_TO_HOST_64(x)      (U64)( ( (U64)DD_UTIL_EndianSwap( (U32)(   ( x ) & 0x00000000FFFFFFFFULL         ) ) << 32 ) + \
                                                    ( (U64)DD_UTIL_EndianSwap( (U32)( ( ( x ) & 0xFFFFFFFF00000000ULL ) >> 32 ) )       ) )
   #define  LITTLE_ENDIAN_TO_HOST_32(x)      DD_UTIL_EndianSwap( x )
   #define  LITTLE_ENDIAN_TO_HOST_16(x)      (U16)(DD_UTIL_EndianSwap( x ) >> 16)
   #define  BIG_ENDIAN_TO_HOST_64(x)         (U64)(x)
   #define  BIG_ENDIAN_TO_HOST_32(x)         (x)
   #define  BIG_ENDIAN_TO_HOST_16(x)         (U16)(x)
   
#else
   #define  HOST_TO_LITTLE_ENDIAN_64(x)      (U64)(x)
   #define  HOST_TO_LITTLE_ENDIAN_32(x)      (x)
   #define  HOST_TO_LITTLE_ENDIAN_16(x)      (U16)(x)
   #define  HOST_TO_BIG_ENDIAN_64(x)         (U64)( ( (U64)DD_UTIL_EndianSwap( (U32)(   ( x ) & 0x00000000FFFFFFFFULL         ) ) << 32 ) + \
                                                    ( (U64)DD_UTIL_EndianSwap( (U32)( ( ( x ) & 0xFFFFFFFF00000000ULL ) >> 32 ) )       ) )
   #define  HOST_TO_BIG_ENDIAN_32(x)         DD_UTIL_EndianSwap( x )
   #define  HOST_TO_BIG_ENDIAN_16(x)         (U16)(DD_UTIL_EndianSwap( x ) >> 16)
   #define  LITTLE_ENDIAN_TO_HOST_64(x)      (U64)(x)
   #define  LITTLE_ENDIAN_TO_HOST_32(x)      (x)
   #define  LITTLE_ENDIAN_TO_HOST_16(x)      (U16)(x)
   #define  BIG_ENDIAN_TO_HOST_64(x)         (U64)( ( (U64)DD_UTIL_EndianSwap( (U32)(   ( x ) & 0x00000000FFFFFFFFULL         ) ) << 32 ) + \
                                                    ( (U64)DD_UTIL_EndianSwap( (U32)( ( ( x ) & 0xFFFFFFFF00000000ULL ) >> 32 ) )       ) )
   #define  BIG_ENDIAN_TO_HOST_32(x)         DD_UTIL_EndianSwap( x )
   #define  BIG_ENDIAN_TO_HOST_16(x)         (U16)(DD_UTIL_EndianSwap( x ) >> 16)
#endif

/********************         TYPE DEFINITIONS               ******************/

/********************        FUNCTION PROTOTYPES             ******************/

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/

/********************              FUNCTIONS                 ******************/



#endif // GLOBAL_H
