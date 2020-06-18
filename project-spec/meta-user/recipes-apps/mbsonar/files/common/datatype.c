/*******************************************************************************
*           Copyright (c) 2008  Techsonic Industries, Inc.                     *
*         - Contains CONFIDENTIAL and TRADE SECRET information -               *
********************************************************************************

           Description: Data type manipulation functions including serialization
                        and deserialization. Data types defined with these
                        facilities can be sent in messages and stored in the
                        database.

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
#else
   #include "global.h"
   #include "types.h"
#endif
#include "datatype.h"
#include "error.h"
#ifndef __RTXC__
   #include "gxml.h"
   #include <stdlib.h>
#endif
#include <string.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
//#if MODEL_HAS_NETWORKING
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/

/********************               MACROS                   ******************/

/********************         TYPE DEFINITIONS               ******************/
#define XML_ATTRIBUTE_VERSION       0

/********************        FUNCTION PROTOTYPES             ******************/
PRIVATE const G_DATA_TYPE * DATATYPE_pFindVersion(const G_DATA_TYPE *pDataType, int nVersion);

/********************          LOCAL VARIABLES               ******************/


//Table mapping data units to data dimensions (i.e. distance, speed, time, etc.)
//Used to do sanity check on unit conversion compatibility
const G_DATA_DIMENSION s_DataDimensionTable[G_DATA_UNIT_COUNT] =
{
   G_DD_SCALAR,                    //G_DU_NONE

   //distance/depth/altitude
   G_DD_DISTANCE_DEPTH_ALTITUDE,   //G_DU_METERS,
   G_DD_DISTANCE_DEPTH_ALTITUDE,   //G_DU_STAT_MILES
   G_DD_DISTANCE_DEPTH_ALTITUDE,   //G_DU_NAUT_MILES
   G_DD_DISTANCE_DEPTH_ALTITUDE,   //G_DU_KM,
   G_DD_DISTANCE_DEPTH_ALTITUDE,   //G_DU_INCH,
   G_DD_DISTANCE_DEPTH_ALTITUDE,   //G_DU_FEET,
   G_DD_DISTANCE_DEPTH_ALTITUDE,   //G_DU_FATHOMS,

   //speed
   G_DD_SPEED,                     //G_DU_MPS,
   G_DD_SPEED,                     //G_DU_KNOTS,
   G_DD_SPEED,                     //G_DU_MPH,
   G_DD_SPEED,                     //G_DU_KPH,

   //time
   G_DD_TIME,                      //G_DU_SECONDS,
   G_DD_TIME,                      //G_DU_MINUTES,
   G_DD_TIME,                      //G_DU_HOURS,
   G_DD_TIME,                      //G_DU_DAYS,

   //pressure
   G_DD_PRESSURE,                  //G_DU_HECTOPASCALS,
   G_DD_PRESSURE,                  //G_DU_MILLIBAR,
   G_DD_PRESSURE,                  //G_DU_INCHESHG,
   G_DD_PRESSURE,                  //G_DU_MMHG,

   //volume
   G_DD_VOLUME,                    //G_DU_M3,
   G_DD_VOLUME,                    //G_DU_LITERS,
   G_DD_VOLUME,                    //G_DU_USGALLONS,
   G_DD_VOLUME,                    //G_DU_UKGALLONS,

   //electric potential
   G_DD_POTENTIAL,                 //G_DU_VOLTS,

   //electric current
   G_DD_CURRENT,                   //G_DU_AMPERES,
   G_DD_CURRENT,                   //G_DU_MILLIAMPERES,

   //temperature
   G_DD_TEMPERATURE,               //G_DU_CELSIUS,
   G_DD_TEMPERATURE,               //G_DU_FAHRENHEIT,

   //heading
   G_DD_HEADING,                   //G_DU_DEG_TRUE,

   //rate of turn
   G_DD_RATEOFTURN,                //G_DU_DEG_PER_MINUTE,

   //latitude
   G_DD_LATITUDE,                  //G_DU_DEG_LATITUDE,

   //longitude
   G_DD_LONGITUDE,                 //G_DU_DEG_LONGITUDE,
};

PRIVATE const DATA_UNIT_CONVERSION_INFO s_DataConversionTable[G_DATA_UNIT_COUNT] =
{
   {1.0f,            0.0f},   //G_DU_NONE

   //distance/depth/altitude
   {1.0f,            0.0f},               //G_DU_METERS
   {1609.34721869f,  0.0f},               //G_DU_STAT_MILES,
   {1852.0f,         0.0f},               //G_DU_NAUT_MILES,
   {1000.0f,         0.0f},               //G_DU_KM,
   {0.0254f,         0.0f},               //G_DU_INCH,
   {0.3048f,         0.0f},               //G_DU_FEET
   {1.82880365761f,  0.0f},               //G_DU_FATHOMS

   //speed
   {1.0f,            0.0f},               //G_DU_MPS,
   {0.5144456f,      0.0f},               //G_DU_KNOTS,
   {0.44704f,        0.0f},               //G_DU_MPH,
   {1000.0f/3600.0f, 0.0f},               //G_DU_KPH,

   //time
   {1.0f,            0.0f},               //G_DU_SECONDS,
   {60.0f,           0.0f},               //G_DU_MINUTES,
   {3600.0f,         0.0f},               //G_DU_HOURS,
   {86400.0f,        0.0f},               //G_DU_DAYS,

   //pressure
   {1.0f,            0.0f},               //G_DU_HECTOPASCALS,
   {1.0f,            0.0f},               //G_DU_MILLIBAR,
   {33.8639f,        0.0f},               //G_DU_INCHESHG,
   {1.33322f,        0.0f},               //G_DU_MMHG,

   //volume
   {1.0f,            0.0f},               //G_DU_M3,
   {0.001f,          0.0f},               //G_DU_LITERS,
   {0.003785411784f, 0.0f},               //G_DU_USGALLONS,
   {0.004546092f,    0.0f},               //G_DU_UKGALLONS,

   //electric potential
   {1.0f,            0.0f},               //G_DU_VOLTS,

   //electric  current
   {1.0f,            0.0f},               //G_DU_AMPERES,
   {0.001f,          0.0f},               //G_DU_MILLIAMPERES,

   //temperature
   {1.0f,            0.0f},               //G_DU_CELSIUS,
   {5.0f/9.0f,       -32.0f*5.0f/9.0f},   //G_DU_FAHRENHEIT,

   //heading
   {1.0f,            0.0f},               //G_DU_DEG_TRUE,

   //rate of turn
   {1.0f,            0.0f},               //G_DU_DEG_PER_MINUTE,

   //latitude
   {1.0f,            0.0f},               //G_DU_DEG_LATITUDE

   //longitude
   {1.0f,            0.0f},               //G_DU_DEG_LATITUDE
};

#ifndef __RTXC__
PRIVATE const char * s_cXmlAtributes[] =
{
   "version",
   NULL
};

PRIVATE const char *ppcDirectionOfTurnOptions[] =
{
   "Ahead",
   "Port",
   "Starboard",
   NULL
};
#endif

/********************          GLOBAL VARIABLES              ******************/

DT_BASIC_TYPE(GLOBAL, DT_DEFAULT,     G_DF_UNDEFINED,      G_DU_NONE);

//these are the basic generic types
DT_BASIC_TYPE(GLOBAL, DT_UNSIGNED64,  G_DF_UNSIGNED64,      G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_UNSIGNED32,  G_DF_UNSIGNED32,      G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_UNSIGNED16,  G_DF_UNSIGNED16,      G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_UNSIGNED8,   G_DF_UNSIGNED8,       G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_SIGNED64,    G_DF_SIGNED64,        G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_SIGNED32,    G_DF_SIGNED32,        G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_SIGNED16,    G_DF_SIGNED16,        G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_SIGNED8,     G_DF_SIGNED8,         G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_DOUBLE,      G_DF_DOUBLE,          G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_FLOAT,       G_DF_FLOAT,           G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_POINTER,     G_DF_VOID_POINTER,    G_DU_NONE);
DT_BASIC_TYPE(GLOBAL, DT_BOOLEAN,     G_DF_BOOLEAN,         G_DU_NONE);

DT_STRING_TYPE(GLOBAL, DT_STRING);   //C type C_STRING
DT_WSTRING_TYPE(GLOBAL, DT_WSTRING);   //C type C_WSTRING
DT_BUFFER_TYPE(GLOBAL, DT_BUFFER);   //C type C_BUFFER

//ptrs to types that may or may not be null
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_UNSIGNED64, DT_UNSIGNED64);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_UNSIGNED32, DT_UNSIGNED32);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_UNSIGNED16, DT_UNSIGNED16);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_UNSIGNED8,  DT_UNSIGNED8);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_SIGNED64,   DT_SIGNED64);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_SIGNED32,   DT_SIGNED32);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_SIGNED16,   DT_SIGNED16);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_SIGNED8,    DT_SIGNED8);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_DOUBLE,     DT_DOUBLE);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_FLOAT,      DT_FLOAT);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_POINTER,    DT_POINTER);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_BOOLEAN,    DT_BOOLEAN);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_STRING,     DT_STRING);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_WSTRING,    DT_WSTRING);
DT_PTR_TO_TYPE(GLOBAL, DT_PTR_TO_BUFFER,     DT_BUFFER);

//Basic Dimentional Types
#ifdef __RTXC__
DT_BASIC_TYPE(GLOBAL, DT_DISTANCE,      G_DF_FLOAT,      G_DU_METERS);
DT_BASIC_TYPE(GLOBAL, DT_DEPTH,         G_DF_FLOAT,      G_DU_METERS);
DT_BASIC_TYPE(GLOBAL, DT_ALTITUDE,      G_DF_FLOAT,      G_DU_METERS);
DT_BASIC_TYPE(GLOBAL, DT_SPEED,         G_DF_FLOAT,      G_DU_MPS);
DT_BASIC_TYPE(GLOBAL, DT_TIME,          G_DF_FLOAT,      G_DU_SECONDS);
DT_BASIC_TYPE(GLOBAL, DT_DATETIME,      G_DF_UNSIGNED32, G_DU_SECONDS);
DT_BASIC_TYPE(GLOBAL, DT_PRESSURE,      G_DF_FLOAT,      G_DU_HECTOPASCALS);
DT_BASIC_TYPE(GLOBAL, DT_VOLUME,        G_DF_FLOAT,      G_DU_M3);
DT_BASIC_TYPE(GLOBAL, DT_POTENTIAL,     G_DF_FLOAT,      G_DU_VOLTS);
DT_BASIC_TYPE(GLOBAL, DT_CURRENT,       G_DF_FLOAT,      G_DU_AMPERES);
DT_BASIC_TYPE(GLOBAL, DT_TEMPERATURE,   G_DF_FLOAT,      G_DU_CELSIUS);
DT_BASIC_TYPE(GLOBAL, DT_HEADING,       G_DF_FLOAT,      G_DU_DEG_TRUE);
DT_BASIC_TYPE(GLOBAL, DT_RATEOFTURN,    G_DF_FLOAT,      G_DU_DEG_PER_MINUTE);
DT_BASIC_TYPE(GLOBAL, DT_MAGVAR,        G_DF_FLOAT,      G_DU_DEG_TRUE);
DT_BASIC_TYPE(GLOBAL, DT_LATITUDE,      G_DF_FLOAT,      G_DU_DEG_LATITUDE);
DT_BASIC_TYPE(GLOBAL, DT_LONGITUDE,     G_DF_FLOAT,      G_DU_DEG_LONGITUDE);
#else
DT_BASIC_TYPE(GLOBAL, DT_DISTANCE,      G_DF_DOUBLE,     G_DU_METERS);
DT_BASIC_TYPE(GLOBAL, DT_DEPTH,         G_DF_DOUBLE,     G_DU_METERS);
DT_BASIC_TYPE(GLOBAL, DT_ALTITUDE,      G_DF_DOUBLE,     G_DU_METERS);
DT_BASIC_TYPE(GLOBAL, DT_SPEED,         G_DF_DOUBLE,     G_DU_MPS);
DT_BASIC_TYPE(GLOBAL, DT_TIME,          G_DF_DOUBLE,     G_DU_SECONDS);
DT_BASIC_TYPE(GLOBAL, DT_DATETIME,      G_DF_UNSIGNED32, G_DU_SECONDS);
DT_BASIC_TYPE(GLOBAL, DT_PRESSURE,      G_DF_DOUBLE,     G_DU_HECTOPASCALS);
DT_BASIC_TYPE(GLOBAL, DT_VOLUME,        G_DF_DOUBLE,     G_DU_M3);
DT_BASIC_TYPE(GLOBAL, DT_POTENTIAL,     G_DF_DOUBLE,     G_DU_VOLTS);
DT_BASIC_TYPE(GLOBAL, DT_CURRENT,       G_DF_DOUBLE,     G_DU_AMPERES);
DT_BASIC_TYPE(GLOBAL, DT_TEMPERATURE,   G_DF_DOUBLE,     G_DU_CELSIUS);
DT_BASIC_TYPE(GLOBAL, DT_HEADING,       G_DF_DOUBLE,     G_DU_DEG_TRUE);
DT_BASIC_TYPE(GLOBAL, DT_RATEOFTURN,    G_DF_DOUBLE,     G_DU_DEG_PER_MINUTE);
DT_BASIC_TYPE(GLOBAL, DT_MAGVAR,        G_DF_DOUBLE,     G_DU_DEG_TRUE);
DT_BASIC_TYPE(GLOBAL, DT_LATITUDE,      G_DF_DOUBLE,     G_DU_DEG_LATITUDE);
DT_BASIC_TYPE(GLOBAL, DT_LONGITUDE,     G_DF_DOUBLE,     G_DU_DEG_LONGITUDE);
#endif
//debugging/xml strings
GLOBAL const char * const gDATATYPE_DataFormatString[G_DATA_FORMAT_COUNT] =
{
   "Undefined",         //G_DF_UNDEFINED
   "Unsigned64",        //G_DF_UNSIGNED64
   "Signed64",          //G_DF_SIGNED64
   "Double",            //G_DF_DOUBLE
   "Unsigned32",        //G_DF_UNSIGNED32
   "Signed32",          //G_DF_SIGNED32
   "Enum",              //G_DF_ENUM
   "Float",             //G_DF_FLOAT
   "VoidPointer",       //G_DF_VOID_POINTER
   "Boolean",           //G_DF_BOOLEAN
   "Unsigned16",        //G_DF_UNSIGNED16
   "Signed16",          //G_DF_SIGNED16
   "Unsigned8",         //G_DF_UNSIGNED8
   "Signed8",           //G_DF_SIGNED8
   "String",            //G_DF_STRING
   "WideString",        //G_DF_WSTRING
   "Buffer",            //G_DF_BUFFER
   "Opaque",            //G_DF_OPAQUE
   "Excluded",          //G_DF_EXCLUDED
   "FixedArray",        //G_DF_FIXED_ARRAY
   "DynamicArray",      //G_DF_DYNAMIC_ARRAY
   "NullTermPtrArray",  //G_DF_NULL_TERM_PTR_ARRAY
   "PointerToType",     //G_DF_POINTER_TO_TYPE
   "Struct",            //G_DF_STRUCT
   "Custom",            //G_DF_CUSTOM
};

//debugging/xml strings
GLOBAL const char * const gDATATYPE_DataDimensionString[G_DATA_DIMENSION_COUNT] =
{
   "Scalar",                 //G_DD_SCALAR
   "DistanceDepthAltitude",  //G_DD_DISTANCE_DEPTH_ALTITUDE
   "Speed",                  //G_DD_SPEED
   "Time",                   //G_DD_TIME
   "Pressure",               //G_DD_PRESSURE
   "Volume",                 //G_DD_VOLUME
   "Potential",              //G_DD_POTENTIAL
   "Current",                //G_DD_CURRENT
   "Temperature",            //G_DD_TEMPERATURE
   "Heading",                //G_DD_HEADING
   "RateOfTurn",             //G_DD_RATEOFTURN
   "Latitude",               //G_DD_LATITUDE
   "Longitude",              //G_DD_LONGITUDE
};

//debugging/xml strings
GLOBAL const char * const gDATATYPE_DataUnitString[G_DATA_UNIT_COUNT] =
{
   "Scalar",                 //G_DU_NONE
   "Meters",                 //G_DU_METERS
   "StatuteMiles",           //G_DU_STAT_MILES
   "NauticalMiles",          //G_DU_NAUT_MILES
   "Kilometers",             //G_DU_KM
   "Inches",                 //G_DU_INCH
   "Feet",                   //G_DU_FEET
   "Fathoms",                //G_DU_FATHOMS
   "MetersPerSecond",        //G_DU_MPS
   "Knots",                  //G_DU_KNOTS
   "MilesPerHour",           //G_DU_MPH
   "KilometersPerHour",      //G_DU_KPH
   "Seconds",                //G_DU_SECONDS
   "Minutes",                //G_DU_MINUTES
   "Hours",                  //G_DU_HOURS
   "Days",                   //G_DU_DAYS
   "Hectopascals",           //G_DU_HECTOPASCALS
   "Millibars",              //G_DU_MILLIBAR
   "InchesOfMercury",        //G_DU_INCHESHG
   "MillimetersOfMercury",   //G_DU_MMHG
   "Cubic_Meters",           //G_DU_M3
   "Liters",                 //G_DU_LITERS
   "GallonsUS",              //G_DU_USGALLONS
   "GallonsUK",              //G_DU_UKGALLONS
   "Volts",                  //G_DU_VOLTS
   "Amperes",                //G_DU_AMPERES
   "Milliamperes",           //G_DU_MILLIAMPERES
   "DegreesCelcius",         //G_DU_CELSIUS
   "DegreesFahrenheit",      //G_DU_FAHRENHEIT
   "DegreesTrue",            //G_DU_DEG_TRUE
   "DegreesPerMinute",       //G_DU_DEG_PER_MINUTE
   "DegreesLatitude",        //G_DU_DEG_LATITUDE
   "DegreesLongitude",       //G_DU_DEG_LONGITUDE
};

//Base units used for data storage
GLOBAL const G_DATA_UNIT gDATATYPE_BaseDataUnit[G_DATA_DIMENSION_COUNT] =
{
   G_DU_NONE,           //G_DD_SCALAR
   G_DU_METERS,         //G_DD_DISTANCE_DEPTH_ALTITUDE
   G_DU_MPS,            //G_DD_SPEED
   G_DU_SECONDS,        //G_DD_TIME
   G_DU_HECTOPASCALS,   //G_DD_PRESSURE
   G_DU_M3,             //G_DD_VOLUME
   G_DU_VOLTS,          //G_DD_POTENTIAL
   G_DU_AMPERES,        //G_DD_CURRENT
   G_DU_CELSIUS,        //G_DD_TEMPERATURE
   G_DU_DEG_TRUE,       //G_DD_HEADING
   G_DU_DEG_PER_MINUTE, //G_DD_RATEOFTURN
   G_DU_DEG_LATITUDE,   //G_DD_LATITUDE
   G_DU_DEG_LONGITUDE,  //G_DD_LONGIGUTE
};

/********************              FUNCTIONS                 ******************/

/******************************************************************************
 *
 *    Function: DATATYPE_nStandardSerializer
 *
 *    Args:    pvStruct       - ptr to struct to be serialized
 *             ppvSerialized  - where to store the serialized data
 *                              give address of NULL ptr to allocate
 *             pnRemaining    - bytes remaining in serialized buffer
 *                              if pre-allocated buffer, set max size here
 *                              otherwise, should be zero
 *             pnSize         - size of serialized buffer
 *                              set zero
 *             pDataType      - data type
 *
 *    Return:  int - return size of serialized data
 *                   DATA_LENGTH_ERROR on error
 *
 *    Purpose: Standard data serializer
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL int DATATYPE_nStandardSerializer(void *pvStruct, void **ppvSerialized, int *pnRemaining, int *pnSize, const G_DATA_TYPE *pDataType)
{
   U8 *pStructData;
   U8 *pSerializedData;
   int nDataLength;
   int nDataLengthCheck;
   int nAlign;
   int nAdjust;
   const G_DATA_TYPE **DataTypes = pDataType->DataTypes;
   BOOLEAN bAllocated = FALSE;

   //check alignment
   if (0 > (nAlign = pDataType->pfAlignment(ALIGN_MEMORY, pDataType)))
   {
      return(nAlign);
   }
   else if (((U32)(nAlign-1) & (U32)pvStruct) != 0)
   {
      return(DATA_ALIGN_ERROR); //error
   }

   //Calculate the serialized length of the structure
   if (0 > (nDataLength = pDataType->pfLength(pvStruct, pDataType)))
   {
      return(nDataLength); //error
   }

   //if a buffer to contain the serialized data has not been allocated, allocate it here
   if (*ppvSerialized == NULL)
   {
       if (NULL == (*ppvSerialized = malloc(nDataLength)))
       {
         return(DATA_ALLOC_ERROR);
       }
       *pnRemaining = nDataLength;
       *pnSize      = 0;
       bAllocated   = TRUE;
   }

   //verify there is room in the buffer to store the serialized data
   if (nDataLength > *pnRemaining)
   {
      return(DATA_BUFFER_OVERFLOW);
   }

   //copy the derefereced double pointer so we don't modify the value
   pStructData          = pvStruct;
   pSerializedData      = *ppvSerialized;


   //are we serializing a type which has no sub types?
   //then handle them here
   if (DataTypes == NULL)
   {
      switch(pDataType->eDataFormat)
      {
      case G_DF_UNSIGNED64:
      case G_DF_SIGNED64:
      case G_DF_DOUBLE:
         memcpy(pSerializedData, pStructData, sizeof(U64));
            nDataLengthCheck = sizeof(U64);
         break;
      case G_DF_UNSIGNED32:
      case G_DF_SIGNED32:
      case G_DF_FLOAT:
         memcpy(pSerializedData, pStructData, sizeof(U32));
            nDataLengthCheck = sizeof(U32);
         break;
      case G_DF_ENUM:
         memcpy(pSerializedData, pStructData, pDataType->nSizeofType);
         nDataLengthCheck = pDataType->nSizeofType;
         break;
      case G_DF_UNSIGNED16:
      case G_DF_SIGNED16:
         memcpy(pSerializedData, pStructData, sizeof(U16));
            nDataLengthCheck = sizeof(U16);
         break;
      case G_DF_UNSIGNED8:
      case G_DF_SIGNED8:
         memcpy(pSerializedData, pStructData, sizeof(U8));
         nDataLengthCheck = sizeof(U8);
         break;
      case G_DF_BOOLEAN:
         memcpy(pSerializedData, pStructData, sizeof(BOOLEAN));
         nDataLengthCheck = sizeof(BOOLEAN);
         break;
      case G_DF_VOID_POINTER:
         memcpy(pSerializedData, pStructData, sizeof(void*));
            nDataLengthCheck = sizeof(void*);
         break;
      case G_DF_STRING:
         strcpy((char*)pSerializedData, (char*)pStructData);
         nDataLengthCheck = STRING_SIZE((char*)pStructData);
         break;
      case G_DF_WSTRING:
         {
            U32 u32WideChar;
            nDataLengthCheck = WSTRING_SIZE((wchar_t*)pStructData);
            while ((*(wchar_t*)pStructData) != L'\0')
            {
               u32WideChar = (*(wchar_t*)pStructData);
               memcpy(pSerializedData, &u32WideChar, sizeof(U32));
               pSerializedData += sizeof(U32);
               pStructData     += sizeof(wchar_t);
            }
            u32WideChar = 0;
            memcpy(pSerializedData, &u32WideChar, sizeof(U32));
         }
         break;
      case G_DF_BUFFER:
         {
            U32 u32BufferLength;
            memcpy(&u32BufferLength, pStructData, sizeof(U32));
            memcpy(pSerializedData, &u32BufferLength, sizeof(U32));
            pStructData       += sizeof(U32);
            pSerializedData   += sizeof(U32);
            if (u32BufferLength > 0)
            {
               memcpy(pSerializedData, *((void**)pStructData), u32BufferLength);
            }
            nDataLengthCheck = BUFFER_SIZE(u32BufferLength);
         }
         break;
      case G_DF_OPAQUE:
         memcpy(pSerializedData, pStructData, pDataType->nSizeofType);
         nDataLengthCheck = pDataType->nSizeofType;
         break;
      case G_DF_EXCLUDED:
         nDataLengthCheck = 0;
         break;
      case G_DF_UNDEFINED:
      default:
         return(DATA_FORMAT_ERROR);
      }
      *pnRemaining          -= nDataLengthCheck;
      *pnSize               += nDataLengthCheck;
   }
   else
   {
      switch(pDataType->eDataFormat)
      {
      case G_DF_FIXED_ARRAY:
         {
            int i;

            nDataLengthCheck = 0;

            //loop through array
            for(i = 0; i < (pDataType->nSizeofType / (*DataTypes)->nSizeofType); i++)
            {
               int nMemberSize;
               if (0 > (nMemberSize = (*DataTypes)->pfSerialize((void*)pStructData, (void**)(void*)&pSerializedData, pnRemaining, pnSize, *DataTypes)))
               {
                  //PANIC - there was an error
                  if (bAllocated)
                  {
                     free(*ppvSerialized);
                  }
                  return(nMemberSize); //error
               }
               pSerializedData      += nMemberSize;
               nDataLengthCheck     += nMemberSize;
               pStructData          += (*DataTypes)->nSizeofType;
            }
         }
         break;
      case G_DF_DYNAMIC_ARRAY:
         {
            int nElements;

            memcpy(&nElements, pStructData, sizeof(int));
            memcpy(pSerializedData, &nElements, sizeof(int));
            pStructData      += sizeof(int);
            pSerializedData  += sizeof(int);
            *pnRemaining     -= sizeof(int);
            *pnSize          += sizeof(int);
            nDataLengthCheck  = sizeof(int);
            if (nElements > 0)
            {
               int i;
               U8 *pu8ArrayData = *((U8**)pStructData);

               for (i = 0; i < nElements; i++)
               {
                  int nMemberSize;
                  if (0 > (nMemberSize = (*DataTypes)->pfSerialize((void*)pu8ArrayData, (void**)(void*)&pSerializedData, pnRemaining, pnSize, *DataTypes)))
                  {
                     //PANIC - there was an error
                     if (bAllocated)
                     {
                        free(*ppvSerialized);
                     }
                     return(nMemberSize); //error
                  }
                  pSerializedData      += nMemberSize;
                  nDataLengthCheck     += nMemberSize;
                  pu8ArrayData         += (*DataTypes)->nSizeofType;
               }
            }
            pStructData += sizeof(void*);
         }
         break;
      case G_DF_NULL_TERM_PTR_ARRAY:
         {
            int nElements;
            void **pvArrayData;

            if ((*DataTypes)->eDataFormat != G_DF_POINTER_TO_TYPE)
            {
               return(DATA_FORMAT_ERROR);
            }

            pvArrayData = *((void***)pStructData);
            if (pvArrayData != NULL)
            {
               for (nElements = 0; *pvArrayData++ != NULL; nElements++)
               {
                  //empty loop, count elements
               }
            }
            else
            {
               nElements = 0;
            }
            memcpy(pSerializedData, &nElements, sizeof(int));
            pSerializedData  += sizeof(int);
            *pnRemaining     -= sizeof(int);
            *pnSize          += sizeof(int);
            nDataLengthCheck  = sizeof(int);
            if (nElements > 0)
            {
               int i;
               U8 *pu8ArrayData = *((U8**)pStructData);

               for (i = 0; i < nElements; i++)
               {
                  int nMemberSize;
                  if (0 > (nMemberSize = (*DataTypes)->pfSerialize((void*)pu8ArrayData, (void**)(void*)&pSerializedData, pnRemaining, pnSize, *DataTypes)))
                  {
                     //PANIC - there was an error
                     if (bAllocated)
                     {
                        free(*ppvSerialized);
                     }
                     return(nMemberSize); //error
                  }
                  pSerializedData      += nMemberSize;
                  nDataLengthCheck     += nMemberSize;
                  pu8ArrayData         += (*DataTypes)->nSizeofType;
               }
            }
            pStructData += sizeof(void**);
         }
         break;
      case G_DF_POINTER_TO_TYPE:
         {
            void *pNullPtr = NULL;
            *pnRemaining            -= sizeof(void*);
            *pnSize                 += sizeof(void*);
            nDataLengthCheck         = sizeof(void*);
            if ( 0 == memcmp(pStructData, &pNullPtr, sizeof(void*)) )
            {
               memcpy(pSerializedData, &pNullPtr, sizeof(void*));
            }
            else
            {
               void *pSerializedPtr = (void*)SERIALIZED_POINTER;
               memcpy(pSerializedData, &pSerializedPtr, sizeof(void*));
               pSerializedData           += sizeof(void*);
               nDataLengthCheck          += (*DataTypes)->pfSerialize(*((void**)pStructData), (void**)(void*)&pSerializedData, pnRemaining, pnSize, *DataTypes);
            }
         }
         break;
      case G_DF_STRUCT:
         {
            //structures require a version
            memcpy(pSerializedData, &pDataType->nVersion, sizeof(int));
            pSerializedData += sizeof(int);
            *pnRemaining    -= sizeof(int);
            *pnSize         += sizeof(int);
            nDataLengthCheck = sizeof(int);

            //loop through all data types
            while (*DataTypes != NULL)
            {
               int nMemberSize;

               //handle alignment of structure members
               if (0 > (nAlign = pDataType->pfAlignment(ALIGN_STRUCT, *DataTypes)))
               {
                  if (bAllocated)
                  {
                     free(*ppvSerialized);
                  }
                  return(DATA_ALIGN_ERROR); //error
               }
               nAdjust = (U32)(nAlign-1) & (U32)pStructData;
               nAdjust = (nAdjust) ? (nAlign - nAdjust) : 0;
               pStructData += nAdjust;

               if (0 > (nMemberSize = (*DataTypes)->pfSerialize((void*)pStructData, (void**)(void*)&pSerializedData, pnRemaining, pnSize, *DataTypes)))
               {
                  //PANIC - there was an error
                  if (bAllocated)
                  {
                     free(*ppvSerialized);
                  }
                  return(nMemberSize); //error
               }
               pSerializedData      += nMemberSize;
               nDataLengthCheck     += nMemberSize;
               pStructData          += (*DataTypes)->nSizeofType;
               DataTypes++;
            }
         }
         break;
      default:
         return(DATA_FORMAT_ERROR);
      }
   }

   //Final length check
   if (nDataLengthCheck != nDataLength)
   {
      if (bAllocated)
      {
         free(*ppvSerialized);
      }
      return(DATA_LENGTH_ERROR);
   }
   return(nDataLength);
}

/******************************************************************************
 *
 *    Function: DATATYPE_nStandardDeserializer
 *
 *    Args:    ppvStruct      - where to allocate our structure
 *             pvSerialized   - serialized data to deserialze
 *             pnRemaining    - amount of serialized data remaining
 *                              init to initalize size of serialized
 *                              data
 *             pnSize         - amount of serialized data used to
 *                              create structure
 *             pDataType      - type of data we are deserializing
 *
 *    Return:  int - amount of data that was deserialized from the serialized
 *                   data
 *                   DATA_LENGTH_ERROR on error
 *
 *    Purpose: Standard data deserializer
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL int DATATYPE_nStandardDeserializer(void **ppvStruct, void *pvSerialized, int *pnRemaining, int *pnSize, const G_DATA_TYPE *pDataType)
{
   U8 *pStructData;
   U8 *pSerializedData;
   int nDataLength;
   int nAlign;
   int nAdjust;
   const G_DATA_TYPE **DataTypes = pDataType->DataTypes;
   BOOLEAN bAllocated = FALSE;

   //check alignment
   if (0 > (nAlign = pDataType->pfAlignment(ALIGN_MEMORY, pDataType)))
   {
      return(nAlign);
   }
   else if (((U32)(nAlign-1) & (U32)(*ppvStruct)) != 0)
   {
      return(DATA_ALIGN_ERROR); //error
   }

   //allocate data based on the size of the structure
   //if it has a dynamic size (ends with a string or buffer) we'll
   //realloc the memory later
   if (*ppvStruct == NULL)
   {
      if (NULL == (*ppvStruct = malloc(pDataType->nSizeofType)))
      {
         return(DATA_ALLOC_ERROR);
      }
      bAllocated = TRUE;
   }

   pStructData           = *ppvStruct;
   pSerializedData       = pvSerialized;

   //if we're down to a basic type handle it specially
   if (DataTypes == NULL)
   {
      switch(pDataType->eDataFormat)
      {
      case G_DF_UNSIGNED64:
      case G_DF_SIGNED64:
      case G_DF_DOUBLE:
         memcpy(pStructData, pSerializedData, sizeof(U64));
            nDataLength = sizeof(U64);
         break;
      case G_DF_UNSIGNED32:
      case G_DF_SIGNED32:
      case G_DF_FLOAT:
         memcpy(pStructData, pSerializedData, sizeof(U32));
            nDataLength = sizeof(U32);
         break;
      case G_DF_ENUM:
         memcpy(pStructData, pSerializedData, pDataType->nSizeofType);
         nDataLength = pDataType->nSizeofType;
         break;
      case G_DF_UNSIGNED16:
      case G_DF_SIGNED16:
         memcpy(pStructData, pSerializedData, sizeof(U16));
            nDataLength = sizeof(U16);
         break;
      case G_DF_UNSIGNED8:
      case G_DF_SIGNED8:
         memcpy(pStructData, pSerializedData, sizeof(U8));
         nDataLength = sizeof(U8);
         break;
      case G_DF_BOOLEAN:
         memcpy(pStructData, pSerializedData, sizeof(BOOLEAN));
         nDataLength = sizeof(BOOLEAN);
         break;
      case G_DF_VOID_POINTER:
         memcpy(pStructData, pSerializedData, sizeof(void*));
            nDataLength = sizeof(void*);
         break;
      case G_DF_STRING:
         {
            U32 u32StringLength = strlen((char*)pSerializedData);
            if (bAllocated)
            {
               if (NULL == (*ppvStruct = realloc(*ppvStruct, u32StringLength+1)))
               {
                  return(DATA_ALLOC_ERROR);
               }
            }
            strcpy((char*)*ppvStruct, (char *)pSerializedData);
            nDataLength = STRING_SIZE((char*)pSerializedData);
         }
         break;
      case G_DF_WSTRING:
         {
            int i = 0;
            wchar_t wcChar;
            U32 u32WideChar;
            while (1)
            {
               memcpy(&u32WideChar, pSerializedData + sizeof(U32)*i, sizeof(U32));
               wcChar = (wchar_t)u32WideChar;
               if (wcChar == L'\0')
               {
                  break;
               }
               i++;
            }
            nDataLength = sizeof(U32)*(i+1);
            if (bAllocated)
            {
               if (NULL == (*ppvStruct = realloc(*ppvStruct, sizeof(wchar_t)*nDataLength/sizeof(U32))))
               {
                  return(DATA_ALLOC_ERROR);
               }
            }
            for (i = 0; i < nDataLength/sizeof(U32); i++)
            {
               memcpy(&u32WideChar, pSerializedData + sizeof(U32)*i, sizeof(U32));
               ((wchar_t *)*ppvStruct)[i] = (wchar_t)u32WideChar;
            }
         }
         break;
      case G_DF_BUFFER:
         {
            U32 u32BufferLength;

            memcpy(&u32BufferLength, pSerializedData, sizeof(U32));
            if (NULL == (((C_BUFFER*)pStructData)->pData = malloc(u32BufferLength)))
            {
               return(DATA_ALLOC_ERROR);
            }
            ((C_BUFFER*)pStructData)->nSize  = u32BufferLength;
            pSerializedData += sizeof(U32);
            memcpy(((C_BUFFER*)pStructData)->pData, pSerializedData, u32BufferLength);
            nDataLength = BUFFER_SIZE(u32BufferLength);
         }
         break;
      case G_DF_OPAQUE:
         memcpy(*ppvStruct, pSerializedData, pDataType->nSizeofType);
         nDataLength = pDataType->nSizeofType;
         break;
      case G_DF_EXCLUDED:
         memset(*ppvStruct, 0, pDataType->nSizeofType);
         nDataLength = 0;
         break;
      case G_DF_UNDEFINED:
      default:
         return(DATA_FORMAT_ERROR);
      }
      *pnRemaining          -= nDataLength;
      *pnSize               += nDataLength;
   }
   else
   {
      switch(pDataType->eDataFormat)
      {
      case G_DF_FIXED_ARRAY:
         {
            int i;

            nDataLength = 0;

            //loop through all data types
            for(i = 0; i < (pDataType->nSizeofType / (*DataTypes)->nSizeofType); i++)
            {
               int nMemberSize;

               if (0 > (nMemberSize = (*DataTypes)->pfDeserialize((void**)(void*)&pStructData, (void*)pSerializedData, pnRemaining, pnSize, *DataTypes)))
               {
                  return(nMemberSize); //error
               }
               pSerializedData      += nMemberSize;
               nDataLength          += nMemberSize;
               pStructData          += (*DataTypes)->nSizeofType;
            }
         }
         break;
      case G_DF_DYNAMIC_ARRAY:
         {
            int nElements;

            memcpy(&nElements, pSerializedData, sizeof(int));
            memcpy(pStructData, &nElements, sizeof(int));
            pStructData      += sizeof(int);
            pSerializedData  += sizeof(int);
            nDataLength       = sizeof(int);

            if (nElements > 0)
            {
               int i;
               U8 *pu8ArrayData;

               if (NULL == (*((void**)pStructData) = malloc(nElements * (*DataTypes)->nSizeofType)))
               {
                  return(DATA_ALLOC_ERROR);
               }
               pu8ArrayData = *((U8**)pStructData);

               for (i = 0; i < nElements; i++)
               {
                  int nMemberSize;

                  if (0 > (nMemberSize = (*DataTypes)->pfDeserialize((void**)(void*)&pu8ArrayData, (void*)pSerializedData, pnRemaining, pnSize, *DataTypes)))
                  {
                     return(nMemberSize); //error
                  }
                  pSerializedData      += nMemberSize;
                  nDataLength          += nMemberSize;
                  pu8ArrayData         += (*DataTypes)->nSizeofType;
               }
            }
            else
            {
               *(void**)pStructData = NULL;
            }
            pStructData += sizeof(void*);
         }
         break;
      case G_DF_NULL_TERM_PTR_ARRAY:
         {
            int nElements;

            if ((*DataTypes)->eDataFormat != G_DF_POINTER_TO_TYPE)
            {
               return(DATA_FORMAT_ERROR);
            }
            memcpy(&nElements, pSerializedData, sizeof(int));
            pSerializedData  += sizeof(int);
            nDataLength       = sizeof(int);

            if (nElements > 0)
            {
               int i;
               U8 *pu8ArrayData;

               if (NULL == (*((void**)pStructData) = malloc((nElements+1) * (*DataTypes)->nSizeofType)))
               {
                  return(DATA_ALLOC_ERROR);
               }
               pu8ArrayData = *((U8**)pStructData);

               for (i = 0; i < nElements; i++)
               {
                  int nMemberSize;

                  if (0 > (nMemberSize = (*DataTypes)->pfDeserialize((void**)(void*)&pu8ArrayData, (void*)pSerializedData, pnRemaining, pnSize, *DataTypes)))
                  {
                     return(nMemberSize); //error
                  }
                  pSerializedData      += nMemberSize;
                  nDataLength          += nMemberSize;
                  pu8ArrayData         += (*DataTypes)->nSizeofType;
               }
               *(void**)pu8ArrayData = NULL;
            }
            else
            {
               *(void**)pStructData = NULL;
            }
            pStructData += sizeof(void*);
         }
         break;
      case G_DF_POINTER_TO_TYPE:
         {
            void *pNullPtr = NULL;
            *pnRemaining -= sizeof(void*);
            *pnSize      += sizeof(void*);
            nDataLength   = sizeof(void*);
            memcpy(pStructData, &pNullPtr, sizeof(void*));
            if (0 != memcmp(pSerializedData, &pNullPtr, sizeof(void*)))
            {
               pSerializedData += sizeof(void*);
               nDataLength     += (*DataTypes)->pfDeserialize((void**)pStructData, (void*)pSerializedData, pnRemaining, pnSize, *DataTypes);
            }
         }
         break;
      case G_DF_STRUCT:
         {
            const G_DATA_TYPE  *pVerDataType;
            const G_DATA_TYPE **VerDataTypes;
            U8                 *pVerStructData, *pVerStructDataStart;
            int                 nVersion;

            //find the right version for this structure
            memcpy(&nVersion, pSerializedData, sizeof(int));
            if (NULL == (pVerDataType = DATATYPE_pFindVersion(pDataType, nVersion)))
            {
               return(DATA_VERSION_ERROR);
            }
            VerDataTypes = pVerDataType->DataTypes;

            if (pVerDataType == pDataType)
            {
               pVerStructDataStart = pStructData;
            }
            else if (NULL == (pVerStructDataStart = malloc(pVerDataType->nSizeofType)))
            {
               return(DATA_ALLOC_ERROR);
            }

            pVerStructData = pVerStructDataStart;

            pSerializedData += sizeof(int);
            *pnRemaining    -= sizeof(int);
            *pnSize         += sizeof(int);
            nDataLength      = sizeof(int);

            //loop through all data types
            while (*VerDataTypes != NULL)
            {
               int nMemberSize;

               //handle alignment of structure members
               if (0 > (nAlign = pVerDataType->pfAlignment(ALIGN_STRUCT, *VerDataTypes)))
               {
                  return(nAlign); //error
               }
               nAdjust = (U32)(nAlign-1) & (U32)(pVerStructData-pVerStructDataStart);
               nAdjust = (nAdjust) ? (nAlign - nAdjust) : 0;
               pVerStructData += nAdjust;

               if ((*VerDataTypes)->eDataFormat == G_DF_STRING)
               {
                  U8 *pTemp;

                  if (NULL == (pTemp = realloc(pVerStructDataStart, pVerDataType->nSizeofType + strlen((char *)pSerializedData) + 1)))
                  {
                     return(DATA_ALLOC_ERROR);
                  }
                  pVerStructData = pTemp + (pVerStructData - pVerStructDataStart); //lint !e644 (it's init'ed) in case we get a new memory pointer
                  pVerStructDataStart = pTemp;
               } //lint !e429
               if (0 > (nMemberSize = (*VerDataTypes)->pfDeserialize((void**)(void*)&pVerStructData, (void*)pSerializedData, pnRemaining, pnSize, *VerDataTypes)))
               {
                  return(nMemberSize); //error
               }
               pSerializedData      += nMemberSize;
               nDataLength          += nMemberSize;
               pVerStructData       += (*VerDataTypes)->nSizeofType;
               VerDataTypes++;
            }

            if (pVerDataType != pDataType)
            {
               int nConvert;

               if (0 > (nConvert = pVerDataType->pfVersionConvert(pStructData, pVerStructDataStart, pDataType, pVerDataType)))
               {
                  return(nConvert);
               }
               free(pVerStructDataStart);
            }
         }
         break;
      default:
         return(DATA_FORMAT_ERROR);
      }
   }

   return(nDataLength);
}

/******************************************************************************
 *
 *    Function: DATATYPE_nStandardSerializedLength
 *
 *    Args:    pvStruct  - struct data to calculate serialized length
 *             pDataType - type of data to do the calculation on
 *
 *    Return:  int - size of the would-be serialized data
 *
 *    Purpose: Calculate the size of serial buffer needed to store
 *             serialized data
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL int DATATYPE_nStandardSerializedLength(void *pvStruct, const G_DATA_TYPE *pDataType)
{
   U8 *pStructData;
   int nDataLength;
   int nAlign;
   int nAdjust;
   const G_DATA_TYPE **DataTypes = pDataType->DataTypes;

   //check alignment
   if (0 > (nAlign = pDataType->pfAlignment(ALIGN_MEMORY, pDataType)))
   {
      return(nAlign);
   }
   else if (((U32)(nAlign-1) & (U32)pvStruct) != 0)
   {
      return(DATA_ALIGN_ERROR); //error
   }

   pStructData = (U8*)pvStruct;

   if (DataTypes == NULL)
   {
      switch(pDataType->eDataFormat)
      {
      case G_DF_UNSIGNED64:
      case G_DF_SIGNED64:
      case G_DF_DOUBLE:
         nDataLength = sizeof(U64);
         break;
      case G_DF_UNSIGNED32:
      case G_DF_SIGNED32:
      case G_DF_FLOAT:
         nDataLength = sizeof(U32);
         break;
      case G_DF_ENUM:
         nDataLength = pDataType->nSizeofType;
         break;
      case G_DF_UNSIGNED16:
      case G_DF_SIGNED16:
         nDataLength = sizeof(U16);
         break;
      case G_DF_UNSIGNED8:
      case G_DF_SIGNED8:
         nDataLength = sizeof(U8);
         break;
      case G_DF_BOOLEAN:
         nDataLength = sizeof(BOOLEAN);
         break;
      case G_DF_VOID_POINTER:
         nDataLength = sizeof(void*);
         break;
      case G_DF_STRING:
         nDataLength = STRING_SIZE((char*)pStructData);
         break;
      case G_DF_WSTRING:
         nDataLength = WSTRING_SIZE((wchar_t*)pStructData);
         break;
      case G_DF_BUFFER:
         nDataLength = BUFFER_SIZE(*((U32*)pStructData));
         break;
      case G_DF_OPAQUE:
         nDataLength = pDataType->nSizeofType;
         break;
      case G_DF_EXCLUDED:
         nDataLength = 0;
         break;
      case G_DF_UNDEFINED:
      default:
         return(DATA_FORMAT_ERROR);
      }
   }
   else
   {
      switch(pDataType->eDataFormat)
      {
      case G_DF_FIXED_ARRAY:
         {
            int i;

            nDataLength = 0;

            for(i = 0; i < (pDataType->nSizeofType / (*DataTypes)->nSizeofType); i++)
            {
               int nMemberSize;

               if (0 > (nMemberSize = (*DataTypes)->pfLength((void*)pStructData, *DataTypes)))
               {
                  return(nMemberSize); //error
               }

               pStructData += (*DataTypes)->nSizeofType;
               nDataLength += nMemberSize;
            }
         }
         break;
      case G_DF_DYNAMIC_ARRAY:
         {
            int nElements;

            nElements    = *((int *)pStructData);
            pStructData += sizeof(int);
            nDataLength  = sizeof(int);

            if (nElements > 0)
            {
               int i;
               U8 *pu8ArrayData = *((U8**)pStructData);

               for (i = 0; i < nElements; i++)
               {
                  int nMemberSize;

                  if (0 > (nMemberSize = (*DataTypes)->pfLength((void*)pu8ArrayData, *DataTypes)))
                  {
                     return(nMemberSize); //error
                  }

                  pu8ArrayData += (*DataTypes)->nSizeofType;
                  nDataLength  += nMemberSize;
               }
            }
            pStructData += sizeof(void*);
         }
         break;
      case G_DF_NULL_TERM_PTR_ARRAY:
         {
            int nElements;
            void **pvArrayData;

            if ((*DataTypes)->eDataFormat != G_DF_POINTER_TO_TYPE)
            {
               return(DATA_FORMAT_ERROR);
            }

            nDataLength  = sizeof(int);
            pvArrayData = *((void***)pStructData);
            if (pvArrayData != NULL)
            {
               for (nElements = 0; *pvArrayData++ != NULL; nElements++)
               {
                  //empty loop, count elements
               }
               if (nElements > 0)
               {
                  int i;
                  U8 *pu8ArrayData = *((U8**)pStructData);

                  for (i = 0; i < nElements; i++)
                  {
                     int nMemberSize;

                     if (0 > (nMemberSize = (*DataTypes)->pfLength((void*)pu8ArrayData, *DataTypes)))
                     {
                        return(nMemberSize); //error
                     }
                     pu8ArrayData += (*DataTypes)->nSizeofType;
                     nDataLength  += nMemberSize;
                  }
               }
            }
            pStructData += sizeof(void**);
         }
         break;
      case G_DF_POINTER_TO_TYPE:
         if (*((void**)pStructData) == NULL)
         {
            nDataLength = sizeof(void*);
         }
         else
         {
            nDataLength =  sizeof(void*) + (*DataTypes)->pfLength(*((void**)pStructData), *DataTypes);
         }
         break;
      case G_DF_STRUCT:
         nDataLength = sizeof(int); //version
         while (*DataTypes != NULL)
         {
            int nMemberSize;

            //handle alignment of structure members
            if (0 > (nAlign = pDataType->pfAlignment(ALIGN_STRUCT, *DataTypes)))
            {
               return(nAlign); //error
            }
            nAdjust = (U32)(nAlign-1) & (U32)pStructData;
            nAdjust = (nAdjust) ? (nAlign - nAdjust) : 0;
            pStructData += nAdjust;

            if (0 > (nMemberSize = (*DataTypes)->pfLength((void*)pStructData, *DataTypes)))
            {
               return(nMemberSize); //error
            }

            pStructData += (*DataTypes)->nSizeofType;
            nDataLength += nMemberSize;
            DataTypes++;
         }
         break;
      default:
         return(DATA_FORMAT_ERROR);
      }
   }

   return(nDataLength);
}


/******************************************************************************
 *
 *    Function: DATATYPE_nStandardDeallocator
 *
 *    Args:    pvStruct       - structure/data to deallocate
 *             bTypeAllocated - this portion of structure was allocated?
 *             pDataType      - type of structure/data to deallocate
 *
 *    Return:  int - 0 on success
 *                   DATA_LENGTH_ERROR on error
 *
 *    Purpose: deallocate data which has been allocated with the
 *             standard deserializer
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL int DATATYPE_nStandardDeallocator(void *pvStruct, BOOLEAN bTypeAllocated, const G_DATA_TYPE *pDataType)
{
   U8 *pStructData;
   int nAlign;
   int nAdjust;
   const G_DATA_TYPE **DataTypes = pDataType->DataTypes;

   //check alignment
   if (0 > (nAlign = pDataType->pfAlignment(ALIGN_MEMORY, pDataType)))
   {
      return(nAlign); //error
   }
   else if (((U32)(nAlign-1) & (U32)pvStruct) != 0)
   {
      return(DATA_ALIGN_ERROR);
   }

   pStructData = (U8*)pvStruct;

   if (DataTypes == NULL)
   {
      if (pDataType->eDataFormat == G_DF_BUFFER)
      {
         free(((C_BUFFER*)pStructData)->pData);
      }
   }
   else
   {
      switch(pDataType->eDataFormat)
      {
      case G_DF_FIXED_ARRAY:
         {
            int i;

            for(i = 0; i < (pDataType->nSizeofType / (*DataTypes)->nSizeofType); i++)
            {
               (void)(*DataTypes)->pfDeallocate((void*)pStructData, FALSE, (*DataTypes));
               pStructData += (*DataTypes)->nSizeofType;
            }
         }
         break;
      case G_DF_DYNAMIC_ARRAY:
         {
            int nElements;

            nElements = *((int *)pStructData);
            pStructData += sizeof(int);

            if (nElements > 0)
            {
               int i;
               U8 *pu8ArrayData = *((U8**)pStructData);

               for(i = 0; i < nElements; i++)
               {
                  (void)(*DataTypes)->pfDeallocate((void*)pu8ArrayData, FALSE, (*DataTypes));
                  pu8ArrayData += (*DataTypes)->nSizeofType;
               }
               free(*((void**)pStructData));
            }
            pStructData += sizeof(void*);
         }
         break;
      case G_DF_NULL_TERM_PTR_ARRAY:
         {
            int i;
            void **pvArrayData;

            if ((*DataTypes)->eDataFormat != G_DF_POINTER_TO_TYPE)
            {
               return(DATA_FORMAT_ERROR);
            }
            pvArrayData = *((void***)pStructData);
            if (pvArrayData != NULL)
            {
               for (i = 0; *pvArrayData++ != NULL; i++)
               {
                  (void)(*DataTypes)->pfDeallocate((void*)pvArrayData, FALSE, (*DataTypes));
               }
               free(*((void**)pStructData));
            }
            pStructData += sizeof(void**);
         } //lint !e550
         break;
      case G_DF_POINTER_TO_TYPE:
         if (*((void**)pvStruct) != NULL)
         {
            (void)(*DataTypes)->pfDeallocate(*((void**)pvStruct), TRUE, *DataTypes);
         }
         break;
      case G_DF_STRUCT:
         {
            pStructData = (U8*)pvStruct;
            while (*DataTypes != NULL)
            {
               //handle alignment of structure members
               if (0 > (nAlign = pDataType->pfAlignment(ALIGN_STRUCT, *DataTypes)))
               {
                  return(nAlign); //error
               }
               nAdjust = (U32)(nAlign-1) & (U32)pStructData;
               nAdjust = (nAdjust) ? (nAlign - nAdjust) : 0;
               pStructData += nAdjust;

               (void)(*DataTypes)->pfDeallocate((void*)pStructData, FALSE, (*DataTypes));
               pStructData += (*DataTypes)->nSizeofType;
               DataTypes++;
            }
         }
         break;
      default:
         return(DATA_FORMAT_ERROR);
      }
   }
   if (bTypeAllocated)
   {
      free(pvStruct);
   }

   return(DATA_SUCCESS);
}

/******************************************************************************
 *
 *    Function: DATATYPE_nStandardAlignment
 *
 *    Args:    eAlignType - ALIGN_MEMORY, ALIGN_STRUCT
 *             pDataType - Data Type
 *
 *    Return:  Alignment for type (1, 2, 4, or 8)
 *
 *    Purpose: Figure out the alignment for a type
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL int DATATYPE_nStandardAlignment(ALIGN_TYPE eAlignType, const G_DATA_TYPE *pDataType)
{
   int nAlignment;
   const G_DATA_TYPE **DataTypes = pDataType->DataTypes;

   if (DataTypes == NULL)
   {
      switch(pDataType->eDataFormat)
      {
      case G_DF_UNSIGNED64:
      case G_DF_SIGNED64:
      case G_DF_DOUBLE:
         nAlignment = (eAlignType == ALIGN_STRUCT) ? sizeof(U64) : sizeof(U32);
         break;
      case G_DF_VOID_POINTER:
         nAlignment = sizeof(void *);
         break;
      case G_DF_UNSIGNED32:
      case G_DF_SIGNED32:
      case G_DF_FLOAT:
      case G_DF_BUFFER:
         nAlignment = sizeof(U32);
         break;
      case G_DF_BOOLEAN:
         nAlignment = sizeof(BOOLEAN);
         break;
      case G_DF_ENUM:
         nAlignment = MIN(pDataType->nSizeofType, (eAlignType == ALIGN_STRUCT) ? sizeof(U64) : sizeof(U32));
         break;
      case G_DF_UNSIGNED16:
      case G_DF_SIGNED16:
         nAlignment = sizeof(U16);
         break;
      case G_DF_UNSIGNED8:
      case G_DF_SIGNED8:
      case G_DF_STRING:
         nAlignment = sizeof(U8);
         break;
      case G_DF_WSTRING:
         nAlignment = sizeof(wchar_t);
         break;
      case G_DF_EXCLUDED:
      case G_DF_OPAQUE:
         nAlignment = MIN(pDataType->nAlignment, (eAlignType == ALIGN_STRUCT) ? sizeof(U64) : sizeof(U32));
         break;
      case G_DF_UNDEFINED:
      default:
         return(DATA_FORMAT_ERROR);
      }
   }
   else
   {
      switch(pDataType->eDataFormat)
      {
      case G_DF_FIXED_ARRAY:
         nAlignment = (*DataTypes)->pfAlignment(eAlignType, *DataTypes);
         break;
      case G_DF_DYNAMIC_ARRAY:
         {
            int nTempAlignment = sizeof(void *);
            nAlignment = (nTempAlignment > sizeof(int)) ? nTempAlignment : sizeof(int);
         }
         break;
      case G_DF_NULL_TERM_PTR_ARRAY:
         nAlignment = sizeof(void **);
         break;
      case G_DF_POINTER_TO_TYPE:
         nAlignment = sizeof(void *); //should handle alignment of 64-bit OS
         break;
      case G_DF_STRUCT:
         {
            int nTempAlignment;

            nAlignment = 1;
            while (*DataTypes != NULL)
            {
               nTempAlignment = (*DataTypes)->pfAlignment(eAlignType, *DataTypes);
               if (nTempAlignment > nAlignment)
               {
                  nAlignment = nTempAlignment;
               }
               DataTypes++;
            }
         }
         break;
      default:
         return(DATA_FORMAT_ERROR);
      }
   }

   return(nAlignment);
}

GLOBAL int DATATYPE_nStandardXmlWriter(void *pvStruct, GXML_WRITER *pWriter, const char *pcName, const G_DATA_TYPE *pDataType)
{
   return(DATA_XML_WRITE_ERROR);
}

GLOBAL int DATATYPE_nStandardXmlReader(void **ppvStruct, GXML_READER *pReader, const G_DATA_TYPE *pDataType)
{
   return(DATA_XML_PARSE_ERROR);
}

/******************************************************************************/
/**
 * Compare the format of two types to see if they are equivalent
 *
 * @param pDataType1 first type to compare
 * @param pDataType2 second type to compare
 *
 * @return BOOLEAN - TRUE on Success
 */
/******************************************************************************/
GLOBAL BOOLEAN DATATYPE_bCompatibleTypes(const G_DATA_TYPE *pDataType1, const G_DATA_TYPE *pDataType2)
{
   if (pDataType1 == pDataType2)
   {
      return(TRUE);
   }
   else if (pDataType1->eDataFormat == pDataType2->eDataFormat)
   {
      switch(pDataType1->eDataFormat)
      {
      case G_DF_UNSIGNED64:
      case G_DF_SIGNED64:
      case G_DF_DOUBLE:
      case G_DF_UNSIGNED32:
      case G_DF_SIGNED32:
      case G_DF_FLOAT:
      case G_DF_UNSIGNED16:
      case G_DF_SIGNED16:
      case G_DF_UNSIGNED8:
      case G_DF_SIGNED8:
      case G_DF_BOOLEAN:
      case G_DF_VOID_POINTER:
      case G_DF_STRING:
      case G_DF_WSTRING:
      case G_DF_BUFFER:
         return(TRUE);
      case G_DF_ENUM:
      case G_DF_OPAQUE:
      case G_DF_EXCLUDED:
         return((pDataType1->nSizeofType == pDataType2->nSizeofType) ? TRUE : FALSE);
      case G_DF_FIXED_ARRAY:
      case G_DF_DYNAMIC_ARRAY:
      case G_DF_NULL_TERM_PTR_ARRAY:
      case G_DF_POINTER_TO_TYPE:
         return(DATATYPE_bCompatibleTypes(*pDataType1->DataTypes, *pDataType2->DataTypes));
      case G_DF_STRUCT:
         {
            const G_DATA_TYPE **DataTypes1 = pDataType1->DataTypes;
            const G_DATA_TYPE **DataTypes2 = pDataType2->DataTypes;

            while (*DataTypes1 != NULL && *DataTypes2 != NULL)
            {
               if (!DATATYPE_bCompatibleTypes(*DataTypes1, *DataTypes2))
               {
                  return(FALSE);
               }
               DataTypes1++;
               DataTypes2++;
            }
            if (*DataTypes1 != NULL || *DataTypes2 != NULL)
            {
               return(FALSE);
            }
         }
         return(TRUE);
      case G_DF_CUSTOM:
      case G_DF_UNDEFINED:
      default:
         return(FALSE);
      }
   }
   else
   {
      return(FALSE);
   }
}

/******************************************************************************
 *
 *    Function: DATATYPE_dConvertUnitToUnit
 *
 *    Args:    dValue    - Value to convert
 *             eUnitFrom - from units
 *             eUnitTo   - to units
 *
 *    Return:  double - value in converted units
 *
 *    Purpose: Convert a double value from units to units
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL double DATATYPE_dConvertUnitToUnit(double dValue, G_DATA_UNIT eUnitFrom, G_DATA_UNIT eUnitTo)
{
   double dInternalValue;

   if (s_DataDimensionTable[eUnitFrom] == s_DataDimensionTable[eUnitTo])
   {
      dInternalValue = (dValue * s_DataConversionTable[eUnitFrom].dConversionRatio) + s_DataConversionTable[eUnitFrom].dConversionOffset;
      return((dInternalValue - s_DataConversionTable[eUnitTo].dConversionOffset) / s_DataConversionTable[eUnitTo].dConversionRatio);
   }
   else
   {
      printf("DATATYPE_dConvertDataToUnit: Incompatible unit conversion - returning 0.0");
      return(0.0);
   }

}

/******************************************************************************
 *
 *    Function: DATATYPE_dConvertDataToUnit
 *
 *    Args:    dValue        - value to convert
 *             pDataTypeFrom - type of value (indicates default units)
 *             eUnitTo       - units to convert to
 *
 *    Return:  double - value in converted units
 *
 *    Purpose: Convert a double value from its default type units to other units
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL double DATATYPE_dConvertDataToUnit(double dValue, const G_DATA_TYPE *pDataTypeFrom, G_DATA_UNIT eUnitTo)
{
   return(DATATYPE_dConvertUnitToUnit(dValue, pDataTypeFrom->eDataUnit, eUnitTo));
}

/******************************************************************************
 *
 *    Function: DATATYPE_dConvertUnitToData
 *
 *    Args:    dValue      - value to convert
 *             eUnitFrom   - units to convert from
 *             pDataTypeTo - data type (indicates units to convert to)
 *
 *    Return:  double - value in data types' units
 *
 *    Purpose: Covert a double value from a unit to the default units for a
 *             particular data type
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL double DATATYPE_dConvertUnitToData(double dValue, G_DATA_UNIT eUnitFrom, const G_DATA_TYPE *pDataTypeTo)
{
   return(DATATYPE_dConvertUnitToUnit(dValue, eUnitFrom, pDataTypeTo->eDataUnit));
}

/******************************************************************************/
/**
 * Find the version of the datatype that matches the serialized or XML
 *
 * @param pDataType
 * @param nVersion
 *
 * @return G_DATA_TYPE* Datatype which matches version
 */
/******************************************************************************/
PRIVATE const G_DATA_TYPE * DATATYPE_pFindVersion(const G_DATA_TYPE *pDataType, int nVersion)
{
   while (pDataType != NULL)
   {
      if (pDataType->nVersion == nVersion)
      {
         return(pDataType);
      }
      pDataType = pDataType->pPreviousVersionType;
   }
   return(NULL);
}
//#endif //MODEL_HAS_NETWORKING

