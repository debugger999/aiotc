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

static int delOldTs(const void *ptr, void *obj) {
    int j, k;
    FILE *fp;
    char buf[512], m3u8File[256], tsFile[256];
    objParam *pObjParam = (objParam *)obj;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    snprintf(m3u8File, sizeof(m3u8File), "%s/webpages/m3u8/stream%d/play.m3u8", pConfigParams->workdir, pObjParam->id);
    if(!access(m3u8File, F_OK)) {
        fp = fopen(m3u8File, "rb");
        if(fp == NULL) {
            app_warring("fopen %s failed", m3u8File);
            return -1;
        }

        char *p1, *p2, *p3;
        int tryCnt = 48;
        int len = fread(buf, 1, sizeof(buf), fp);
        if(len > 64) {
            p1 = buf + len - 2;
            while((*p1 != '\r') && (*p1 != '\n') && (tryCnt > 0)) {
                p1 --;
                tryCnt --;
            }
            if(tryCnt <= 0) {
                fclose(fp);
                return 0;
            }
            p1 ++;
            p2 = p1 + strlen("play");
            p3 = strstr(p2, ".ts");
            if(p3 == NULL) {
                fclose(fp);
                return 0;
            }
            *p3 = '\0';
            int tsNum;
            int latestNum = atoi(p2);
            if(latestNum > 5) {
                for(j = 1; j <= 5; j ++) {
                    tsNum = latestNum - HLS_LIST_NUM - j;
                    if(tsNum >= 0) {
                        snprintf(tsFile, sizeof(tsFile), "%s/webpages/m3u8/stream%d/play%d.ts", 
                                pConfigParams->workdir, pObjParam->id, tsNum);
                        if(!access(tsFile, F_OK)) {
                            remove(tsFile);
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            if(latestNum != pPreviewParams->lastTsNum) {
                pPreviewParams->lastTsNum = latestNum;
                pPreviewParams->lastTsSameCnt = 0;
            }
            else {
                pPreviewParams->lastTsSameCnt ++;
                if(pPreviewParams->lastTsSameCnt > 60) {
                    app_warring("stream%d, last ts file is too long, rm m3u8File", pObjParam->id);
                    remove(m3u8File);
                    pPreviewParams->lastTsSameCnt = 0;
                }
            }

            for(k = 0; k < 3; k ++) {
                snprintf(tsFile, sizeof(tsFile), "%s/webpages/m3u8/stream%d/play%d.ts", 
                        pConfigParams->workdir, pObjParam->id, latestNum + k);
                if(!access(tsFile, F_OK)) {
                    int fileSize = file_size(tsFile);
                    if(fileSize > 20000000) {
                        printf("id:%d, exception ts file, remove %s, size:%d\n", pObjParam->id, tsFile, fileSize);
                        remove(tsFile);
                    }
                }
            }
        }
        fclose(fp);
    }
    else {
        for(k = 0; k < 3; k ++) {
            snprintf(tsFile, sizeof(tsFile), "%s/webpages/m3u8/stream%d/play%d.ts", pConfigParams->workdir, pObjParam->id, k);
            if(!access(tsFile, F_OK)) {
                int fileSize = file_size(tsFile);
                if(fileSize > 20000000) {
                    printf("exception ts file, remove %s, size:%d\n", tsFile, fileSize);
                    remove(tsFile);
                }
            }
        }
    }

    return 0;
}

static void *hls_thread(void *arg) {
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
    pPreviewParams->checkFunc = delOldTs;

    printf("preview, id:%d, addr: http://%s:%d/m3u8/stream%d/play.m3u8\n", 
            pObjParam->id, pAiotcParams->localIp, pSystemParams->httpPort, pObjParam->id);

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

int hls_init(const void *buf, void *arg) {
    taskOps *pTaskOps = (taskOps *)arg;
    int *prewInit = (int *)pTaskOps->arg;

    if(*prewInit) {
        return 0;
    }

    ffmpegInit();
    *prewInit = 1;

    return 0;
}

int hls_start(const void *buf, void *arg) {
    pthread_t pid;

    if(pthread_create(&pid, NULL, hls_thread, arg) != 0) {
        app_err("pthread create preview thread err");
    }
    else {
        pthread_detach(pid);
    }

    return 0;
}

int hls_stop(const void *buf, void *arg) {
    objParam *pObjParam = (objParam *)arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;

    pPreviewParams->running = 0;

    return 0;
}

int hls_uninit(const void *buf, void *arg) {
    return 0;
}

