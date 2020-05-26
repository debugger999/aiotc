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
#include "share.h"
#include "task.h"

#define PROC_LOAD_MAX       90
#define PROC_BEAT_TIMEOUT   20

typedef int (*ProcFunc)(void *arg);

typedef struct {
    char            name[24];
    char            subName[24];
    char            taskName[24];
    ProcFunc        proc;
    pid_t           pid;
    key_t           msgKey;
    struct timeval  tv;
    sem_t           mutex_pobj;
    queue_common    pobjQueue;      // objParam
    void            *procTaskOps;   // taskOps
    int             load;
    int             lastReboot;
    int             running;
    void            *arg; // aiotcParams
} pidOps;

pidOps *getRealOps(pidOps *pOps);
pidOps *getOpsByPid(pid_t pid, void *arg);
pidOps *getOpsByName(const char *name, const char *subName, const char *taskName);
pidOps *getEmptyProc(const char *name, const char *subName, const char *taskName, void *arg);
pidOps *getTaskProc(const char *name, const char *subName, const char *taskName, void *arg);
int initTaskOps(pidOps *pOps, taskOps *pTaskOps);
int createBeatTask(pidOps *pOps, int (*obj_task_beat)(node_common *, void *), int sec);
int creatProcByPid(pid_t oldPid, int status, void *arg);
pid_t createProcess(const char *name, const char *subName, const char *taskName, void *arg);

typedef struct {
    sem_t mutex_pid;
    queue_common pidQueue; // pidOps
    void *arg; // aiotcParams
} pidsParams;

#endif
