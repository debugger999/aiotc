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

static int creatProcByPid(pid_t oldPid, aiotcParams *pAiotcParams) {
    pid_t pid;
    pidOps *pOps;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    pOps = getOpsByPid(oldPid, pAiotcParams);
    if(pOps == NULL) {
        app_warring("get ops by pid %d failed", oldPid);
        return -1;
    }
    if((int)tv.tv_sec - pOps->lastReboot < 300) {
        app_warring("proc %s, pid %d, restart too high frequency, don't run it", pOps->name, pid);
        return -1;
    }

    pid = fork();
    if(pid == -1) {
        app_err("fork failed");
        exit(-1);
    }
    else if(pid == 0) {
        pOps->proc(pAiotcParams);
        exit(0);
    }
    else {
        pOps->pid = pid;
        pOps->lastReboot = (int)tv.tv_sec;
        app_debug("restart %s pid : %d", pOps->name, pid);
    }

    return 0;
}

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

    app_warring("child process %d quit, exit status %d, restart it ...", quit_pid, status);
    creatProcByPid(quit_pid, pAiotcParams);

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

    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    if(sem_init(&pMasterParams->mutex_slave, 1, 1) < 0) {
      app_err("sem init failed");
      return -1;
    }
    if(sem_init(&pMasterParams->mutex_mobj, 1, 1) < 0) {
      app_err("sem init failed");
      return -1;
    }
    if(sem_init(&pObjParams->mutex_obj, 1, 1) < 0) {
      app_err("sem init failed");
      return -1;
    }
    if(sem_init(&pPidsParams->mutex_pid, 1, 1) < 0) {
      app_err("sem init failed");
      return -1;
    }

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

    pAiotcParams->running = 1;
    *ppAiotcParams = pAiotcParams;

    return 0;
}

static int destroy(aiotcParams *pAiotcParams) {
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    ncx_slab_pool_t *headsp = pShmParams->headsp;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;
    pidsParams *pPidsParams = (pidsParams *)pAiotcParams->pidsArgs;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;

    sem_destroy(&pMasterParams->mutex_slave);
    sem_destroy(&pMasterParams->mutex_mobj);
    sem_destroy(&pObjParams->mutex_obj);
    sem_destroy(&pPidsParams->mutex_pid);
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

static int creatProcess(const char *name, aiotcParams *pAiotcParams) {
    pid_t pid;
    pidOps *pOps;

    pOps = getOpsByName(name);
    if(pOps == NULL) {
        app_warring("get ops by name %s failed", name);
        return -1;
    }

    pid = fork();
    if(pid == -1) {
        app_err("fork failed");
        exit(-1);
    }
    else if(pid == 0) {
        pOps->proc(pAiotcParams);
        exit(0);
    }
    else {
        pOps->pid = pid;
        put2PidQueue(pOps, pAiotcParams);
        app_debug("%s pid : %d", name, pid);
    }

    return 0;
}

static int startTask(const char *name, const char *taskName, objParam *pObjParam) {
    pidOps *pOps;
    char cmd[128];
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    pOps = getEmptyProc(name, pObjParam->subtype, taskName, pObjParam);
    if(pOps != NULL) {
        snprintf(cmd, sizeof(cmd), "{\"cmd\":\"startTask\",\"data\":{\"id\":%d,\"val\":%s}}",
                pObjParam->id, taskName);
        msgSend(cmd, pOps->msgKey, pConfigParams->mainMsgKey);
    }
    else {
        app_warring("get empty proc failed, id:%d,%s,%s,%s", 
                pObjParam->id, name, pObjParam->subtype, taskName);
    }

    return 0;
}

static int stopTask(const char *name, const char *taskName, objParam *pObjParam) {
    pidOps *pOps;
    char cmd[128];

    pOps = getTaskProc(name, pObjParam->subtype, taskName, pObjParam);
    if(pOps != NULL) {
        snprintf(cmd, sizeof(cmd), "{\"cmd\":\"stopTask\",\"data\":{\"id\":%d,\"val\":%s}}",
                pObjParam->id, taskName);
        msgSend(cmd, pOps->msgKey, 0);
    }
    else {
        app_warring("get task proc failed, id:%d,%s,%s,%s", 
                pObjParam->id, name, pObjParam->subtype, taskName);
    }

    return 0;
}

static int objManager(node_common *p, void *arg) {
    pidOps *pOps;
    int nowSec = (int)(*(__time_t *)arg);
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;

    printf("objManager, id:%d, type:%s, subtype:%s, livestream:%d, capture:%d, record:%d, preview:%s, "
            "liveBeat:%d, captureBeat:%d, recordBeat:%d, nowSec:%d\n", 
            pObjParam->id, pObjParam->type, pObjParam->subtype, pTaskParams->livestream, 
            pTaskParams->capture, pTaskParams->record, pTaskParams->preview, 
            pTaskParams->liveBeat, pTaskParams->captureBeat, pTaskParams->recordBeat, nowSec);
    // check start
    if(pTaskParams->livestream && (nowSec - pTaskParams->liveBeat) > PROC_BEAT_TIMEOUT) {
        startTask("obj", "livestream", pObjParam);
    }
    if(pTaskParams->capture && (nowSec - pTaskParams->captureBeat) > PROC_BEAT_TIMEOUT) {
        startTask("obj", "capture", pObjParam);
    }
    if(pTaskParams->record && (nowSec - pTaskParams->recordBeat) > PROC_BEAT_TIMEOUT) {
        startTask("obj", "record", pObjParam);
    }
    if(strlen(pTaskParams->preview) > 0 && (nowSec - pTaskParams->previewBeat) > PROC_BEAT_TIMEOUT) {
        startTask("obj", "preview", pObjParam);
    }
    if(0) {
        pOps = getEmptyProc("alg", "face", "null", pObjParam);
        if(pOps != NULL) {
        }
    }

    // check stop
    if(pTaskParams->livestream == 0 && pTaskParams->liveBeat > 0) {
        stopTask("obj", "livestream", pObjParam);
    }
    if(pTaskParams->capture == 0 && pTaskParams->captureBeat > 0) {
        stopTask("obj", "capture", pObjParam);
    }
    if(pTaskParams->record == 0 && pTaskParams->recordBeat > 0) {
        stopTask("obj", "record", pObjParam);
    }
    if(strlen(pTaskParams->preview) == 0 && pTaskParams->previewBeat > 0) {
        stopTask("obj", "preview", pObjParam);
    }

    return 0;
}

static void *slave_objmanager_thread(void *arg) {
    struct timeval tv;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;

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

static int createTasks(aiotcParams *pAiotcParams) {
    pthread_t pid;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    if(pConfigParams->masterEnable) {
        creatProcess("master", pAiotcParams);
    }
    if(pConfigParams->masterEnable == 0 || pConfigParams->masterEnable == 2) {
        creatProcess("rest", pAiotcParams);
        if(pthread_create(&pid, NULL, slave_objmanager_thread, pAiotcParams) != 0) {
            app_err("pthread_create slave obj manager thread err");
        }
        else {
            pthread_detach(pid);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
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

    while(pAiotcParams->running) {
        sleep(2);
    }

    destroy(pAiotcParams);

    app_debug("run over");

    return 0;
}

