#ifndef RPMSG_UTILITY_H
#define RPMSG_UTILITY_H

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <linux/rpmsg.h>

#define RPMSG_BUS_SYS "/sys/bus/rpmsg"

int   rpmsg_create_ept(int rpfd, struct rpmsg_endpoint_info *eptinfo);
int   bind_rpmsg_chrdev(const char *rpmsg_dev_name);
int   get_rpmsg_chrdev_fd(const char *rpmsg_dev_name,
                          char *rpmsg_ctrl_name);
char *get_rpmsg_ept_dev_name(const char *rpmsg_char_name,
                             const char *ept_name,
                             char *ept_dev_name);
#endif  //RPMSG_UTILITY_H
