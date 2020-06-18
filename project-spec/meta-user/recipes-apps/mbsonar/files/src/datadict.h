#ifndef DATADICT_H
#define DATADICT_H

#include "fcid.h"
/********************               MACROS                   ******************/
#define MAKE_FOURCHAR_ID(a,b,c,d) ((FOURCHARID)((((d)*256L+(c))*256L+(b))*256L+(a)))

typedef enum
{
   DF_ERROR = 0, /* Shouldn't ever get this! */
   DF_FLOAT,
   DF_UNSIGNED32,
   DF_SIGNED32,
//   DF_VOID_POINTER,
//   DF_ENUMERATED
} DATA_FORMAT;

#endif
