#include "types.h"
#include "lsvc.h"

const char *str_semaphores[] = 
{
   "",
   "SEMA_MBOX_PING",
   "SEMA_MBOX_GMSG",
   "SEMA_MBOX_SLPD",
   "SEMA_MBOX_SUDPOUT",
   "SEMA_MBOX_NVRAM",
   "SEMA_MBOX_INLINETEST",
   "SEMA_MBOX_SW_UPDATE",
   "SEMA_PING_QNE",
   "SEMA_GMSG_SELECT",
   "SEMA_GMSG_PING",
   "SEMA_TIMER_GMSG",
   "SEMA_NVRAM_UPDATE",
   "SEMA_TIMER_MB_WET_STATE",
};

const char *str_queues[] =
{
   "",
   "Q_PING"
};

const char *str_mutexes[] =
{
   "",
   "RES_DATABASE",
   "RES_PING",
   "RES_GMSG_MBOX_LIST",
   "RES_SONAR_SHARE",
};

const char *str_tasks[] =
{
   "",
   "PINGTASK",
   "GMSGTASK",
   "GMSGSOCKETTASK",
   "SLPDTASK",
   "SUDPOUTTASK",
   "NVRAMTASK",
   "INLINETESTTASK",
   "SWUPDATETASK",
};

const char *str_mailboxes[] = 
{
   "",
   "M_PING",
   "M_GMSG",
   "M_SLPD",
   "M_SUDPOUT",
   "M_NVRAM",
   "M_INLINETEST",
   "M_SW_UPDATE",
};

// stack sizes and start functions from ctask.c
#define STKSZ1 4096   /* PINGTASK */
#define STKSZ2 4096   /* GMSGTASK */
#define STKSZ3 4096   /* GMSGSOCKETTASK */
#define STKSZ4 16384  /* SLPDTASK */
#define STKSZ5 2048   /* SUDPOUTTASK */
#define STKSZ6 1024   /* NVRAMTASK */
#define STKSZ7 4096   /* INLINETESTTASK */
#define STKSZ8 4096   /* SWUPDATETASK */

extern void T_GMSG_vMain(void);         /* GMSGTASK */
extern void T_PING_vMain(void);         /* PINGTASK */
extern void T_SLPD_vMain(void);         /* SLPDTASK */
extern void T_GMSG_vSocketMain(void);   /* GMSGSOCKETTASK */
extern void T_SUDP_OUT_vMain(void);     /* SUDPOUTTASK */
extern void T_NVRAM_vMain(void);        /* NVRAMTASK */
extern void T_INLINETEST_vMain(void);   /* INLINETESTTASK */
extern void T_SW_UPDATE_vMain(void);    /* SWUPDATETASK */

// task parameters
const TASK_PARAM TaskParams[] = {
  {STKSZ1, 100, &T_PING_vMain},
  {STKSZ2,  90, &T_GMSG_vMain},
  {STKSZ3,  90, &T_GMSG_vSocketMain},
  {STKSZ4,  90, &T_SLPD_vMain},
  {STKSZ5,  90, &T_SUDP_OUT_vMain},
  {STKSZ6,  90, &T_NVRAM_vMain},
  {STKSZ7, 100, &T_INLINETEST_vMain},
  {STKSZ8, 100, &T_SW_UPDATE_vMain},
};

#define Q1WD 2L     /* Q_PING */
#define Q1DP 64L

// queue parameters
const QUEUE_PARAM QueueParams[] = 
{
  {Q1WD, Q1DP}
};

const U32 nmboxes = NMBOXES;
const U32 nsemas = NSEMAS;
const U32 ntmrs = NTMRS;
const U32 nqueues = NQUEUES;
const U32 nres = NRES;
const U32 ntasks = NTASKS;
