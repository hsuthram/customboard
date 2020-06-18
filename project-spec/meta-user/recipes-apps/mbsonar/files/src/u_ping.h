#ifndef U_PING_H
#define U_PING_H

#include "types.h"
/********************              DEFINES                   ******************/
#define TOTAL_SONAR_SOURCES 5    //2D, DI, SI, 360, MBSonar
#define SONAR_MSG_TIMEOUT   500  // half second
/********************         TYPE DEFINITIONS               ******************/
struct sonarRegisteredClientNode
{
   U32 u32ClientAddress;
   struct sonarRegisteredClientNode *next;
};
/********************        FUNCTION PROTOTYPES             ******************/
extern void U_PING_vAddClientNode(struct sonarRegisteredClientNode **p, U32 u32ClientAddress);
extern void U_PING_vDeleteClientNode(struct sonarRegisteredClientNode **p, U32 u32ClientAddress);
#endif


