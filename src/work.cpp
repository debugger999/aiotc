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

#include "platform.h"
#include "pids.h"
#include "work.h"
#include "shm.h"

static void *output_thread(void *arg) {
    pidOps *pOps = (pidOps *)arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;

    while(pAiotcParams->running && pOps->running) {
        sleep(1);
    }

    app_debug("run over");

    return NULL;
}

int workProcInit(void *arg) {
    taskOps *pTaskOps;
    outParams *pOutParams;
    workParams *pWorkParams;
    pidOps *pOps = (pidOps *)arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    pOps->procTaskOps = shmMalloc(pShmParams->headsp, sizeof(taskOps));
    if(pOps->procTaskOps == NULL) {
        app_err("shm malloc failed");
        return -1;
    }
    memset(pOps->procTaskOps, 0, sizeof(taskOps));
    pTaskOps = (taskOps *)pOps->procTaskOps;
    pTaskOps->arg = shmMalloc(pShmParams->headsp, sizeof(workParams));
    if(pTaskOps->arg == NULL) {
        app_err("shm malloc failed");
        shmFree(pShmParams->headsp, pOps->procTaskOps);
        return -1;
    }
    pWorkParams = (workParams *)pTaskOps->arg;
    pOutParams = (outParams *)&pWorkParams->outParam;

    if(sem_init(&pOutParams->mutex_out, 1, 1) < 0) {
        app_err("sem_init failed");
        shmFree(pShmParams->headsp, pOps->procTaskOps);
        shmFree(pShmParams->headsp, pTaskOps->arg);
        pTaskOps->arg = NULL;
        return -1;
    }

    return 0;
}

static int workInit(pidOps *pOps) {
    pthread_t pid;

    if(pthread_create(&pid, NULL, output_thread, pOps) != 0) {
        app_err("create output thread failed");
    }
    else {
        pthread_detach(pid);
    }

    return 0;
}

int workProcess(void *arg) {
    pidOps *pOps = (pidOps *)arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;

    pOps = getRealOps(pOps);
    if(pOps == NULL) {
        return -1;
    }
    pOps->running = 1;

    workInit(pOps);

    while(pAiotcParams->running && pOps->running) {
        sleep(2);
    }
    pOps->running = 0;

    app_debug("pid:%d, run over", pOps->pid);

    return 0;
}

