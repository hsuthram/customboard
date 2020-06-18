#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u_log.h"
#include "u_ping.h"
/******************************************************************************
 *
 *    Function: U_PING_vAddClientNode
 *
 *    Args:     head of the linked list
 *              ip address to be added to list
 *
 *    Return:   -none-
 *              but the list will get modified
 *
 *    Purpose:  adds a new client to the sonar sources link ed list
 *              there are 4 lists: 2D, 360, SI, and DI
 *              The 360 and SI list is excluisve to the 360 (so the control head
 *              list for these two sources will always be NULL).
 *              The 2D and DI list is exclusive to the control head.  And
 *              the DI list really means the "Advanced" list.  It holds
 *              the registed client list of the non-2D element.  This could be
 *              an SI, DI or quad beam element.
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void U_PING_vAddClientNode(struct sonarRegisteredClientNode **p, U32 u32ClientAddress)
{
   struct sonarRegisteredClientNode *newClient;

   newClient = malloc(sizeof(struct sonarRegisteredClientNode));

   if (newClient != NULL)
   {
      newClient->next = NULL;
      newClient->u32ClientAddress = u32ClientAddress;
   }
   else
   {
      U_PrintLog("Cannot allocate memory");
   }
   
   if (*p == NULL)  //first entry
   {
      *p = newClient;   
   }
   else
   {
      struct sonarRegisteredClientNode *tmp = *p;
      BOOLEAN                           bFound = FALSE;
      int                               i =  0;
      
      while (tmp->next != NULL)
      {
         printf("[%d]%s: node[%d] address [%08X] new client address [%08X]\n", __LINE__, __FUNCTION__, i, tmp->u32ClientAddress, u32ClientAddress);
         
         if (tmp->u32ClientAddress == u32ClientAddress)
         {
            bFound = TRUE;
            printf("[%d]%s: client at %08X already registered\n", __LINE__, __FUNCTION__, u32ClientAddress);
            break;
         }
         else
         {
            tmp = tmp->next;
            i++;
         }
      }
      
      if (!bFound)  //add to the end
      {
         tmp->next = newClient;
         
#if 1
         {
            struct sonarRegisteredClientNode *tmp1 = *p;
      
            while (tmp1 != NULL) 
            {
               printf("[%d]%s: (add node) whole list %08X\n", __LINE__, __FUNCTION__, tmp1->u32ClientAddress);
               tmp1 = tmp1->next;
            }
         }
#endif
      }
   }
}

/******************************************************************************
 *
 *    Function: U_PING_vDeleteClientNode
 *
 *    Args:     head of the linked list
 *              ip address to be deleted from list
 *
 *    Return:   -none-
 *              but the list will get modified
 *
 *    Purpose:  adds a new client to the sonar sources link ed list
 *              there are 4 lists: 2D, 360, SI, and DI
 *              The 360 and SI list is excluisve to the 360 (so the control head
 *              list for these two sources will always be NULL).
 *              The 2D and DI list is exclusive to the control head.  And
 *              the DI list really means the "Advanced" list.  It holds
 *              the registed client list of the non-2D element.  This could be
 *              an SI, DI or quad beam element.
 *
 *    Notes:
 *
 ******************************************************************************/
GLOBAL void U_PING_vDeleteClientNode(struct sonarRegisteredClientNode **p, U32 u32ClientAddress)
{
   struct sonarRegisteredClientNode *tmp = *p;

   if (tmp == NULL)
   {
      printf("[%d]%s: empty list for sonar source, cannot delete address %08X\n", __LINE__, __FUNCTION__, u32ClientAddress);
      return;
   }

   if ((tmp->next == NULL) && (tmp->u32ClientAddress == u32ClientAddress))  //only 1 client in the list
   {
      printf("[%d]%s: deleted address %08X (only member in list) for sonar source\n", __LINE__, __FUNCTION__, u32ClientAddress);
      free(tmp);
      *p = NULL;
      return;
   }

   if (tmp->u32ClientAddress == u32ClientAddress)  //first client in the list
   {
      printf("[%d]%s: deleted address %08X (first member in list) for sonar source\n", __LINE__, __FUNCTION__, u32ClientAddress);
      *p = tmp->next;
      free(tmp);
   }
   else
   {
      while (tmp->next != NULL)
      {
         if (tmp->next->u32ClientAddress == u32ClientAddress)
         {
            struct sonarRegisteredClientNode *toDelete = tmp->next;

            tmp->next = toDelete->next;
            free(toDelete);
            printf("[%d]%s: deleted address %08X for sonar source\n", __LINE__, __FUNCTION__, u32ClientAddress);
            break;
         }
         else
         {
            tmp = tmp->next;
         }
      }
   }
}
