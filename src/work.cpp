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
#include "work.h"
#include "misc.h"
#include "system.h"
#include "shm.h"
#include "obj.h"
#include "cJSON.h"

char *makeJson(outJsonParams *pJsonParams) {
    char *json;
    cJSON *dataRoot, *sceneRoot, *personRoot, *vehRoot;
    cJSON *personRt, *faceRoot, *bodyRoot, *rectRoot;
    cJSON *root, *vehRt, *plateRoot;

    root =  cJSON_CreateObject();
    dataRoot =  cJSON_CreateObject(); 
    sceneRoot =  cJSON_CreateObject(); 
    personRoot =  cJSON_CreateArray();
    vehRoot =  cJSON_CreateArray();
    cJSON_AddStringToObject(root, "msgType", pJsonParams->msgType);
    cJSON_AddItemToObject(root, "data", dataRoot);
    cJSON_AddNumberToObject(dataRoot, "id", pJsonParams->id);
    cJSON_AddNumberToObject(dataRoot, "timestamp", pJsonParams->timestamp);
    cJSON_AddItemToObject(dataRoot, "sceneImg", sceneRoot);
    cJSON_AddItemToObject(dataRoot, "person", personRoot);
    cJSON_AddItemToObject(dataRoot, "veh", vehRoot);
    
    if(pJsonParams->sceneUrl[0] != '\0') {
        cJSON_AddStringToObject(sceneRoot, "url", pJsonParams->sceneUrl);
    }

    if(pJsonParams->faceUrl[0] != '\0' || pJsonParams->personBodyUrl[0] != '\0') {
        personRt = cJSON_CreateObject(); 
        cJSON_AddItemToArray(personRoot, personRt);
    }
    if(pJsonParams->faceUrl[0] != '\0') {
        faceRoot = cJSON_CreateObject();
        rectRoot = cJSON_CreateObject();
        cJSON_AddItemToObject(personRt, "face", faceRoot);
        cJSON_AddItemToObject(faceRoot, "rect", rectRoot);
        cJSON_AddStringToObject(faceRoot, "url", pJsonParams->faceUrl);
        cJSON_AddNumberToObject(rectRoot, "x", pJsonParams->faceRect.x);
        cJSON_AddNumberToObject(rectRoot, "y", pJsonParams->faceRect.y);
        cJSON_AddNumberToObject(rectRoot, "w", pJsonParams->faceRect.w);
        cJSON_AddNumberToObject(rectRoot, "h", pJsonParams->faceRect.h);
    }
    if(pJsonParams->personBodyUrl[0] != '\0') {
        bodyRoot = cJSON_CreateObject();
        rectRoot = cJSON_CreateObject();
        cJSON_AddItemToObject(personRt, "body", bodyRoot);
        cJSON_AddItemToObject(bodyRoot, "rect", rectRoot);
        cJSON_AddStringToObject(bodyRoot, "url", pJsonParams->personBodyUrl);
        cJSON_AddNumberToObject(rectRoot, "x", pJsonParams->personBodyRect.x);
        cJSON_AddNumberToObject(rectRoot, "y", pJsonParams->personBodyRect.y);
        cJSON_AddNumberToObject(rectRoot, "w", pJsonParams->personBodyRect.w);
        cJSON_AddNumberToObject(rectRoot, "h", pJsonParams->personBodyRect.h);
    }

    if(pJsonParams->vehBodyUrl[0] != '\0' || pJsonParams->plateUrl[0] != '\0') {
        vehRt = cJSON_CreateObject(); 
        cJSON_AddItemToArray(vehRoot, vehRt);
    }
    if(pJsonParams->vehBodyUrl[0] != '\0') {
        bodyRoot = cJSON_CreateObject();
        rectRoot = cJSON_CreateObject();
        cJSON_AddItemToObject(vehRt, "body", bodyRoot);
        cJSON_AddItemToObject(bodyRoot, "rect", rectRoot);
        cJSON_AddStringToObject(bodyRoot, "url", pJsonParams->vehBodyUrl);
        cJSON_AddNumberToObject(rectRoot, "x", pJsonParams->vehBodyRect.x);
        cJSON_AddNumberToObject(rectRoot, "y", pJsonParams->vehBodyRect.y);
        cJSON_AddNumberToObject(rectRoot, "w", pJsonParams->vehBodyRect.w);
        cJSON_AddNumberToObject(rectRoot, "h", pJsonParams->vehBodyRect.h);
    }
    if(pJsonParams->plateUrl[0] != '\0') {
        plateRoot = cJSON_CreateObject();
        rectRoot = cJSON_CreateObject();
        cJSON_AddItemToObject(vehRt, "plate", plateRoot);
        cJSON_AddItemToObject(plateRoot, "rect", rectRoot);
        cJSON_AddStringToObject(plateRoot, "url", pJsonParams->plateUrl);
        cJSON_AddNumberToObject(rectRoot, "x", pJsonParams->plateRect.x);
        cJSON_AddNumberToObject(rectRoot, "y", pJsonParams->plateRect.y);
        cJSON_AddNumberToObject(rectRoot, "w", pJsonParams->plateRect.w);
        cJSON_AddNumberToObject(rectRoot, "h", pJsonParams->plateRect.h);
        if(pJsonParams->plateNo[0] != '\0') {
            cJSON_AddStringToObject(plateRoot, "plateNo", pJsonParams->plateNo);
        }
        if(pJsonParams->plateColor[0] != '\0') {
            cJSON_AddStringToObject(plateRoot, "color", pJsonParams->plateColor);
        }
    }

    json = cJSON_Print(root);
    cJSON_Delete(root);

    return json;
}

static int checkCallback(node_common *p, void *arg) {
    objParam *pObjParam = (objParam *)p->name;
    checkObjDir(pObjParam);
    return 0;
}

static int checkObjDirs(workParams *pWorkParams) {
    aiotcParams *pAiotcParams = (aiotcParams *)pWorkParams->arg;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;

    semWait(&pObjParams->mutex_obj);
    traverseQueue(&pObjParams->objQueue, NULL, checkCallback);
    semPost(&pObjParams->mutex_obj);

    return 0;
}

static int checkWorkdir(workParams *pWorkParams) {
    char date[32];
    char path[256];
    struct tm _time;
    struct timeval tv;
    aiotcParams *pAiotcParams = (aiotcParams *)pWorkParams->arg;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &_time);
    snprintf(date, sizeof(date), "%d%02d%02d", _time.tm_year + 1900, _time.tm_mon + 1, _time.tm_mday);
    if(strncmp(pWorkParams->date, date, sizeof(date)) != 0) {
        snprintf(path, sizeof(path), "%s/webpages/img/%s", pConfigParams->workdir, date);
        dirCheck(path);
        strncpy(pWorkParams->date, date, sizeof(date));
        checkObjDirs(pWorkParams);
    }
    
    return 0;
}

int copyToMqQueue(char *json, workParams *pWorkParams) {
    int ret;
    node_common node;
    aiotcParams *pAiotcParams = (aiotcParams *)pWorkParams->arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    outParams *pOutParams = (outParams *)&pWorkParams->outParam;
    int len = strlen(json)/1024*1024 + 1024;

    memset(&node, 0, sizeof(node));
    node.arg = shmMalloc(pShmParams->headsp, len);
    if(node.arg == NULL) {
        app_err("shm malloc failed, len:%d", len);
        return -1;
    }
    strncpy((char *)node.arg, json, len);
    sem_wait(&pOutParams->mutex_out);
    ret = putToShmQueue(pShmParams->headsp, &pOutParams->pOutQueue, &node, 1000);
    sem_post(&pOutParams->mutex_out);
    if(ret != 0) {
        shmFree(pShmParams->headsp, node.arg);
    }

    return 0;
}

static int mqOpen(amqp_connection_state_t *ppConn, mqOutParams *pMqParams, aiotcParams *pAiotcParams) {
    int size = 0;
    int wait = 10;
    char *mqout = NULL, *arrbuf = NULL;
    systemParams *pSystemParams = (systemParams *)pAiotcParams->systemArgs;

    if(pMqParams->port == 0) {
        do {
            if(pSystemParams->sysOrgData != NULL) {
                break;
            }
            sleep(1);
        } while(wait--);
        if(pSystemParams->sysOrgData == NULL) {
            app_warning("please system init first");
            goto end;
        }

        mqout = getArrayBufFromJson(pSystemParams->sysOrgData, "msgOutParams", NULL, NULL, size);
        if(mqout == NULL) {
            goto end;
        }
        arrbuf = getBufFromArray(mqout, NULL, NULL, NULL, 0);
        if(arrbuf == NULL) {
            goto end;
        }
        initMqParams(pMqParams, arrbuf);
        if(pMqParams->port == 0) {
            goto end;
        }
    }

    mqOpenConnect(pMqParams, ppConn, 3);
    if(*ppConn == NULL) {
        static int cnt = 0;
        if(cnt ++ % 1000 == 0) {
            app_err("mq open %s:%d failed", pMqParams->host, pMqParams->port);
        }
	}
    else {
        app_debug("mq open %s:%d success", pMqParams->host, pMqParams->port);
    }

end:
    if(mqout != NULL) {
        free(mqout);
    }
    if(arrbuf != NULL) {
        free(arrbuf);
    }
    return 0;
}

static void *output_thread(void *arg) {
    char *json;
    node_common *p;
    pidOps *pOps = (pidOps *)arg;
    amqp_connection_state_t mq_conn = NULL;
    taskOps *pTaskOps = (taskOps *)pOps->procTaskOps;
    workParams *pWorkParams = (workParams *)pTaskOps->arg;
    outParams *pOutParams = (outParams *)&pWorkParams->outParam;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    mqOutParams *pMqParams = &(pOutParams->mqOutParam);

    mqOpen(&mq_conn, pMqParams, pAiotcParams);
    while(pAiotcParams->running && pOps->running) {
        p = NULL;
        sem_wait(&pOutParams->mutex_out);
        getFromQueue(&pOutParams->pOutQueue, &p);
        sem_post(&pOutParams->mutex_out);
        if(p != NULL) {
            json = (char *)p->arg;
            //printf("out:%s\n", json);
            if(mq_conn != NULL) {
                sendMsg2Mq(pMqParams, &mq_conn, json);
            }
            else {
                mqOpen(&mq_conn, pMqParams, pAiotcParams);
            }
            shmFree(pShmParams->headsp, p->arg); 
            shmFree(pShmParams->headsp, p); 
        }
        usleep(50000);
    }

    if(mq_conn != NULL) {
        mqCloseConnect(mq_conn);
    }
    app_debug("run over");

    return NULL;
}

int workProcInit(void *arg) {
    taskOps *pTaskOps;
    outParams *pOutParams;
    workParams *pWorkParams;
    pidOps *pOps = (pidOps *)arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    pOps->procTaskOps = shmMalloc(pShmParams->headsp, sizeof(taskOps));
    if(pOps->procTaskOps == NULL) {
        app_err("shm malloc failed");
        return -1;
    }
    memset(pOps->procTaskOps, 0, sizeof(taskOps));
    pTaskOps = (taskOps *)pOps->procTaskOps;
    pTaskOps->arg = shmMalloc(pShmParams->headsp, sizeof(workParams));
    if(pTaskOps->arg == NULL) {
        app_err("shm malloc failed");
        shmFree(pShmParams->headsp, pOps->procTaskOps);
        return -1;
    }
    memset(pTaskOps->arg, 0, sizeof(workParams));
    pWorkParams = (workParams *)pTaskOps->arg;
    pOutParams = (outParams *)&pWorkParams->outParam;

    pWorkParams->arg = pAiotcParams;
    pAiotcParams->workArgs = pWorkParams;
    if(sem_init(&pOutParams->mutex_out, 1, 1) < 0) {
        app_err("sem_init failed");
        shmFree(pShmParams->headsp, pOps->procTaskOps);
        shmFree(pShmParams->headsp, pTaskOps->arg);
        pTaskOps->arg = NULL;
        return -1;
    }

    return 0;
}

static int workInit(pidOps *pOps) {
    pthread_t pid;

    if(pthread_create(&pid, NULL, output_thread, pOps) != 0) {
        app_err("create output thread failed");
    }
    else {
        pthread_detach(pid);
    }

    return 0;
}

int workProcess(void *arg) {
    int cnt = 0;
    pidOps *pOps = (pidOps *)arg;
    taskOps *pTaskOps = (taskOps *)pOps->procTaskOps;
    workParams *pWorkParams = (workParams *)pTaskOps->arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;

    pOps = getRealOps(pOps);
    if(pOps == NULL) {
        return -1;
    }
    pOps->running = 1;

    workInit(pOps);

    while(pAiotcParams->running && pOps->running) {
        if(cnt % 300 == 0) {
            checkWorkdir(pWorkParams);
        }
        cnt ++;
        sleep(2);
    }
    pOps->running = 0;

    app_debug("pid:%d, run over", pOps->pid);

    return 0;
}

