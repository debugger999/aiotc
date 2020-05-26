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
#include "msg.h"
#include "obj.h"
#include "task.h"
#include "rtsp.h"

static int rtsp_callback(unsigned char *p_buf, int size, void *param) {
    //char buf[4] = {0x00, 0x00, 0x00, 0x01};
    objParam *pObjParam = (objParam *)param;
    pidOps *pOps = (pidOps *)pObjParam->reserved;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    int nowSec = (int)pOps->tv.tv_sec;

    if(pTaskParams->liveTaskBeat != nowSec) {
        pTaskParams->liveTaskBeat = nowSec;
    }

    return 0; 
}

static int rtsp_start(const void *buf, void *arg) {
    int tcp;
    char *url = NULL;
    objParam *pObjParam = (objParam *)arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    char *buff = pObjParam->originaldata;

    if(pTaskParams->liveArgs == NULL) {
        pTaskParams->liveArgs = malloc(sizeof(rtspParams)); // TODO : free it if proc is empty long time
        if(pTaskParams->liveArgs == NULL) {
            app_err("malloc failed");
            return -1;
        }
        memset(pTaskParams->liveArgs, 0, sizeof(rtspParams));
    }

    rtspParams *pRtspParams = (rtspParams *)pTaskParams->liveArgs;
    player_params *player = &pRtspParams->player;
    if(player->playhandle != NULL) {
        app_warring("rtsp %s is already running", player->url);
        return -1;
    }

    tcp = getIntValFromJson(buff, "data", "tcpEnable", NULL);
    url = getStrValFromJson(buff, "data", "url", NULL);
    if(tcp < 0 || url == NULL) {
        printf("get rtsp json params failed, id:%d\n", pObjParam->id);
        goto end;
    }
    player->cb = rtsp_callback;
    player->streamUsingTCP = tcp;
    player->buffersize = DEFAULT_RTSP_BUFFER_SIZE;
    strncpy((char *)player->url, url, sizeof(player->url));
    player->arg = pObjParam;
    if(player_start_play(player)) {
        app_err("player_start_play %s failed ", player->url);
        goto end;
    }
    printf("rtsp start ok, id:%d\n", pObjParam->id);

end:
    if(url != NULL) {
        free(url);
    }

    return 0;
}

static int rtsp_stop(const void *buf, void *arg) {
    objParam *pObjParam = (objParam *)arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    rtspParams *pRtspParams = (rtspParams *)pTaskParams->liveArgs;

    if(pRtspParams == NULL) {
        app_warring("id:%d, rtsp params is null", pObjParam->id);
        return -1;
    }
    player_params *player = &pRtspParams->player;

    player->threadDoneFlag = 1;
    if(player->playhandle != NULL) {
        player_stop_play(player);
    }
    printf("rtsp stop ok, id:%d\n", pObjParam->id);

    return 0;
}

static taskOps rtspTaskOps = {
    .init = NULL,
    .uninit = NULL,
    .start = rtsp_start,
    .stop = rtsp_stop,
    .ctrl = NULL
};

static int rtspStartTask(char *buf, void *arg) {
    return 0;
}

static cmdTaskParams g_CmdParams[] = {
    {"startTask",       rtspStartTask},
    {"null",            NULL}
};

static int rtspProcBeat(node_common *p, void *arg) {
    pidOps *pOps = (pidOps *)arg;
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;

    if(pTaskParams->livestream) {
        pTaskParams->liveBeat = (int)pOps->tv.tv_sec;
    }

    return 0;
}

static int rtspTaskBeat(node_common *p, void *arg) {
    pidOps *pOps = (pidOps *)arg;
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    taskOps *pTaskOps = (taskOps *)pOps->procTaskOps;
    int nowSec = (int)pOps->tv.tv_sec;

    if(pTaskParams->livestream) {
        printf("##test, id:%d, %d, liveBeat:%d, liveTaskBeat:%d\n", 
                pObjParam->id, nowSec, pTaskParams->liveBeat, pTaskParams->liveTaskBeat);
        if(pTaskParams->liveTaskBeat == 0) {
            pTaskOps->start("live", pObjParam);
            pTaskParams->liveTaskBeat = nowSec;
        }
        else if(nowSec - pTaskParams->liveTaskBeat > TASK_BEAT_TIMEOUT) {
            app_warring("id:%d, detected exception, restart it ...", pObjParam->id);
            pTaskOps->stop("live", pObjParam);
            pTaskOps->start("live", pObjParam);
            pTaskParams->liveTaskBeat = nowSec;
        }
    }

    return 0;
}

int rtspProcess(void *arg) {
    msgParams msgParam;
    pidOps *pOps = (pidOps *)arg;

    pOps = getRealOps(pOps);
    if(pOps == NULL) {
        return -1;
    }
    pOps->running = 1;

    initTaskOps(pOps, &rtspTaskOps);
    createBeatTask(pOps, rtspProcBeat, 5);
    createBeatTask(pOps, rtspTaskBeat, TASK_BEAT_SLEEP);

    memset(&msgParam, 0, sizeof(msgParam));
    msgParam.key = pOps->msgKey;
    msgParam.pUserCmdParams = g_CmdParams;
    msgParam.arg = pOps->arg;
    msgParam.running = 1;
    createMsgThread(&msgParam);

    while(pOps->running) {
        sleep(2);
    }
    pOps->running = 0;

    app_debug("pid:%d, run over", getpid());

    return 0;
}

