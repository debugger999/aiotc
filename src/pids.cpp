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
#include "db.h"
#include "shm.h"
#include "master.h"
#include "rest.h"
#include "work.h"
#include "pids.h"
#include "stream.h"
#include "rtsp.h"
#include "ehome.h"
#include "decode.h"
#include "face.h"

// main进程尽量简单，只做进程守护，所有进程都需要通过main创建，
// 进程是无状态的，进程创建后只能被动接收任务命令，进程挂掉后，main守护启动，不做其他事情
// obj/task可以是多进程，也可以是单进程多线程，
// 存在一个obj任务监控模块，发现obj任务停止，表示任务刚添加还没有启动或相关任务进程挂掉重启，
// 寻找一个最空闲的进程，没有则创建新进程，把obj任务分配上去。
// 动态的stop、del命令需要有资源同步机制，如shm的user
// 做到进程无状态最重要两点：
// 所有参数都在共享内存中，大家零拷贝共享，避免消息同步；
// 进程任务启动依赖obj任务监控模块，避免rest命令传输启动导致的进程间状态耦合和维护；
// 为了防止各种算法进程编译库冲突，alg模块独立为自己的编译工程，不适合fork
// objManager : getEmptyProc(name, subName)->proc.start(cmd)
static PidOps g_pid_ops[] = {
    {"master",  "null",         masterProcess},
    {"rest",    "null",         restProcess},
    {"work",    "null",         workProcess},
    {"obj",     "tcp",          streamProcess},
    {"obj",     "rtsp",         rtspProcess},
    {"obj",     "ehome",        ehomeProcess},
    {"decode",  "video",        decodeProcess},
    {"alg",     "face",         faceProcess},
    /*
    {"obj",     "gb28181",      gb28181Process},
    {"obj",     "gat1400",      gat1400Process},
    {"obj",     "ftp",          ftpProcess},
    {"alg",     "veh",          vehProcess},
    {"alg",     "pose",         poseProcess},
    {"alg",     "yolo",         yoloProcess},
    */
    {"null",    "null",         NULL}
};

PidOps *getOpsByName(const char *name ) {
    int i;
    PidOps *pOps;
    PidOps *pOpsRet = NULL;

    for(i = 0; ; i ++) {
        pOps = g_pid_ops + i;
        if(!strncasecmp(pOps->name, "null", 32)) {
            app_warring("get proc by name %s failed", name);
            break;
        }
        if(!strncasecmp(pOps->name, name, 32)) {
            pOpsRet = pOps;
            break;
        }
    }

    return pOpsRet;
}

static int conditionByPid(node_common *p, void *arg) {
    pid_t pid = *(pid_t *)arg;
    PidOps *pOps = (PidOps *)p->name;
    return pid == pOps->pid;
}

PidOps *getOpsByPid(pid_t pid, void *arg) {
    node_common *p = NULL;
    PidOps *pOpsRet = NULL;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;

    semWait(&pPidsParams->mutex_pid);
    searchFromQueue(&pPidsParams->pidQueue, &pid, &p, conditionByPid);
    if(p != NULL) {
        pOpsRet = (PidOps *)p->name;
    }
    else {
        printf("pid %d not exsit\n", pid);
    }
    semPost(&pPidsParams->mutex_pid);

    return pOpsRet;
}

int put2PidQueue(PidOps *pOps, void *arg) {
    node_common node;
    PidOps *p = (PidOps *)node.name;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    memset(&node, 0, sizeof(node));
    memcpy(p, pOps, sizeof(PidOps));
    p->arg = pAiotcParams;
    semWait(&pPidsParams->mutex_pid);
    putToShmQueue(pShmParams->headsp, &pPidsParams->pidQueue, &node, pConfigParams->slaveObjMax*3);
    semPost(&pPidsParams->mutex_pid);

    return 0;
}

