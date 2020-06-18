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

int rpmsg_create_ept(int rpfd, struct rpmsg_endpoint_info *eptinfo)
{
   int ret = ioctl(rpfd, RPMSG_CREATE_EPT_IOCTL, eptinfo);

   if (ret)
   {
      perror("Failed to create endpoint.\n");
   }

   return ret;
}

char *get_rpmsg_ept_dev_name(const char *rpmsg_char_name,
                             const char *ept_name,
                             char *ept_dev_name)
{
   char sys_rpmsg_ept_name_path[64];
   char svc_name[64];
   char *sys_rpmsg_path = "/sys/class/rpmsg";
   FILE *fp;
   int   i;
   int   ept_name_len;

   for (i = 0; i < 128; i++) 
   {
      sprintf(sys_rpmsg_ept_name_path, "%s/%s/rpmsg%d/name", sys_rpmsg_path, rpmsg_char_name, i);
      printf("checking %s\n", sys_rpmsg_ept_name_path);

      if (access(sys_rpmsg_ept_name_path, F_OK) < 0)
      {
         continue;
      }

      fp = fopen(sys_rpmsg_ept_name_path, "r");

      if (!fp) 
      {
         printf("failed to open %s\n", sys_rpmsg_ept_name_path);
         break;
      }

      fgets(svc_name, sizeof(svc_name), fp);
      fclose(fp);
      printf("svc_name: %s.\n",svc_name);
      ept_name_len = strlen(ept_name);

      if (ept_name_len > sizeof(svc_name))
      {
         ept_name_len = sizeof(svc_name);
      }

      if (!strncmp(svc_name, ept_name, ept_name_len)) 
      {
         sprintf(ept_dev_name, "rpmsg%d", i);
         return ept_dev_name;
      }
   }

   printf("Not able to RPMsg endpoint file for %s:%s.\n", rpmsg_char_name, ept_name);
   return NULL;
}

int bind_rpmsg_chrdev(const char *rpmsg_dev_name)
{
   char  fpath[256];
   char *rpmsg_chdrv = "rpmsg_chrdev";
   int   fd;
   int   ret;

   /* rpmsg dev overrides path */
   sprintf(fpath, "%s/devices/%s/driver_override", RPMSG_BUS_SYS, rpmsg_dev_name);
   fd = open(fpath, O_WRONLY);

   if (fd < 0) 
   {
      fprintf(stderr, "Failed to open %s, %s\n", fpath, strerror(errno));
      return -EINVAL;
   }

   ret = write(fd, rpmsg_chdrv, strlen(rpmsg_chdrv) + 1);

   if (ret < 0) 
   {
      fprintf(stderr, "Failed to write %s to %s, %s\n", rpmsg_chdrv, fpath, strerror(errno));
      return -EINVAL;
   }

   close(fd);

   /* bind the rpmsg device to rpmsg char driver */
   sprintf(fpath, "%s/drivers/%s/bind", RPMSG_BUS_SYS, rpmsg_chdrv);
   fd = open(fpath, O_WRONLY);

   if (fd < 0) 
   {
      fprintf(stderr, "Failed to open %s, %s\n", fpath, strerror(errno));
      return -EINVAL;
   }

   ret = write(fd, rpmsg_dev_name, strlen(rpmsg_dev_name) + 1);

   if (ret < 0) 
   {
      fprintf(stderr, "Failed to write %s to %s, %s\n", rpmsg_dev_name, fpath, strerror(errno));
      return -EINVAL;
   }

   close(fd);
   return 0;
}

int get_rpmsg_chrdev_fd(const char *rpmsg_dev_name,
                        char *rpmsg_ctrl_name)
{
   char           dpath[256];
   char           fpath[512];
   char          *rpmsg_ctrl_prefix = "rpmsg_ctrl";
   DIR           *dir;
   struct dirent *ent;
   int            fd;

   sprintf(dpath, "%s/devices/%s/rpmsg", RPMSG_BUS_SYS, rpmsg_dev_name);
   dir = opendir(dpath);

   if (dir == NULL) 
   {
      fprintf(stderr, "Failed to open dir %s\n", dpath);
      return -EINVAL;
   }

   while ((ent = readdir(dir)) != NULL) 
   {
      if (!strncmp(ent->d_name, rpmsg_ctrl_prefix, strlen(rpmsg_ctrl_prefix))) 
      {
         printf("Opening file %s.\n", ent->d_name);
         sprintf(fpath, "/dev/%s", ent->d_name);
         fd = open(fpath, O_RDWR | O_NONBLOCK);

         if (fd < 0) 
	 {
            fprintf(stderr, "Failed to open rpmsg char dev %s,%s\n", fpath, strerror(errno));
            return fd;
         }

         sprintf(rpmsg_ctrl_name, "%s", ent->d_name);
         return fd;
      }
   }

   fprintf(stderr, "No rpmsg char dev file is found\n");
   return -EINVAL;
}
