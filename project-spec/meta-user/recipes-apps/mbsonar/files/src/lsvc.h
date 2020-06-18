#include "types.h"
#include "l_task.h"
#include "l_queue.h"

#define NMBOXES      7
#define M_PING       1
#define M_GMSG       2
#define M_SUDPOUT    3
#define M_SLPD       4
#define M_NVRAM      5
#define M_INLINETEST 6
#define M_SW_UPDATE  7

#define NSEMAS                  13 
#define SEMA_MBOX_PING          1
#define SEMA_MBOX_GMSG          2
#define SEMA_MBOX_SLPD          3
#define SEMA_MBOX_SUDPOUT       4
#define SEMA_MBOX_NVRAM         5
#define SEMA_MBOX_INLINETEST    6
#define SEMA_MBOX_SW_UPDATE     7
#define SEMA_PING_QNE           8
#define SEMA_GMSG_SELECT        9
#define SEMA_GMSG_PING          10 
#define SEMA_TIMER_GMSG         11
#define SEMA_NVRAM_UPDATE       12
#define SEMA_TIMER_MB_WET_STATE 13

#define NTASKS         8
#define PINGTASK       1
#define GMSGTASK       2
#define GMSGSOCKETTASK 3
#define SLPDTASK       4
#define SUDPOUTTASK    5
#define NVRAMTASK      6
#define INLINETESTTASK 7
#define SWUPDATETASK   8

#define NTMRS 75

#define NQUEUES 1
#define Q_PING  1  /* Ping Descriptor Q */

#define NRES 4
#define RES_DATABASE       1  /* Database storage */
#define RES_PING           2  /* */
#define RES_GMSG_MBOX_LIST 3  /* */
#define RES_SONAR_SHARE    4

extern const U32 nmboxes;
extern const U32 nsemas;
extern const U32 ntmrs;
extern const U32 nqueues;
extern const U32 nres;
extern const U32 ntasks;

extern const char *str_tasks[];
extern const char *str_mailboxes[];
extern const char *str_semaphores[];
extern const char *str_queues[];
extern const char *str_mutexes[];

extern const TASK_PARAM TakParams[];
extern const QUEUE_PARAM QueueParams[]; 
