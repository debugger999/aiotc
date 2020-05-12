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

#ifndef __AIOTC_MASTER_H__
#define __AIOTC_MASTER_H__

#include "platform.h"
#include "share.h"

#define MASTER_OBJ_MAX  10000
#define SLAVE_LOAD_MAX  90

typedef struct {
    char ip[32];
    char internetIp[32];
    int restPort;
    int streamPort;
    int systemInit;
    int load;
    int online;     // sec
    int offline;    // sec
} slaveParams;

typedef struct {
    int slaveLoadOk;
    sem_t mutex_slave;
    queue_common slaveQueue;
    sem_t mutex_mobj;
    queue_common mobjQueue;
    int running;
    void *arg; // aiotcParams
} masterParams;

int masterProcess(void *args);

#endif
