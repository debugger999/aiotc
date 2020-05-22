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

static int rtspStartTask(char *buf, void *arg) {
    //aiotcParams *pAiotcParams = (aiotcParams *)(arg);
    //app_debug("##test, localIp:%s, buf:%s", pAiotcParams->localIp, buf);
    return 0;
}

static cmdTaskParams g_CmdParams[] = {
    {"startTask",       rtspStartTask},
    {"null",            NULL}
};

static int objTaskBeat(node_common *p, void *arg) {
    struct timeval *tv = (struct timeval *)arg;
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;

    // TODO
    pTaskParams->liveBeat = (int)tv->tv_sec;
    pTaskParams->previewBeat = (int)tv->tv_sec;

    return 0;
}

static void *rtsp_beat_thread(void *arg) {
    struct timeval tv;
    pidOps *pOps = (pidOps *)arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;

    while(pAiotcParams->running) {
        gettimeofday(&tv, NULL);
        semWait(&pOps->mutex_pobj);
        traverseQueue(&pOps->pobjQueue, &tv, objTaskBeat);
        semPost(&pOps->mutex_pobj);
        sleep(3);
    }

    return NULL;
}

static int createBeatTask(pidOps *pOps) {
    pid_t pid;
    pthread_t ptid;
    int wait = 100;
    pidOps *p = NULL;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;

    pid = getpid();
    do {
        p = getOpsByPid(pid, pAiotcParams);
        if(p != NULL) {
            break;
        }
        usleep(100000);
    } while(wait --);

    if(p == NULL) {
        app_warring("get pid ops failed, pid:%d, %s-%s-%s", pid, pOps->name, pOps->subName, pOps->taskName);
        return -1;
    }
    if(pthread_create(&ptid, NULL, rtsp_beat_thread, p) != 0) {
        app_err("create rtsp beat thread failed");
    }
    else {
        pthread_detach(ptid);
    }

    return 0;
}

int rtspProcess(void *arg) {
    msgParams msgParam;
    pidOps *pOps = (pidOps *)arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;

    memset(&msgParam, 0, sizeof(msgParam));
    msgParam.key = pOps->msgKey;
    msgParam.pUserCmdParams = g_CmdParams;
    msgParam.arg = pAiotcParams;
    msgParam.running = 1;
    createMsgThread(&msgParam);
    createBeatTask(pOps);

    while(pAiotcParams->running) {
        sleep(2);
    }

    app_debug("run over");

    return 0;
}

