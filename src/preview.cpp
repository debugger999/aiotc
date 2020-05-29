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
#include "obj.h"
#include "task.h"
#include "db.h"
#include "preview.h"
#include "hls.h"
#include "httpflv.h"
#include "wspreview.h"

static taskOps previewOps[] = {
    {
        .type = "hls",
        .init = NULL,
        .uninit = NULL,
        .start = hls_start,
        .stop = hls_stop,
        .ctrl = NULL
    },
    {
        .type = "http-flv",
        .init = NULL,
        .uninit = NULL,
        .start = httpflv_start,
        .stop = httpflv_stop,
        .ctrl = NULL
    },
    {
        .type = "ws",
        .init = NULL,
        .uninit = NULL,
        .start = wspreview_start,
        .stop = wspreview_stop,
        .ctrl = NULL
    }
};

static int initPreviewOps(taskParams *pTaskParams) {
    int i;
    taskOps *p;
    int num = sizeof(previewOps) / sizeof(taskOps);

    if(pTaskParams->previewArgs == NULL) {
        pTaskParams->previewArgs = malloc(sizeof(previewParams));
        if(pTaskParams->previewArgs == NULL) {
            app_err("malloc failed");
            return -1;
        }
        memset(pTaskParams->previewArgs, 0, sizeof(previewParams));
    }

    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;
    for(i = 0; i < num; i ++) {
        p = previewOps + i;
        if(!strcmp(pTaskParams->preview, p->type)) {
            pPreviewParams->start = p->start;
            pPreviewParams->stop = p->stop;
            break;
        }
    }
    if(pPreviewParams->start == NULL) {
        app_warring("get preview ops failed, type:%s", pTaskParams->preview);
        return -1;
    }

    return 0;
}

static int preview_start(const void *buf, void *arg) {
    previewParams *pPreviewParams;
    objParam *pObjParam = (objParam *)arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;

    if(initPreviewOps(pTaskParams) != 0) {
        return -1;
    }

    pPreviewParams = (previewParams *)pTaskParams->previewArgs;
    pPreviewParams->start("preview", pObjParam);

    return 0;
}

static int preview_stop(const void *buf, void *arg) {
    objParam *pObjParam = (objParam *)arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;

    if(pPreviewParams != NULL && pPreviewParams->stop != NULL) {
        pPreviewParams->stop("preview", pObjParam);
    }

    return 0;
}

static taskOps previewTaskOps = {
    .type = NULL,
    .init = NULL,
    .uninit = NULL,
    .start = preview_start,
    .stop = preview_stop,
    .ctrl = NULL
};

static int previewProcBeat(node_common *p, void *arg) {
    pidOps *pOps = (pidOps *)arg;
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;

    if(strlen(pTaskParams->preview) > 0) {
        pTaskParams->previewBeat = (int)pOps->tv.tv_sec;
    }
    else if(strlen(pTaskParams->preview) == 0 && pTaskParams->previewTaskBeat == 0) {
        delObjFromPidQue(pObjParam->id, pOps, 0);
        pTaskParams->previewBeat = 0;
    }

    return 0;
}

static int previwTaskBeat(node_common *p, void *arg) {
    pidOps *pOps = (pidOps *)arg;
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    taskOps *pTaskOps = (taskOps *)pOps->procTaskOps;
    int nowSec = (int)pOps->tv.tv_sec;

    if(strlen(pTaskParams->preview) > 0) {
        if(pTaskParams->previewTaskBeat == 0) {
            pTaskOps->start("preview", pObjParam);
            pTaskParams->previewTaskBeat = nowSec;
        }
        else if(nowSec - pTaskParams->previewTaskBeat > TASK_BEAT_TIMEOUT) {
            //if(pTaskParams->previewRestart ++ < 3) {
            //    app_warring("id:%d, %s, detected exception, restart it ...", pObjParam->id, pOps->taskName);
            //}
            //else {
            //    printf("id:%d, %s, detected exception, restart it ...\n", pObjParam->id, pOps->taskName);
            //}
            //pTaskOps->stop("preview", pObjParam);
            //pTaskOps->start("preview", pObjParam);
            pTaskParams->previewTaskBeat = nowSec;
        }
    }
    else if(pTaskParams->previewTaskBeat > 0) {
        pTaskOps->stop("preview", pObjParam);
        pTaskParams->previewTaskBeat = 0;
    }

    return 0;
}

static int previewInit(pidOps *pOps) {
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    pOps->taskMax = pConfigParams->slaveObjMax;
    if(pOps->taskMax <= 0) {
        app_warring("get task max failed, %s-%s-%s", pOps->name, pOps->subName, pOps->taskName);
        pOps->taskMax = 1;
    }

    return 0;
}

int previewProcess(void *arg) {
    pidOps *pOps = (pidOps *)arg;

    pOps = getRealOps(pOps);
    if(pOps == NULL) {
        return -1;
    }
    pOps->running = 1;

    previewInit(pOps);
    initTaskOps(pOps, &previewTaskOps);
    createBeatTask(pOps, previewProcBeat, 5);
    createBeatTask(pOps, previwTaskBeat, TASK_BEAT_SLEEP);

    while(pOps->running) {
        sleep(2);
    }
    pOps->running = 0;

    app_debug("pid:%d, run over", pOps->pid);

    return 0;
}

