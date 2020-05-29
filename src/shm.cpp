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
#include "shm.h"
#include "share.h"

static int initHeadShm(shmParams *pShmParams, int shmHeadSize) {
    pShmParams->headsp = (ncx_slab_pool_t *)pShmParams->shmAddr;
    pShmParams->headsp->addr = pShmParams->shmAddr;
    pShmParams->headsp->min_shift = 3;
    pShmParams->headsp->end = (u_char *)pShmParams->shmAddr + shmHeadSize;
    ncx_slab_init(pShmParams->headsp);
    pShmParams->headsp->mutex = (sem_t *)ncx_slab_alloc_locked(pShmParams->headsp, sizeof(sem_t)); 
    if(sem_init(pShmParams->headsp->mutex, 1, 1) < 0) {
        app_err("sem init failed");
        return -1;
    }

    return 0;
}

static int initShmSlab(shmParam *pShm, int poolSize, shmParams *pShmParams) {
    pShm->sp = (ncx_slab_pool_t *)pShm->shmAddr;
    pShm->sp->addr = pShm->shmAddr;
    pShm->sp->min_shift = 3;
    pShm->sp->end = (u_char *)pShm->shmAddr + poolSize;
    ncx_slab_init(pShm->sp);
    pShm->sp->mutex = (sem_t *)shmMalloc(pShmParams->headsp, sizeof(sem_t)); 
    if(sem_init(pShm->sp->mutex, 1, 1) < 0) {
        app_err("sem init failed");
        return -1;
    }

    pShm->queue = (queue_common *)shmMalloc(pShmParams->headsp, sizeof(queue_common));
    memset(pShm->queue, 0, sizeof(queue_common));
    pShm->mutex_shm = (sem_t *)shmMalloc(pShmParams->headsp, sizeof(sem_t)); 
    if(sem_init(pShm->mutex_shm, 1, 1) < 0) {
        app_err("sem init failed");
        return -1;
    }

    return 0;
}

int initShmArray(shmParams *pShmParams) {
    int i, j;
    long long int offset = 0;
    aiotcParams *pAiotcParams = (aiotcParams *)pShmParams->arg;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    int videoShmSize = pConfigParams->videoFrameSizeMax*pConfigParams->videoQueLen;
    int captureShmSize = pConfigParams->captureFrameSizeMax*pConfigParams->captureQueLen;

    pShmParams->num = pConfigParams->videoMax + pConfigParams->captureMax;
    pShmParams->pShmArray = (shmParam *)shmMalloc(pShmParams->headsp, sizeof(shmParam)*pShmParams->num);
    memset(pShmParams->pShmArray, 0, sizeof(shmParam)*pShmParams->num);

    offset += pConfigParams->shmHeadSize;
    for(i = 0; i < pConfigParams->videoMax; i ++) {
        shmParam *pShm = pShmParams->pShmArray + i;
        pShm->shmAddr = (char *)pShmParams->shmAddr + offset;
        initShmSlab(pShm, videoShmSize, pShmParams);
        strncpy(pShm->type, "video", sizeof(pShm->type));
        pShm->id = i+1;
        offset += videoShmSize;
    }
    for(j = 0; j < pConfigParams->captureMax; j ++, i ++) {
        shmParam *pShm = pShmParams->pShmArray + i;
        pShm->shmAddr = (char *)pShmParams->shmAddr + offset;
        initShmSlab(pShm, captureShmSize, pShmParams);
        strncpy(pShm->type, "capture", sizeof(pShm->type));
        pShm->id = i+1;
        offset += captureShmSize;
    }

    return 0;
}

int shmInit(configParams *pConfigParams, shmParams *pShmParams) {
    int alignSize = 1024*1024;

    pShmParams->shmTotalSize = (long long int)pConfigParams->shmHeadSize;
    if(pConfigParams->masterEnable == 0 || pConfigParams->masterEnable == 2) {
        pShmParams->shmTotalSize += (long long int)pConfigParams->videoFrameSizeMax*
                                    pConfigParams->videoQueLen*pConfigParams->videoMax +
                                   (long long int)pConfigParams->captureFrameSizeMax*
                                   pConfigParams->captureQueLen*pConfigParams->captureMax;
    }
    pShmParams->shmTotalSize = pShmParams->shmTotalSize/alignSize*alignSize + alignSize;

    //clearShmAndMsg(pMediaServerParams);

    pShmParams->shmId = shmget(pConfigParams->shmKey, pShmParams->shmTotalSize, 0666|IPC_CREAT);
    if(pShmParams->shmId < 0) {
        app_err("shmget failed, key:%x, shmTotalSize:%lld, systemFreeMem:%lld, err:%s", 
                pConfigParams->shmKey, pShmParams->shmTotalSize, getSysFreeMem(), strerror(errno));
        exit(-1);
    }
    pShmParams->shmAddr = shmat(pShmParams->shmId, 0, 0);
    if(pShmParams->shmAddr == (void *)-1) {
        app_err("shmget failed, key:%x, shmTotalSize:%lld, err:%s", 
                pConfigParams->shmKey, pShmParams->shmTotalSize, strerror(errno));
        exit(-1);
    }

    initHeadShm(pShmParams, pConfigParams->shmHeadSize);

    app_debug("shm key : %d, shmAddr : %p, total size : %lld", 
            pConfigParams->shmKey, pShmParams->shmAddr, pShmParams->shmTotalSize);
    
    return 0;
}

int shmDestroy(void *ptr) {
    return 0;
}

