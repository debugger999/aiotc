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
#include "misc.h"
#include "shm.h"
#include "master.h"
#include "rest.h"
#include "obj.h"
#include "pids.h"
#include "system.h"
#include "slave.h"
#include "task.h"
#include "msg.h"

static int main_child_quit_signal(const int signal, void *ptr) {
    int status;
    int quit_pid = wait(&status);
    static aiotcParams *pAiotcParams = NULL;

    if(pAiotcParams == NULL) {
        pAiotcParams = (aiotcParams *)ptr;
        return 0;
    }
    if(!pAiotcParams->running) {
        return 0;
    }
    creatProcByPid(quit_pid, status, pAiotcParams);

    return 0;
}

static int initPtr(aiotcParams *pAiotcParams, shmParams *pShmParams) {
    pAiotcParams->masterArgs = shmMalloc(pShmParams->headsp, sizeof(masterParams));
    pAiotcParams->slaveArgs = shmMalloc(pShmParams->headsp, sizeof(slaveParams));
    pAiotcParams->restArgs = shmMalloc(pShmParams->headsp, sizeof(restParams));
    pAiotcParams->shmArgs = shmMalloc(pShmParams->headsp, sizeof(shmParams));
    pAiotcParams->configArgs = shmMalloc(pShmParams->headsp, sizeof(configParams));
    pAiotcParams->pidsArgs = shmMalloc(pShmParams->headsp, sizeof(pidsParams));
    pAiotcParams->dbArgs = shmMalloc(pShmParams->headsp, sizeof(dbParams));
    pAiotcParams->systemArgs = shmMalloc(pShmParams->headsp, sizeof(systemParams));
    pAiotcParams->objArgs = shmMalloc(pShmParams->headsp, sizeof(objParams));
    memset(pAiotcParams->masterArgs, 0, sizeof(masterParams));
    memset(pAiotcParams->slaveArgs, 0, sizeof(slaveParams));
    memset(pAiotcParams->restArgs, 0, sizeof(restParams));
    memset(pAiotcParams->shmArgs, 0, sizeof(shmParams));
    memset(pAiotcParams->configArgs, 0, sizeof(configParams));
    memset(pAiotcParams->pidsArgs, 0, sizeof(pidsParams));
    memset(pAiotcParams->dbArgs, 0, sizeof(dbParams));
    memset(pAiotcParams->systemArgs, 0, sizeof(systemParams));
    memset(pAiotcParams->objArgs, 0, sizeof(objParams));
    ((masterParams *)pAiotcParams->masterArgs)->arg = pAiotcParams;
    ((slaveParams *)pAiotcParams->slaveArgs)->arg = pAiotcParams;
    ((restParams *)pAiotcParams->restArgs)->arg = pAiotcParams;
    ((shmParams *)pAiotcParams->shmArgs)->arg = pAiotcParams;
    ((configParams *)pAiotcParams->configArgs)->arg = pAiotcParams;
    ((pidsParams *)pAiotcParams->pidsArgs)->arg = pAiotcParams;
    ((dbParams *)pAiotcParams->dbArgs)->arg = pAiotcParams;
    ((systemParams *)pAiotcParams->systemArgs)->arg = pAiotcParams;
    ((systemParams *)pAiotcParams->objArgs)->arg = pAiotcParams;
    pShmParams->arg = pAiotcParams;

    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    if(sem_init(&pMasterParams->mutex_slave, 1, 1) < 0) {
        app_err("sem_init failed");
        return -1;
    }
    if(sem_init(&pMasterParams->mutex_mobj, 1, 1) < 0) {
        app_err("sem_init failed");
        return -1;
    }
    if(sem_init(&pObjParams->mutex_obj, 1, 1) < 0) {
        app_err("sem_init failed");
        return -1;
    }
    if(sem_init(&pPidsParams->mutex_pid, 1, 1) < 0) {
        app_err("sem_init failed");
        return -1;
    }

    return 0;
}

static int miscInit(aiotcParams *pAiotcParams) {
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    initShmArray(pShmParams);
    pSlaveParams->keyCache = (int *)shmMalloc(pShmParams->headsp, pConfigParams->msgKeyMax*sizeof(int));
    if(pSlaveParams->keyCache == NULL) {
        app_err("shm malloc %ld failed", pConfigParams->msgKeyMax*sizeof(int));
        return -1;
    }
    memset(pSlaveParams->keyCache, 0, pConfigParams->msgKeyMax*sizeof(int));
    pSlaveParams->mainMsgKey = pConfigParams->msgKeyStart;

    initHttpPort(pAiotcParams);

    clearSystemIpc(pAiotcParams);

    return 0;
}

static int init(aiotcParams **ppAiotcParams) {
    shmParams shmParam;
    configParams configParam;
    aiotcParams *pAiotcParams;

    memset(&configParam, 0, sizeof(configParam));
    memset(&shmParam, 0, sizeof(shmParam));
    configInit(&configParam);
    shmInit(&configParam, &shmParam);
    pAiotcParams = (aiotcParams *)shmMalloc(shmParam.headsp, sizeof(aiotcParams));
    if(pAiotcParams == NULL) {
        app_err("shm malloc failed");
        return -1;
    }
    memset(pAiotcParams, 0, sizeof(aiotcParams));

    initPtr(pAiotcParams, &shmParam);
    memcpy(pAiotcParams->configArgs, &configParam, sizeof(configParams));
    memcpy(pAiotcParams->shmArgs, &shmParam, sizeof(shmParams));
    dbInit(pAiotcParams);
    miscInit(pAiotcParams);

    pAiotcParams->running = 1;
    *ppAiotcParams = pAiotcParams;

    return 0;
}

int destroy(aiotcParams *pAiotcParams) {
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    ncx_slab_pool_t *headsp = pShmParams->headsp;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;

    sem_destroy(&pMasterParams->mutex_slave);
    sem_destroy(&pMasterParams->mutex_mobj);
    sem_destroy(&pObjParams->mutex_obj);
    sem_destroy(&pPidsParams->mutex_pid);
    shmFree(headsp, pSlaveParams->keyCache);
    shmFree(headsp, pAiotcParams->masterArgs);
    shmFree(headsp, pAiotcParams->slaveArgs);
    shmFree(headsp, pAiotcParams->restArgs);
    shmFree(headsp, pAiotcParams->configArgs);
    shmFree(headsp, pAiotcParams->pidsArgs);
    shmFree(headsp, pAiotcParams->dbArgs);
    shmFree(headsp, pAiotcParams->systemArgs);
    shmFree(headsp, pAiotcParams->shmArgs);
    shmFree(headsp, pAiotcParams);
    shmDestroy(headsp);

    return 0;
}

static int startObjTask(const char *name, const char *taskName, objParam *pObjParam) {
    pidOps *pOps;
    //aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    //slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;

    pOps = getEmptyProc(name, pObjParam->subtype, taskName, pObjParam);
    if(pOps != NULL) {
        //char cmd[128];
        //snprintf(cmd, sizeof(cmd), "{\"cmd\":\"startTask\",\"data\":{\"id\":%d,\"val\":\"%s\"}}",
        //        pObjParam->id, taskName);
        //msgSend(cmd, pOps->msgKey, pSlaveParams->mainMsgKey, 10);
        app_debug("get empty proc success, pid:%d, id:%d,%s,%s,%s", 
                pOps->pid, pObjParam->id, name, pObjParam->subtype, taskName);
    }
    else {
        printf("get empty proc failed, id:%d,%s,%s,%s\n", 
                pObjParam->id, name, pObjParam->subtype, taskName);
    }

    return 0;
}

/*
static int stopTask(const char *name, const char *taskName, objParam *pObjParam) {
    char cmd[128];
    pidOps *pOps;

    pOps = getTaskProc(name, pObjParam->subtype, taskName, pObjParam);
    if(pOps != NULL) {
        snprintf(cmd, sizeof(cmd), "{\"cmd\":\"stopTask\",\"data\":{\"id\":%d,\"val\":%s}}",
                pObjParam->id, taskName);
        msgSend(cmd, pOps->msgKey, 0, 0);
    }
    else {
        app_warring("get task proc failed, id:%d,%s,%s,%s", 
                pObjParam->id, name, pObjParam->subtype, taskName);
    }

    return 0;
}
*/

static int objManager(node_common *p, void *arg) {
    pidOps *pOps;
    char taskName[32];
    int nowSec = (int)(*(__time_t *)arg);
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;

    /*
    printf("main objManager, id:%d, type:%s, subtype:%s, "
            "live:%d-%d, capture:%d-%d, record:%d-%d, preview:%s-%d, nowSec:%d\n", 
            pObjParam->id, pObjParam->type, pObjParam->subtype, pTaskParams->livestream, pTaskParams->liveBeat,
            pTaskParams->capture, pTaskParams->captureBeat, pTaskParams->record, pTaskParams->recordBeat, 
            pTaskParams->preview,  pTaskParams->previewBeat, nowSec);
    */
    if(pTaskParams->livestream && (nowSec - pTaskParams->liveBeat) > PROC_BEAT_TIMEOUT) {
        strncpy(taskName, "live", sizeof(taskName));
        if(pTaskParams->liveBeat > 0) {
            app_warring("id:%d, %s, proc beat timeout, restart obj task", pObjParam->id, taskName);
        }
        startObjTask("obj", taskName, pObjParam);
        pTaskParams->liveBeat = nowSec;
    }
    if(pTaskParams->capture && (nowSec - pTaskParams->captureBeat) > PROC_BEAT_TIMEOUT) {
        strncpy(taskName, "capture", sizeof(taskName));
        if(pTaskParams->captureBeat > 0) {
            app_warring("id:%d, %s, proc beat timeout, restart obj task", pObjParam->id, taskName);
        }
        startObjTask("obj", taskName, pObjParam);
        pTaskParams->captureBeat = nowSec;
    }
    if(pTaskParams->record && (nowSec - pTaskParams->recordBeat) > PROC_BEAT_TIMEOUT) {
        strncpy(taskName, "record", sizeof(taskName));
        if(pTaskParams->recordBeat > 0) {
            app_warring("id:%d, %s, proc beat timeout, restart obj task", pObjParam->id, taskName);
        }
        startObjTask("obj", taskName, pObjParam);
        pTaskParams->recordBeat = nowSec;
    }
    if(strlen(pTaskParams->preview) > 0 && (nowSec - pTaskParams->previewBeat) > PROC_BEAT_TIMEOUT) {
        strncpy(taskName, "preview", sizeof(taskName));
        if(pTaskParams->previewBeat > 0) {
            app_warring("id:%d, %s, proc beat timeout, restart obj task", pObjParam->id, taskName);
        }
        startObjTask("obj", taskName, pObjParam);
        pTaskParams->previewBeat = nowSec;
    }
    if(0) {
        pOps = getEmptyProc("alg", "face", "null", pObjParam);
        if(pOps != NULL) {
        }
    }

    /*
    if(pTaskParams->livestream == 0 && pTaskParams->liveTaskBeat == 0 &&
            pTaskParams->capture == 0 && pTaskParams->captureTaskBeat == 0 &&
            pTaskParams->record == 0 && pTaskParams->recordTaskBeat == 0 &&
            strlen(pTaskParams->preview) == 0 && pTaskParams->previewTaskBeat == 0 &&
            pTaskParams->algQueue.queLen == 0) {
        delObjFromPidQue();
    }
    */

    return 0;
}

static void *slave_objmanager_thread(void *arg) {
    struct timeval tv;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;

    sleep(5);
    while(pAiotcParams->running) {
        gettimeofday(&tv, NULL);
        semWait(&pObjParams->mutex_obj);
        traverseQueue(&pObjParams->objQueue, &tv.tv_sec, objManager);
        semPost(&pObjParams->mutex_obj);
        sleep(1);
    }

    app_debug("run over");

    return NULL;
}

static cmdTaskParams g_CmdParams[] = {
    {"null",            NULL}
};

static int createTasks(aiotcParams *pAiotcParams) {
    pthread_t pid;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    if(pConfigParams->masterEnable) {
        createProcess("master", "null", "null", pAiotcParams);
    }
    if(pConfigParams->masterEnable == 0 || pConfigParams->masterEnable == 2) {
        createProcess("work", "null", "null", pAiotcParams);
        createProcess("rest", "null", "null", pAiotcParams);
        if(pthread_create(&pid, NULL, slave_objmanager_thread, pAiotcParams) != 0) {
            app_err("pthread_create slave obj manager thread err");
        }
        else {
            pthread_detach(pid);
        }
    }

    return 0;
}

static int startMsgThread(msgParams *pMsgParams, aiotcParams *pAiotcParams) {
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    memset(pMsgParams, 0, sizeof(msgParams));
    pMsgParams->key = pConfigParams->msgKeyStart;
    pMsgParams->pUserCmdParams = g_CmdParams;
    pMsgParams->arg = pAiotcParams;
    pMsgParams->running = 1;
    createMsgThread(pMsgParams);

    return 0;
}

// TODO: detect main panic
int main(int argc, char *argv[]) {
    msgParams msgParam;
    aiotcParams *pAiotcParams = NULL;

    dirCheck("log");
    app_debug("Built: %s %s, version:%s, aiotcProc starting ...", __TIME__, __DATE__, AIOTC_VERSION);

    init(&pAiotcParams);
    if(pAiotcParams == NULL) {
        return -1;
    }

    main_child_quit_signal(-1, pAiotcParams);
    signal(SIGCHLD, (void (*)(int))main_child_quit_signal);
    createTasks(pAiotcParams);
    startMsgThread(&msgParam, pAiotcParams);

    while(pAiotcParams->running) {
        sleep(2);
    }

    //destroy(pAiotcParams);

    app_debug("run over");

    return 0;
}

