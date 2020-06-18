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
#ifndef DATATYPE_H
   #define  DATATYPE_H
#ifdef __cplusplus
   extern "C" {
#endif
//#include "compile_flags.h"
/********************           INCLUDE FILES                ******************/
#include "global.h"
#include "types.h"
#include <wchar.h>
/********************               ENUMS                    ******************/

/********************              DEFINES                   ******************/
#define  DATA_SUCCESS            ( 0)
#define  DATA_LENGTH_ERROR       (-1)
#define  DATA_VERSION_ERROR      (-2)
#define  DATA_ALLOC_ERROR        (-3)
#define  DATA_FORMAT_ERROR       (-4)
#define  DATA_NULL_ERROR         (-5)
#define  DATA_XML_WRITE_ERROR    (-6)
#define  DATA_XML_PARSE_ERROR    (-7)
#define  DATA_ALIGN_ERROR        (-8)
#define  DATA_CONVERT_ERROR      (-9)
#define  DATA_BUFFER_OVERFLOW    (-10) //not an error need to increase buffer size

#define  SERIALIZED_POINTER      ((void*)0x87654321)
#define  STANDARD_SERIALIZER     DATATYPE_nStandardSerializer
#define  STANDARD_DESERIALIZER   DATATYPE_nStandardDeserializer
#define  STANDARD_LENGTH         DATATYPE_nStandardSerializedLength
#define  STANDARD_DEALLOCATOR    DATATYPE_nStandardDeallocator
#define  STANDARD_ALIGNMENT      DATATYPE_nStandardAlignment
#define  STANDARD_XML_WRITER     DATATYPE_nStandardXmlWriter
#define  STANDARD_XML_READER     DATATYPE_nStandardXmlReader

#define  G_DF_UNSIGNED64_SIZE      sizeof(U64)
#define  G_DF_SIGNED64_SIZE        sizeof(S64)
#define  G_DF_DOUBLE_SIZE          sizeof(double)
#define  G_DF_UNSIGNED32_SIZE      sizeof(U32)
#define  G_DF_SIGNED32_SIZE        sizeof(S32)
#define  G_DF_FLOAT_SIZE           sizeof(float)
#define  G_DF_VOID_POINTER_SIZE    sizeof(void*)
#define  G_DF_BOOLEAN_SIZE         sizeof(BOOLEAN)
#define  G_DF_UNSIGNED16_SIZE      sizeof(U16)
#define  G_DF_SIGNED16_SIZE        sizeof(S16)
#define  G_DF_UNSIGNED8_SIZE       sizeof(U8)
#define  G_DF_SIGNED8_SIZE         sizeof(S8)
#define  G_DF_STRING_SIZE          (4*sizeof(C_STRING))                //neeed a minimul of null character (but round up to 4)
#define  G_DF_WSTRING_SIZE         (2*sizeof(C_WSTRING))               //1 wchar + 1 wide null
#define  G_DF_BUFFER_SIZE          sizeof(C_BUFFER)
#define  DYNAMIC_ARRAY_SIZE        sizeof(DYNAMIC_ARRAY)
#define  G_DF_UNDEFINED_SIZE       0

#ifdef __RTXC__
   #define G_DF_STD_FLOAT       G_DF_FLOAT
#else
   #define G_DF_STD_FLOAT       G_DF_DOUBLE
#endif

/********************               MACROS                   ******************/

/**
 * declare a basic/primitive database data type (has no sub types)
 */
#define  DT_BASIC_TYPE(scope, type, format, unit) \
         scope const G_DATA_TYPE type[1] = {{#type, format, unit, 0, NULL, NULL, \
         format##_SIZE, 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, STANDARD_DEALLOCATOR, \
         STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, NULL}}

/**
 * declare an enumerated type
 */
#define  DT_ENUM_TYPE(scope, type, size, xml_names) \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_ENUM, G_DU_NONE, 0, NULL, NULL, \
         size, 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, STANDARD_DEALLOCATOR, STANDARD_ALIGNMENT, \
         STANDARD_XML_WRITER, STANDARD_XML_READER, xml_names, NULL}}

/**
 * declare a string type
 */
#define DT_STRING_TYPE(scope, type) \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_STRING, G_DU_NONE, 0, NULL, NULL, \
         G_DF_STRING_SIZE, 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, STANDARD_DEALLOCATOR, \
         STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, NULL}}

/**
 * declare a wide string type
 */
#define DT_WSTRING_TYPE(scope, type) \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_WSTRING, G_DU_NONE, 0, NULL, NULL, \
         G_DF_WSTRING_SIZE, 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, STANDARD_DEALLOCATOR, \
         STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, NULL}}

/**
 * declare a buffer type
 *
 * @see C_BUFFER
 */
#define DT_BUFFER_TYPE(scope, type) \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_BUFFER, G_DU_NONE, 0, NULL, NULL, \
         G_DF_BUFFER_SIZE, 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, STANDARD_DEALLOCATOR, \
         STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, NULL}}

/**
 * declare a fixed array type
 */
#define  DT_ARRAY_TYPE(scope, type, num_elements, size_of_element, element_type) \
         scope const G_DATA_TYPE *type##_MEMBER_LIST[] = {element_type, NULL}; \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_FIXED_ARRAY, G_DU_NONE, 0, NULL, NULL, \
         num_elements*size_of_element, 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, STANDARD_DEALLOCATOR, \
         STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, type##_MEMBER_LIST}}

/**
 * declare a dynamic array type
 *
 * @see DT_DYNAMIC_ARRAY
 */
#define  DT_DYNAMIC_ARRAY_TYPE(scope, type, element_type) \
         scope const G_DATA_TYPE *type##_MEMBER_LIST[] = {element_type, NULL}; \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_DYNAMIC_ARRAY, G_DU_NONE, 0, NULL, NULL, \
         DYNAMIC_ARRAY_SIZE, 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, STANDARD_DEALLOCATOR, \
         STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, type##_MEMBER_LIST}}

/**
 * declare a null terminated ptr array of a type (element_type must be a ptr
 * type), within a struct it is a double ptr to a type
 */
#define  DT_NULL_TERM_PTR_ARRAY_TYPE(scope, type, ptr_type) \
         scope const G_DATA_TYPE *type##_MEMBER_LIST[] = {ptr_type, NULL}; \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_NULL_TERM_PTR_ARRAY, G_DU_NONE, 0, NULL, NULL, \
         sizeof(void**), 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, STANDARD_DEALLOCATOR, \
         STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, type##_MEMBER_LIST}}

/**
 * declare opaque data type
 */
#define  DT_OPAQUE_TYPE(scope, type, size, align) \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_OPAQUE, G_DU_NONE, 0, NULL, NULL, \
         size, align, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, \
         STANDARD_DEALLOCATOR, STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, NULL}}

/**
 * declare data to exclude from serialization
 */
#define  DT_EXCLUDED_TYPE(scope, type, size, align) \
      scope const G_DATA_TYPE type[1] = {{#type, G_DF_EXCLUDED, G_DU_NONE, 0, NULL, NULL, \
      size, align, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, \
      STANDARD_DEALLOCATOR, STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, NULL}}

/**
 * declare a structure-based data type
 */
#define  DT_STRUCT_TYPE(scope, type, version, prev_ver_type, ver_convert, size, xml_names, members ...) \
         scope const G_DATA_TYPE *type##_MEMBER_LIST[] = {members, NULL}; \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_STRUCT, G_DU_NONE, version, prev_ver_type, ver_convert, \
         size, 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, \
         STANDARD_DEALLOCATOR, STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, xml_names, type##_MEMBER_LIST}}

/**
 * declare a custom data type with custom serialize function (standard
 * serializer, etc. are preferred)
 */
#define  DT_CUSTOM_TYPE(scope, type, version, prev_ver_type, ver_convert, size, xml_names, serializer, \
         deserializer, serialized_length, deallocator, alignment, xml_writer, xml_reader) \
         scope const G_DATA_TYPE type[1] = {{#type, G_DF_CUSTOM, G_DU_NONE, version, prev_ver_type, ver_convert, \
         size, 0, serializer, deserializer, serialized_length, deallocator, alignment, \
         xml_writer, xml_reader, xml_names, NULL}}

/**
 * declare a pointer to any previously defined data type
 */
#define  DT_PTR_TO_TYPE(scope, ptr_type, type) \
         scope const G_DATA_TYPE *ptr_type##_MEMBER_LIST[] = {type, NULL}; \
         scope const G_DATA_TYPE ptr_type[1] = {{#ptr_type, G_DF_POINTER_TO_TYPE, G_DU_NONE, \
         0, NULL, NULL, sizeof(void*), 0, STANDARD_SERIALIZER, STANDARD_DESERIALIZER, STANDARD_LENGTH, \
         STANDARD_DEALLOCATOR, STANDARD_ALIGNMENT, STANDARD_XML_WRITER, STANDARD_XML_READER, NULL, ptr_type##_MEMBER_LIST}}

/**
 * declare an external reference to a data type
 */
#define  DT_EXTERN_TYPE(type) \
         extern const G_DATA_TYPE type[]

//functions to calculate size of string and buffers
#define  ROUND_UP_U32_BOUNDARY(x)         (((x) + 0x03) & (~0x03))
#define  STRING_SIZE(str)                 (strlen(str)+1)
#define  WSTRING_SIZE(wstr)               (sizeof(U32)*(wcslen(wstr)+1))
#define  BUFFER_SIZE(len)                 (sizeof(U32)+(len))

/********************         TYPE DEFINITIONS               ******************/
typedef void GXML_WRITER;
typedef void GXML_READER;

typedef struct
{
  int    nSize;
  void  *pData;
} C_BUFFER;

typedef char    C_STRING;
typedef wchar_t C_WSTRING;

typedef struct
{
   int   nMembers;
   void *pData;
} DYNAMIC_ARRAY;

typedef enum
{
   G_DF_UNDEFINED,           // unknown/don't care format
   G_DF_UNSIGNED64,          // 64-bit unsigned
   G_DF_SIGNED64,            // 64-bit signed
   G_DF_DOUBLE,              // double precision float
   G_DF_UNSIGNED32,          // 32-bit unsigned
   G_DF_SIGNED32,            // 32-bit signed
   G_DF_ENUM,                // enumerated
   G_DF_FLOAT,               // single precision float
   G_DF_VOID_POINTER,        // pointers/function pointers, shared memory (BE CAREFUL!)
   G_DF_BOOLEAN,             // boolean
   G_DF_UNSIGNED16,          // 16-bit unsigned
   G_DF_SIGNED16,            // 16-bit signed
   G_DF_UNSIGNED8,           // 8-bit unsigned
   G_DF_SIGNED8,             // 8-bit signed
   G_DF_STRING,              // string
   G_DF_WSTRING,             // wide string
   G_DF_BUFFER,              // buffer
   G_DF_OPAQUE,              // opaque
   G_DF_EXCLUDED,            // data excluded from serialization
   G_DF_FIXED_ARRAY,         // array, fixed size
   G_DF_DYNAMIC_ARRAY,       // array, dynamic size
   G_DF_NULL_TERM_PTR_ARRAY, // ptr array, null-terminated
   G_DF_POINTER_TO_TYPE,     // pointer to a type
   G_DF_STRUCT,              // struct
   G_DF_CUSTOM,              // custom
   G_DATA_FORMAT_COUNT
} G_DATA_FORMAT;

typedef enum
{
   G_DD_SCALAR,
   G_DD_DISTANCE_DEPTH_ALTITUDE,
   G_DD_SPEED,
   G_DD_TIME,
   G_DD_PRESSURE,
   G_DD_VOLUME,
   G_DD_POTENTIAL,
   G_DD_CURRENT,
   G_DD_TEMPERATURE,
   G_DD_HEADING,
   G_DD_RATEOFTURN,
   G_DD_LATITUDE,
   G_DD_LONGITUDE,
   G_DATA_DIMENSION_COUNT
} G_DATA_DIMENSION;

typedef enum
{
   G_DU_NONE,

   //distance/depth/altitude
   G_DU_METERS,
   G_DU_STAT_MILES,
   G_DU_NAUT_MILES,
   G_DU_KM,
   G_DU_INCH,
   G_DU_FEET,
   G_DU_FATHOMS,

   //speed
   G_DU_MPS,
   G_DU_KNOTS,
   G_DU_MPH,
   G_DU_KPH,

   //time
   G_DU_SECONDS,
   G_DU_MINUTES,
   G_DU_HOURS,
   G_DU_DAYS,

   //pressure
   G_DU_HECTOPASCALS,
   G_DU_MILLIBAR,
   G_DU_INCHESHG,
   G_DU_MMHG,

   //volume
   G_DU_M3,
   G_DU_LITERS,
   G_DU_USGALLONS,
   G_DU_UKGALLONS,

   //electric potential
   G_DU_VOLTS,

   //electric current
   G_DU_AMPERES,
   G_DU_MILLIAMPERES,

   //temperature
   G_DU_CELSIUS,
   G_DU_FAHRENHEIT,

   //heading
   G_DU_DEG_TRUE,

   //rate of turn
   G_DU_DEG_PER_MINUTE,

   //latitude
   G_DU_DEG_LATITUDE,

   //longitude
   G_DU_DEG_LONGITUDE,

   G_DATA_UNIT_COUNT
} G_DATA_UNIT;

typedef struct
{
   double dConversionRatio;
   double dConversionOffset;
} DATA_UNIT_CONVERSION_INFO;

typedef enum
{
   ALIGN_MEMORY,
   ALIGN_STRUCT
} ALIGN_TYPE;

//tricky circular definition
struct __tag_G_DATA_TYPE;

typedef int (*DATA_SERIALIZE_FP)(void *pvStruct, void **ppvSerialized, int *pnRemaining,
                                 int *pnSize, const struct __tag_G_DATA_TYPE *pDataType);
typedef int (*DATA_DESERIALIZE_FP)(void **ppvStruct, void *pvSerialized, int *pnRemaining,
                                   int *pnSize, const struct __tag_G_DATA_TYPE *pDataType);
typedef int (*DATA_LENGTH_FP)(void *pvStruct, const struct __tag_G_DATA_TYPE *pDataType);
typedef int (*DATA_DEALLOCATE_FP)(void *pvStruct, BOOLEAN bTypeAllocated,
                                  const struct __tag_G_DATA_TYPE *pDataType);
typedef int (*DATA_ALIGNMENT_FP)(ALIGN_TYPE eAlignType, const struct __tag_G_DATA_TYPE *pDataType);
typedef int (*DATA_XML_WRITE_FP)(void *pvStruct, GXML_WRITER *pWriter, const char *pcName,
                                 const struct __tag_G_DATA_TYPE *pDataType);
typedef int (*DATA_XML_READ_FP)(void **ppvStruct, GXML_READER *pReader,
                                const struct __tag_G_DATA_TYPE *pDataType);
typedef int (*DATA_VERSION_CONVERT_FP)(void *pvStructOut, void *pvStructIn,
                                       const struct __tag_G_DATA_TYPE *pDataTypeOut,
                                       const struct __tag_G_DATA_TYPE *pDataTypeIn);

typedef struct __tag_G_DATA_TYPE
{
   const char                    *  pTypeId;
   G_DATA_FORMAT                    eDataFormat;
   G_DATA_UNIT                      eDataUnit;
   int                              nVersion;
   const struct __tag_G_DATA_TYPE*  pPreviousVersionType;
   DATA_VERSION_CONVERT_FP          pfVersionConvert;
   int                              nSizeofType;
   int                              nAlignment;
   DATA_SERIALIZE_FP                pfSerialize;
   DATA_DESERIALIZE_FP              pfDeserialize;
   DATA_LENGTH_FP                   pfLength;
   DATA_DEALLOCATE_FP               pfDeallocate;
   DATA_ALIGNMENT_FP                pfAlignment;
   DATA_XML_WRITE_FP                pfXmlWrite;
   DATA_XML_READ_FP                 pfXmlRead;
   const char                   **  pcXmlNames;
   const struct __tag_G_DATA_TYPE **  DataTypes;
} G_DATA_TYPE;

/********************        FUNCTION PROTOTYPES             ******************/

extern int     DATATYPE_nStandardSerializer(void *pvStruct, void **ppvSerialized, int *pnRemaining, int *pnSize, const G_DATA_TYPE *pDataType);
extern int     DATATYPE_nStandardDeserializer(void **ppvStruct, void *pvSerialized, int *pnRemaining, int *pnSize, const G_DATA_TYPE *pDataType);
extern int     DATATYPE_nStandardSerializedLength(void *pvStruct, const G_DATA_TYPE *pDataType);
extern int     DATATYPE_nStandardDeallocator(void *pvStruct, BOOLEAN bTypeAllocated, const G_DATA_TYPE *pDataType);
extern int     DATATYPE_nStandardAlignment(ALIGN_TYPE eAlignType, const G_DATA_TYPE *pDataType);
extern int     DATATYPE_nStandardXmlWriter(void *pvStruct, GXML_WRITER *pWriter, const char *pcName, const G_DATA_TYPE *pDataType);
extern int     DATATYPE_nStandardXmlReader(void **ppvStruct, GXML_READER *pReader, const G_DATA_TYPE *pDataType);

extern BOOLEAN DATATYPE_bCompatibleTypes(const G_DATA_TYPE *pDataType1, const G_DATA_TYPE *pDataType2);

extern double  DATATYPE_dConvertUnitToUnit(double dValue, G_DATA_UNIT eUnitFrom, G_DATA_UNIT eUnitTo);
extern double  DATATYPE_dConvertDataToUnit(double dValue, const G_DATA_TYPE *pDataTypeFrom, G_DATA_UNIT eUnitTo);
extern double  DATATYPE_dConvertUnitToData(double dValue, G_DATA_UNIT eUnitFrom, const G_DATA_TYPE *pDataTypeTo);

/********************          LOCAL VARIABLES               ******************/

/********************          GLOBAL VARIABLES              ******************/
/* Basic Data Types */
DT_EXTERN_TYPE(DT_DEFAULT);
DT_EXTERN_TYPE(DT_UNSIGNED64);
DT_EXTERN_TYPE(DT_UNSIGNED32);
DT_EXTERN_TYPE(DT_UNSIGNED16);
DT_EXTERN_TYPE(DT_UNSIGNED8);
DT_EXTERN_TYPE(DT_SIGNED64);
DT_EXTERN_TYPE(DT_SIGNED32);
DT_EXTERN_TYPE(DT_SIGNED16);
DT_EXTERN_TYPE(DT_SIGNED8);
DT_EXTERN_TYPE(DT_DOUBLE);
DT_EXTERN_TYPE(DT_FLOAT);
DT_EXTERN_TYPE(DT_POINTER);
DT_EXTERN_TYPE(DT_BOOLEAN);
DT_EXTERN_TYPE(DT_STRING);
DT_EXTERN_TYPE(DT_WSTRING);
DT_EXTERN_TYPE(DT_BUFFER);

/* pointers to types (may be null) */
DT_EXTERN_TYPE(DT_PTR_TO_UNSIGNED64);
DT_EXTERN_TYPE(DT_PTR_TO_UNSIGNED32);
DT_EXTERN_TYPE(DT_PTR_TO_UNSIGNED16);
DT_EXTERN_TYPE(DT_PTR_TO_UNSIGNED8);
DT_EXTERN_TYPE(DT_PTR_TO_SIGNED64);
DT_EXTERN_TYPE(DT_PTR_TO_SIGNED32);
DT_EXTERN_TYPE(DT_PTR_TO_SIGNED16);
DT_EXTERN_TYPE(DT_PTR_TO_SIGNED8);
DT_EXTERN_TYPE(DT_PTR_TO_DOUBLE);
DT_EXTERN_TYPE(DT_PTR_TO_FLOAT);
DT_EXTERN_TYPE(DT_PTR_TO_POINTER);
DT_EXTERN_TYPE(DT_PTR_TO_BOOLEAN);
DT_EXTERN_TYPE(DT_PTR_TO_STRING);
DT_EXTERN_TYPE(DT_PTR_TO_WSTRING);
DT_EXTERN_TYPE(DT_PTR_TO_BUFFER);

/* basic dimensional types */
DT_EXTERN_TYPE(DT_DISTANCE);           //double in meters
DT_EXTERN_TYPE(DT_DEPTH);              //double in meters
DT_EXTERN_TYPE(DT_ALTITUDE);           //double in meters
DT_EXTERN_TYPE(DT_SPEED);              //double in mps
DT_EXTERN_TYPE(DT_TIME);               //double in seconds
DT_EXTERN_TYPE(DT_DATETIME);           //U32    in seconds
DT_EXTERN_TYPE(DT_PRESSURE);           //double in hectopascals
DT_EXTERN_TYPE(DT_VOLUME);             //double in cubic meters
DT_EXTERN_TYPE(DT_POTENTIAL);          //double in volts
DT_EXTERN_TYPE(DT_CURRENT);            //double in amperes
DT_EXTERN_TYPE(DT_TEMPERATURE);        //double in degrees celsius
DT_EXTERN_TYPE(DT_HEADING);            //double in degrees true
DT_EXTERN_TYPE(DT_RATEOFTURN);         //double in degrees per minute (positive means bow turns to starboard)
DT_EXTERN_TYPE(DT_DIRECTION_OF_TURN);  //enum
DT_EXTERN_TYPE(DT_MAGVAR);             //double in degrees true (add this to the degrees true heading to get magnetic heading)
DT_EXTERN_TYPE(DT_LATITUDE);           //double in degrees latitude
DT_EXTERN_TYPE(DT_LONGITUDE);          //double in degrees longitude

//Circular headers prevented this from being in gtime.h/gtime.c
extern const char * s_cGTimeXmlNames[];
DT_EXTERN_TYPE(DT_GTIME);         //GTIME Representation of Date/Time

extern const char * const gDATATYPE_DataFormatString[];
extern const char * const gDATATYPE_DataDimensionString[];
extern const char * const gDATATYPE_DataUnitString[];
extern const G_DATA_UNIT   gDATATYPE_BaseDataUnit[];

/********************              FUNCTIONS                 ******************/

#ifdef __cplusplus
   }
#endif
#endif      // DATATYPE_H
