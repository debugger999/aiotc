/******************************************************************************
 * Copyright (C) 2019-2025 debugger999 <debugger999@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#ifndef __AIOTC_PLATFORM_H__
#define __AIOTC_PLATFORM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/sysinfo.h>
#include <ctype.h>
#include <execinfo.h>
#include "log.h"

#define AIOTC_VERSION               "V0.01.202004010001"
#define NGINX_PATH                  "/usr/local/nginx"
#define AIOTC_CFG                   "config.json"

typedef struct {
    char localIp[128];
    void *masterArgs;   // masterParams
    void *slaveArgs;    // slaveParams
    void *restArgs;     // restParams
    void *objArgs;      // objParams
    void *shmArgs;      // shmParams
    void *configArgs;   // configParams
    void *pidsArgs;     // pidsParams
    void *dbArgs;       // dbParams
    void *systemArgs;   // systemParams
    int running;
} aiotcParams;

#endif
