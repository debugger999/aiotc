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
#include "pids.h"
#include "master.h"
#include "slave.h"
#include "rest.h"
#include "work.h"
#include "record.h"
#include "stream.h"
#include "rtsp.h"
#include "gat1400.h"
#include "ehome.h"
#include "preview.h"
#include "decode.h"
#include "face.h"
#include "misc.h"
#include "task.h"

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
static pidOps g_pid_ops[] = {
    {"master",  "null",     "null",         masterProcess,  NULL},
    {"rest",    "null",     "null",         restProcess,    NULL},
    {"work",    "null",     "null",         workProcess,    workProcInit},
    {"obj",     "rtsp",     "live",         rtspProcess,    NULL},
    {"obj",     "rtsp",     "preview",      previewProcess, NULL},
    {"obj",     "rtsp",     "record",       recordProcess,  NULL},
    {"obj",     "ehome",    "live",         ehomeProcess,   NULL},
    {"obj",     "ehome",    "capture",      ehomeProcess,   NULL},
    {"obj",     "ehome",    "preview",      previewProcess, NULL},
    {"obj",     "ehome",    "record",       recordProcess,  NULL},
    {"obj",     "tcp",      "live",         streamProcess,  NULL},
    {"decode",  "video",    "null",         decodeProcess,  NULL},
    {"alg",     "face",     "null",         faceProcess,    NULL},
    {"obj",     "gat1400",  "capture",      gat1400Process, NULL},
    /*
    {"obj",     "gb28181",  "null",         gb28181Process, NULL},
    {"obj",     "ftp",      "null",         ftpProcess,     NULL},
    {"alg",     "veh",      "null",         vehProcess,     NULL},
    {"alg",     "pose",     "null",         poseProcess,    NULL},
    {"alg",     "yolo",     "null",         yoloProcess,    NULL},
    */
    {"null",    "null",     "null",         NULL,           NULL}
};

static int setKey2System(key_t key, const char *name, const char *subName, 
        const char *taskName, aiotcParams *pAiotcParams) {
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;

    if(!strcmp(name, "rest")) {
        pSlaveParams->restMsgKey = key;
    }
    else if(!strcmp(name, "work")) {
        pSlaveParams->workMsgKey = key;
    }

    return 0;
}

static key_t getMsgKey(const char *name, const char *subName, const char *taskName, aiotcParams *pAiotcParams) {
    key_t key = -1;
    int i, j, max, used, empty;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;
    int *keyCache = pSlaveParams->keyCache;

    empty = -1;
    max = pConfigParams->msgKeyStart + pConfigParams->msgKeyMax;
    for(i = pConfigParams->msgKeyStart + 1; i < max; i ++) {
        used = 1;
        for(j = 0; j < pConfigParams->msgKeyMax; j ++) {
            if(keyCache[j] == i || keyCache[j] == 0) {
                if(keyCache[j] == 0) {
                    used = 0;
                }
                break;
            }
            if(keyCache[j] == -1 && empty == -1) {
                empty = j;
            }
        }
        if(!used) {
            key = i;
            if(empty >= 0) {
                keyCache[empty] = i;
            }
            else {
                keyCache[j] = i;
            }
            setKey2System(key, name, subName, taskName, pAiotcParams);
            break;
        }
    }

    return key;
}

static int delMsgKey(int key, aiotcParams *pAiotcParams) {
    int j;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;
    int *keyCache = pSlaveParams->keyCache;

    for(j = 0; j < pConfigParams->msgKeyMax; j ++) {
        if(keyCache[j] == key || keyCache[j] == 0) {
            if(keyCache[j] == key) {
                keyCache[j] = -1;
            }
            break;
        }
    }

    return 0;
}

static void *beat_thread(void *arg) {
    commonParams *pParams = (commonParams *)arg;
    pidOps *pOps = (pidOps *)pParams->arga;
    int sec = *(int *)pParams->argb;
    NodeCallback   obj_task_beat = pParams->callback;

    pParams->val = 1;
    while(pOps->running) {
        if(sec == TASK_BEAT_SLEEP) {
            gettimeofday(&pOps->tv, NULL);
        }
        semWait(&pOps->mutex_pobj);
        traverseQueue(&pOps->pobjQueue, pOps, obj_task_beat);
        semPost(&pOps->mutex_pobj);
        sleep(sec);
    }

    return NULL;
}

pidOps *getRealOps(pidOps *pOps) {
    pid_t pid;
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
        app_warning("get pid ops failed, pid:%d, %s-%s-%s", pid, pOps->name, pOps->subName, pOps->taskName);
    }

    return p;
}

int initTaskOps(pidOps *pOps, taskOps *pTaskOps) {
    gettimeofday(&pOps->tv, NULL);
    pOps->procTaskOps = pTaskOps;

    return 0;
}

int createBeatTask(pidOps *pOps, int (*obj_task_beat)(node_common *, void *), int sec) {
    pthread_t pid;
    int wait = 100;
    commonParams params;

    params.arga = pOps;
    params.argb = &sec;
    params.val = 0;
    params.callback = obj_task_beat;
    if(pthread_create(&pid, NULL, beat_thread, &params) != 0) {
        app_err("create thread failed");
    }
    else {
        pthread_detach(pid);
    }

    do {
        if(params.val) {
            break;
        }
        usleep(100000);
    } while(wait --);
    if(wait <= 0) {
        app_warning("wait beat thread failed");
    }

    return 0;
}

pidOps *getOpsByName(const char *name, const char *subName, const char *taskName) {
    int i;
    pidOps *pOps;
    pidOps *pOpsRet = NULL;

    for(i = 0; ; i ++) {
        pOps = g_pid_ops + i;
        if(!strncmp(pOps->name, "null", sizeof(pOps->name))) {
            app_warning("get proc by name failed, %s %s %s", name, subName, taskName);
            break;
        }
        if(strncmp(name, pOps->name, sizeof(pOps->name)) == 0 &&
           strncmp(subName, pOps->subName, sizeof(pOps->subName)) == 0 &&
           strncmp(taskName, pOps->taskName, sizeof(pOps->taskName)) == 0) {
            pOpsRet = pOps;
            break;
        }
    }

    return pOpsRet;
}

static int conditionByPid(node_common *p, void *arg) {
    pid_t pid = *(pid_t *)arg;
    pidOps *pOps = (pidOps *)p->name;
    return pid == pOps->pid;
}

static int conditionByPidOps(node_common *p, void *arg) {
    int find = 0;
    node_common *pNode = NULL;
    pidOps *pOps = (pidOps *)p->name;
    commonParams *pParams = (commonParams *)arg;
    int id = *(int *)pParams->arga;
    const char *name = (const char *)pParams->argb;
    const char *subName = (const char *)pParams->argc;
    pidOps *pTmp = (pidOps *)pParams->arge;

    do {
        if(strncmp(name, pOps->name, sizeof(pOps->name)) ||
           strncmp(subName, pOps->subName, sizeof(pOps->subName)) ||
           pOps->proc != pTmp->proc ||
           pOps->load >= PROC_LOAD_MAX) {
            break;
        }
        semWait(&pOps->mutex_pobj);
        searchFromQueue(&pOps->pobjQueue, &id, &pNode, conditionByObjId);
        if(pNode != NULL) {
            find = 1;
        }
        semPost(&pOps->mutex_pobj);
    } while(0);

    return find;
}

static int conditionByPidOps2(node_common *p, void *arg) {
    int find = 0;
    pidOps *pOps = (pidOps *)p->name;
    commonParams *pParams = (commonParams *)arg;
    const char *name = (const char *)pParams->argb;
    const char *subName = (const char *)pParams->argc;
    pid_t *pid = (pid_t *)pParams->argd;
    pidOps *pTmp = (pidOps *)pParams->arge;

    if(strncmp(name, pOps->name, sizeof(pOps->name)) == 0 &&
       strncmp(subName, pOps->subName, sizeof(pOps->subName)) == 0 &&
       pOps->proc == pTmp->proc &&
       pOps->load < PROC_LOAD_MAX) {
        *pid = pOps->pid;
        find = 1;
    }

    return find;
}

pidOps *getOpsByPid(pid_t pid, void *arg) {
    node_common *p = NULL;
    pidOps *pOpsRet = NULL;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;

    semWait(&pPidsParams->mutex_pid);
    searchFromQueue(&pPidsParams->pidQueue, &pid, &p, conditionByPid);
    if(p != NULL) {
        pOpsRet = (pidOps *)p->name;
    }
    else {
        printf("pid %d not exsit\n", pid);
    }
    semPost(&pPidsParams->mutex_pid);

    return pOpsRet;
}

static int put2PidQueue(pidOps *pOps, void *arg) {
    node_common node;
    pidOps *p = (pidOps *)node.name;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    memset(&node, 0, sizeof(node));
    memcpy(p, pOps, sizeof(pidOps));
    if(sem_init(&p->mutex_pobj, 1, 1) < 0) {
      app_err("sem init failed");
      return -1;
    }
    p->arg = pAiotcParams;
    semWait(&pPidsParams->mutex_pid);
    putToShmQueue(pShmParams->headsp, &pPidsParams->pidQueue, &node, pConfigParams->slaveObjMax*3);
    semPost(&pPidsParams->mutex_pid);

    return 0;
}

static int delOpsByPid(pid_t pid, aiotcParams *pAiotcParams) {
    node_common *p = NULL;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;

    semWait(&pPidsParams->mutex_pid);
    delFromQueue(&pPidsParams->pidQueue, &pid, &p, conditionByPid);
    semPost(&pPidsParams->mutex_pid);
    if(p != NULL) {
        pidOps *pOps = (pidOps *)p->name;
        delMsgKey(pOps->msgKey, pAiotcParams);
        freeShmQueue(pShmParams->headsp, &pOps->pobjQueue, NULL);
        sem_destroy(&pOps->mutex_pobj);
        shmFree(pShmParams->headsp, p);
    }

    return 0;
}

static int putObj2pidQue(objParam *pObjParam, pidOps *pOps) {
    int wait = 100;
    node_common node;
    objParam *p = (objParam *)node.name;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    do {
        if(pOps->taskMax > 0) {
            break;
        }
        usleep(100000);
    } while(wait --);
    if(pOps->taskMax <= 0) {
        app_warning("wait proc init failed, pid:%d, %s-%s-%s", 
                pOps->pid, pOps->name, pOps->subName, pOps->taskName);
        return -1;
    }

    memset(&node, 0, sizeof(node));
    memcpy(p, pObjParam, sizeof(objParam));
    p->reserved = pOps;
    semWait(&pOps->mutex_pobj);
    putToShmQueue(pShmParams->headsp, &pOps->pobjQueue, &node, pOps->taskMax);
    pOps->load = pOps->pobjQueue.queLen*100/pOps->taskMax;
    semPost(&pOps->mutex_pobj);

    return 0;
}

int delObjFromPidQue(int id, pidOps *pOps, int lock) {
    node_common *p = NULL;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    if(lock) semWait(&pOps->mutex_pobj);
    delFromQueue(&pOps->pobjQueue, &id, &p, conditionByObjId);
    pOps->load = pOps->pobjQueue.queLen*100/pOps->taskMax;
    if(lock) semPost(&pOps->mutex_pobj);
    if(p != NULL) {
        shmFree(pShmParams->headsp, p);
    }

    return 0;
}

pid_t createProcess(const char *name, const char *subName, const char *taskName, void *arg) {
    pid_t pid;
    pidOps *pOps;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;

    pOps = getOpsByName(name, subName, taskName);
    if(pOps == NULL) {
        app_warning("get ops by name failed, %s-%s-%s", name, subName, taskName);
        return -1;
    }
    pOps->arg = pAiotcParams;
    pOps->msgKey = getMsgKey(name, subName, taskName, pAiotcParams);
    if(pOps->msgKey < 0) {
        app_warning("get msg key failed, %s-%s-%s", name, subName, taskName);
        return -1;
    }
    if(pOps->procInit != NULL && pOps->procInit(pOps) != 0) {
        app_warning("proc init failed, %s-%s-%s", name, subName, taskName);
        return -1;
    }

    pid = fork();
    if(pid == -1) {
        app_err("fork failed");
        exit(-1);
    }
    else if(pid == 0) {
        pOps->proc(pOps);
        exit(0);
    }
    else {
        pOps->pid = pid;
        put2PidQueue(pOps, pAiotcParams);
        app_debug("pid:%d, %s-%s-%s", pid, name, subName, taskName);
    }

    return pid;
}

int creatProcByPid(pid_t oldPid, int status, void *arg) {
    pid_t pid;
    pidOps *pOps;
    struct timeval tv;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;

    pOps = getOpsByPid(oldPid, pAiotcParams);
    if(pOps == NULL) {
        app_warning("get ops by pid %d failed", oldPid);
        return -1;
    }
    if(!pOps->running) {
        app_debug("pid %d stop normaly, %s-%s-%s", oldPid, pOps->name, pOps->subName, pOps->taskName);
        delOpsByPid(oldPid, pAiotcParams);
        return 0;
    }

    gettimeofday(&tv, NULL);
    if((int)tv.tv_sec - pOps->lastReboot < 300) {
        app_warning("proc %s-%s-%s, pid %d, restart too high frequency, don't run it", 
                pOps->name, pOps->subName, pOps->taskName, pid);
        return -1;
    }
    app_warning("proc %s-%s-%s, pid %d quit, exit status %d, restart it ...", 
            pOps->name, pOps->subName, pOps->taskName, oldPid, status);

    pid = fork();
    if(pid == -1) {
        app_err("fork failed");
        exit(-1);
    }
    else if(pid == 0) {
        pOps->proc(pOps);
        exit(0);
    }
    else {
        pOps->pid = pid;
        pOps->lastReboot = (int)tv.tv_sec;
        app_debug("restart %s pid : %d", pOps->name, pid);
    }

    return 0;
}

pidOps *getEmptyProc(const char *name, const char *subName, const char *taskName, void *arg) {
    pid_t pid = -1;
    pidOps *pOps = NULL;
    node_common *p = NULL;
    commonParams params;
    objParam *pObjParam = (objParam *)arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;

    pidOps *pTmp = getOpsByName(name, subName, taskName);
    if(pTmp == NULL) {
        return NULL;
    }
    memset(&params, 0, sizeof(params));
    params.arga = &pObjParam->id;
    params.argb = (void *)name;
    params.argc = (void *)subName;
    params.argd = (void *)taskName;
    params.arge = pTmp;
    semWait(&pPidsParams->mutex_pid);
    searchFromQueue(&pPidsParams->pidQueue, &params, &p, conditionByPidOps);
    if(p == NULL) {
        params.argd = &pid;
        searchFromQueue(&pPidsParams->pidQueue, &params, &p, conditionByPidOps2);
    }
    if(p != NULL) {
        pOps = (pidOps *)p->name;
    }
    semPost(&pPidsParams->mutex_pid);

    if(pOps == NULL) {
        if(pSlaveParams->load < SLAVE_LOAD_MAX) {
            pid = createProcess(name, subName, taskName, pAiotcParams);
        }
        else {
            app_warning("slave load is too high : %d", pSlaveParams->load);
        }
    }

    if(pid > 0) {
        pOps = getOpsByPid(pid, pAiotcParams);
        if(pOps != NULL) {
            putObj2pidQue(pObjParam, pOps);
        }
    }

    return pOps;
}

pidOps *getTaskProc(const char *name, const char *subName, const char *taskName, void *arg) {
    return NULL;
}

