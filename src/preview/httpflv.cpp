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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "obj.h"
#include "task.h"
#include "ffmpeg.h"
#include "preview.h"
#include "camera.h"
#include "system.h"
#include "misc.h"

int httpflv_init(const void *buf, void *arg) {
    taskOps *pTaskOps = (taskOps *)arg;
    int *prewInit = (int *)pTaskOps->arg;

    if(*prewInit) {
        return 0;
    }

    ffmpegInit();
    *prewInit = 1;

    return 0;
}

static void *httpflv_thread(void *arg) {
    int ret;
    shmParam *pVideoShm;
    objParam *pObjParam = (objParam *)arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;
    cameraParams *pCameraParams = (cameraParams *)pObjParam->objArg;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    systemParams *pSystemParams = (systemParams *)pAiotcParams->systemArgs;

    if(allocVideoShm(pObjParam) != 0) {
        return NULL;
    }
    pVideoShm = (shmParam *)pCameraParams->videoShm;
    setShmUser(pVideoShm, 1);

    pPreviewParams->running = 1;
    ret = previewInit(pObjParam);
    if(ret != 0) {
        pPreviewParams->running = 0;
        app_err("preview init failed, id:%d", pObjParam->id);
    }

    printf("preview, id:%d, addr: http://%s:%d/live?port=1935&app=myapp&stream=stream%d\n", 
            pObjParam->id, pAiotcParams->localIp, pSystemParams->httpFlvPort, pObjParam->id);

    previewLoop(pObjParam);

    setShmUser(pVideoShm, 0);
    if(pVideoShm->queue.useMax == 0) {
        releaseShm(pVideoShm);
    }
    previewUnInit(pObjParam);
    shmFree(pShmParams->headsp, pTaskParams->previewArgs);
    pTaskParams->previewArgs = NULL;

    app_debug("id:%d, run over", pObjParam->id);

    return NULL;
}

int httpflv_start(const void *buf, void *arg) {
    pthread_t pid;

    if(pthread_create(&pid, NULL, httpflv_thread, arg) != 0) {
        app_err("pthread create preview thread err");
    }
    else {
        pthread_detach(pid);
    }

    return 0;
}

int httpflv_stop(const void *buf, void *arg) {
    objParam *pObjParam = (objParam *)arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;

    pPreviewParams->running = 0;

    return 0;
}

int httpflv_uninit(const void *buf, void *arg) {
    return 0;
}

