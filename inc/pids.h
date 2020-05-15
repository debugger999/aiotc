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

#ifndef __AIOTC_PIDS_H__
#define __AIOTC_PIDS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROC_LOAD_MAX       90
#define PROC_HEART_TIMEOUT  10

typedef int (*ProcFunc)(void *arg);

typedef struct {
    char        name[32];
    char        subName[32];
    ProcFunc    proc;
    pid_t       pid;
    int         load;
    int         lastReboot;
    void        *arg; // aiotcParams
} PidOps;

PidOps *getOpsByPid(pid_t pid, void *arg);
PidOps *getOpsByName(const char *name );
int put2PidQueue(PidOps *pOps, void *arg);

typedef struct {
    sem_t mutex_pid;
    queue_common pidQueue; // PidOps
    void *arg; // aiotcParams
} pidsParams;

#endif
