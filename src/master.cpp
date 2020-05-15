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

#include "master.h"
#include "system.h"
#include "misc.h"
#include "rest.h"
#include "db.h"
#include "share.h"
#include "shm.h"
#include "obj.h"
#include "task.h"
#include "alg.h"

static int addSlave(char *buf, aiotcParams *pAiotcParams) {
    node_common node;
    char *slaveIp = NULL, *internetIp = NULL;
    slaveParam *pSlaveParam = (slaveParam *)node.name;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    memset(&node, 0, sizeof(node));
    int restPort = getIntValFromJson(buf, "restPort", NULL, NULL);
    int streamPort = getIntValFromJson(buf, "streamPort", NULL, NULL);
    slaveIp = getStrValFromJson(buf, "slaveIp", NULL, NULL);
    internetIp = getStrValFromJson(buf, "internetIp", NULL, NULL);
    if(slaveIp == NULL || internetIp == NULL || restPort < 0 || streamPort < 0) {
        goto end;
    }
    pSlaveParam->restPort = restPort;
    pSlaveParam->streamPort = streamPort;
    strncpy(pSlaveParam->ip, slaveIp, sizeof(pSlaveParam->ip));
    strncpy(pSlaveParam->internetIp, internetIp, sizeof(pSlaveParam->internetIp));

    node.arg = shmMalloc(pShmParams->headsp, strlen(buf) + 1);
    if(node.arg == NULL) {
        app_err("shm malloc failed, %ld", strlen(buf) + 1);
        goto end;
    }
    strcpy((char *)node.arg, buf);
    semWait(&pMasterParams->mutex_slave);
    putToShmQueue(pShmParams->headsp, &pMasterParams->slaveQueue, &node, 1000);
    semPost(&pMasterParams->mutex_slave);

end:
    if(slaveIp != NULL) {
        free(slaveIp);
    }
    if(internetIp != NULL) {
        free(internetIp);
    }
    return 0;
}

static int delSlave(char *buf, aiotcParams *pAiotcParams) {
    node_common *p = NULL;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    char *slaveIp = getStrValFromJson(buf, "slaveIp", NULL, NULL);
    if(slaveIp == NULL) {
        return -1;
    }
    //TODO : stop something first
    semWait(&pMasterParams->mutex_slave);
    delFromQueue(&pMasterParams->slaveQueue, slaveIp, &p, conditionBySlaveIp);
    semPost(&pMasterParams->mutex_slave);
    if(p != NULL) {
        if(p->arg != NULL) {
            shmFree(pShmParams->headsp, p->arg);
        }
        shmFree(pShmParams->headsp, p);
    }
    free(slaveIp);

    return 0;
}

static int systemInitCB(char *buf, void *arg) {
    char *original = NULL;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    
    original = getObjBufFromJson(buf, "original", NULL, NULL);
    if(original == NULL) {
        goto end;
    }
    systemInits(original, pAiotcParams);

end:
    if(original != NULL) {
        free(original);
    }
    return 0;
}

static int slaveInitCB(char *buf, void *arg) {
    char *original = NULL;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    
    original = getObjBufFromJson(buf, "original", NULL, NULL);
    if(original == NULL) {
        goto end;
    }
    addSlave(original, pAiotcParams);

end:
    if(original != NULL) {
        free(original);
    }
    return 0;
}

static int objInitCB(char *buf, void *arg) {
    char *original = NULL;
    CommonParams params;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    original = getObjBufFromJson(buf, "original", NULL, NULL);
    if(original == NULL) {
        goto end;
    }
    params.arga = &pMasterParams->mutex_mobj;
    params.argb = &pMasterParams->mobjQueue;
    addObj(original, pAiotcParams, pConfigParams->masterObjMax, &params);

end:
    if(original != NULL) {
        free(original);
    }
    return 0;
}

static int initMasterFromDB(aiotcParams *pAiotcParams) {
    dbTraverse(pAiotcParams->dbArgs, "systemInit", pAiotcParams, systemInitCB);
    dbTraverse(pAiotcParams->dbArgs, "slave", pAiotcParams, slaveInitCB);
    dbTraverse(pAiotcParams->dbArgs, "obj", pAiotcParams, objInitCB);

    return 0;
}

static void master_request_login(struct evhttp_request *req, void *arg) {
    request_first_stage;
    char *userName = NULL, *passWord = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    char **ppbody = (char **)pParams->argb;

    *ppbody = (char *)malloc(256);
    userName = getStrValFromJson(buf, "userName", NULL, NULL);
    passWord = getStrValFromJson(buf, "passWord", NULL, NULL);
    if(userName != NULL && passWord != 0 && 
            strcmp(userName, "admin") == 0 && strcmp(passWord, "123456") == 0) { // TODO
        strcpy(*ppbody, "{\"code\":0,\"msg\":\"success\","
                "\"data\":{\"token\":\"987654321abc\",\"validTime\":30}}");
    }
    else {
        strcpy(*ppbody, "{\"code\":-1,\"msg\":\"username or password is incorrect\",\"data\":{}}");
    }

    if(userName != NULL) {
        free(userName);
    }
    if(passWord != NULL) {
        free(passWord);
    }
}

static void master_request_logout(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_system_init(struct evhttp_request *req, void *arg) {
    request_first_stage;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;

    systemInits(buf, pAiotcParams);
    dbWrite(pAiotcParams->dbArgs, "systemInit", "original", buf, 
                        "original.msgOutParams.type", NULL, "mq");
}

static void master_request_slave_add(struct evhttp_request *req, void *arg) {
    request_first_stage;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;

    char *slaveIp = getStrValFromJson(buf, "slaveIp", NULL, NULL);
    if(slaveIp == NULL) {
        return ;
    }
    addSlave(buf, pAiotcParams);
    dbWrite(pAiotcParams->dbArgs, "slave", "original", buf, 
                        "original.slaveIp", NULL, slaveIp);
    free(slaveIp);
}

static void master_request_slave_del(struct evhttp_request *req, void *arg) {
    request_first_stage;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;

    char *slaveIp = getStrValFromJson(buf, "slaveIp", NULL, NULL);
    if(slaveIp == NULL) {
        return ;
    }
    delSlave(buf, pAiotcParams);
    dbDel(pAiotcParams->dbArgs, "slave", "original.slaveIp", NULL, slaveIp);
    free(slaveIp);
}

static void master_request_obj_add(struct evhttp_request *req, void *arg) {
    request_first_stage;
    CommonParams params;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    if(id < 0) {
        return ;
    }
    params.arga = &pMasterParams->mutex_mobj;
    params.argb = &pMasterParams->mobjQueue;
    addObj(buf, pAiotcParams, pConfigParams->masterObjMax, &params);
    dbWrite(pAiotcParams->dbArgs, "obj", "original", buf, "original.id", &id, NULL);
}

static void master_request_obj_del(struct evhttp_request *req, void *arg) {
    request_first_stage;
    CommonParams params;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    params.arga = &pMasterParams->mutex_mobj;
    params.argb = &pMasterParams->mobjQueue;
    delObj(buf, pAiotcParams, &params);
}

static void master_request_stream_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        httpPostSlave("/obj/stream/start", buf, pObjParam);
        pTaskParams->livestream = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "original.task.stream", 1);
}

static void master_request_stream_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        httpPostSlave("/obj/stream/stop", buf, pObjParam);
        pTaskParams->livestream = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "original.task.stream", 0);
}

static void master_request_preview_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    char *type = NULL;
    node_common *p = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    type = getStrValFromJson(buf, "type", NULL, NULL);
    if(id < 0 || type == NULL) {
        goto end;
    }
    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        httpPostSlave("/obj/preview/start", buf, pObjParam);
        strncpy(pTaskParams->preview, type, sizeof(pTaskParams->preview));
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdate(pAiotcParams->dbArgs, "obj", 0, "$set", "original.id", &id, NULL, "original.task.preview", NULL, type);

end:
    if(type != NULL) {
        free(type);
    }
}

static void master_request_preview_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        httpPostSlave("/obj/preview/stop", buf, pObjParam);
        memset(pTaskParams->preview, 0, sizeof(pTaskParams->preview));
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdate(pAiotcParams->dbArgs, "obj", 0, "$set", "original.id", &id, NULL, "original.task.preview", NULL, "");
}

static void master_request_record_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        httpPostSlave("/obj/record/start", buf, pObjParam);
        pTaskParams->record = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "original.task.record", 1);
}

static void master_request_record_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        httpPostSlave("/obj/record/stop", buf, pObjParam);
        pTaskParams->record = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "original.task.record", 0);
}

static void master_request_record_play(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_capture_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        httpPostSlave("/obj/capture/start", buf, pObjParam);
        pTaskParams->capture = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "original.task.capture", 1);
}

static void master_request_capture_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        httpPostSlave("/obj/capture/stop", buf, pObjParam);
        pTaskParams->capture = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "original.task.capture", 0);
}

static void master_request_alg_support(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_task_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    char *algName = NULL;
    CommonParams params;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    algName = getStrValFromJson(buf, "alg", NULL, NULL);
    if(id < 0 || algName == NULL) {
        goto end;
    }
    if(dbExsit(pAiotcParams->dbArgs, "obj", "original.task.alg", NULL, algName)) {
        printf("start task, obj:%d alg:%s already exsit, ignore it\n", id, algName);
        goto end;
    }

    memset(&params, 0, sizeof(params));
    params.arga = &pMasterParams->mutex_mobj;
    params.argb = &pMasterParams->mobjQueue;
    params.argc = &params;
    addAlg(buf, id, algName, pAiotcParams, &params);
    dbUpdate(pAiotcParams->dbArgs, "obj", 0, "$push", "original.id", &id, NULL, "original.task.alg", NULL, algName);

end:
    if(algName != NULL) {
        free(algName);
    }
}

static void master_request_task_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    char *algName = NULL;
    CommonParams params;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    algName = getStrValFromJson(buf, "alg", NULL, NULL);
    if(id < 0 || algName == NULL) {
        goto end;
    }

    memset(&params, 0, sizeof(params));
    params.arga = &pMasterParams->mutex_mobj;
    params.argb = &pMasterParams->mobjQueue;
    params.argc = &params;
    delAlg(buf, id, algName, pAiotcParams, &params);
    dbUpdate(pAiotcParams->dbArgs, "obj", 0, "$pull", "original.id", &id, NULL, "original.task.alg", NULL, algName);

end:
    if(algName != NULL) {
        free(algName);
    }
}

static urlMap master_url_map[] = {
    {"/system/login",       master_request_login},
    {"/system/logout",      master_request_logout},
    {"/system/init",        master_request_system_init},
    {"/system/slave/add",   master_request_slave_add},
    {"/system/slave/del",   master_request_slave_del},
    {"/obj/add/tcp",        master_request_obj_add},
    {"/obj/add/ehome",      master_request_obj_add},
    {"/obj/add/gat1400",    master_request_obj_add},
    {"/obj/add/rtsp",       master_request_obj_add},
    {"/obj/add/gb28181",    master_request_obj_add},
    {"/obj/add/sdk",        master_request_obj_add},
    {"/obj/del",            master_request_obj_del},
    {"/obj/stream/start",   master_request_stream_start},
    {"/obj/stream/stop",    master_request_stream_stop},
    {"/obj/preview/start",  master_request_preview_start},
    {"/obj/preview/stop",   master_request_preview_stop},
    {"/obj/record/start",   master_request_record_start},
    {"/obj/record/stop",    master_request_record_stop},
    {"/obj/record/play",    master_request_record_play},
    {"/obj/capture/start",  master_request_capture_start},
    {"/obj/capture/stop",   master_request_capture_stop},
    {"/alg/support",        master_request_alg_support},
    {"/task/start",         master_request_task_start},
    {"/task/stop",          master_request_task_stop},
    {NULL, NULL}
};

static void *master_restapi_thread(void *arg) {
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    ev_uint16_t port = pConfigParams->masterRestPort;

    //sleep(20); //TODO
    app_debug("%s:%d starting ...", pAiotcParams->localIp, port);
    http_task(master_url_map, port, pAiotcParams);

    app_debug("run over");

    return NULL;
}

static int detachObjSlave(node_common *p, void *arg) {
    char *slaveIp = (char *)arg;
    objParam *pObjParam = (objParam *)p->name;

    if(pObjParam->slave != NULL && pObjParam->attachSlave) {
        slaveParam *pSlaveParam = (slaveParam *)pObjParam->slave;
        if(!strncmp(slaveIp, pSlaveParam->ip, sizeof(pSlaveParam->ip))) {
            pObjParam->attachSlave = 0;
        }
    }

    return 0;
}

static int detachSlave(aiotcParams *pAiotcParams, slaveParam *pSlaveParam) {
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    semWait(&pMasterParams->mutex_mobj);
    traverseQueue(&pMasterParams->mobjQueue, pSlaveParam->ip, detachObjSlave);
    semPost(&pMasterParams->mutex_mobj);

    return 0;
}

static int slaveLoad(node_common *p, void *arg) {
    char url[256];
    char ack[256] = {0};
    struct timeval tv;
    httpAckParams ackParam;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    slaveParam *pSlaveParam = (slaveParam *)p->name;
    systemParams *pSystemParams = (systemParams *)pAiotcParams->systemArgs;

    ackParam.buf = ack;
    ackParam.max = sizeof(ack);
    snprintf(url, sizeof(url), "http://%s:%d/system/slave", pSlaveParam->ip, pSlaveParam->restPort);
    httpGet(url, &ackParam, 3);

    int status = strstr(ack,"systemInit") != NULL ? 1 : 0;
    if(!status) {
        if(!pSlaveParam->offline) {
            gettimeofday(&tv, NULL);
            pSlaveParam->offline = (int)tv.tv_sec;
            pSlaveParam->systemInit = 0;
            detachSlave(pAiotcParams, pSlaveParam);
            app_warring("detected offline, %s:%d", pSlaveParam->ip, pSlaveParam->restPort);
        }
        if(pSlaveParam->online) {
            pSlaveParam->online = 0;
        }
    }
    else {
        int systemInit = getIntValFromJson(ack, "data", "systemInit", NULL);
        if(systemInit == 0) {
            if(!pSlaveParam->online) {
                app_debug("detected online, %s:%d", pSlaveParam->ip, pSlaveParam->restPort);
                gettimeofday(&tv, NULL);
                pSlaveParam->online = (int)tv.tv_sec;
            }
            if(pSlaveParam->offline) {
                pSlaveParam->offline = 0;
            }
            if(pSystemParams->sysOrgData != NULL) {
                snprintf(url, sizeof(url), "http://%s:%d/system/init", pSlaveParam->ip, pSlaveParam->restPort);
                httpPost(url, pSystemParams->sysOrgData, NULL, 3);
                if(!pSlaveParam->systemInit) {
                    pSlaveParam->systemInit = 1;
                }
            }
        }
        pSlaveParam->load = getIntValFromJson(ack, "data", "load", NULL);
    }
    //printf("slave, ip:%s, online:%d, offline:%d, load:%d, ack:%s\n", 
    //        pSlaveParam->ip, pSlaveParam->online, pSlaveParam->offline, pSlaveParam->load, ack);

    return 0;
}

static void *slave_load_thread(void *arg) {
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    while(pAiotcParams->running) {
        semWait(&pMasterParams->mutex_slave);
        traverseQueue(&pMasterParams->slaveQueue, pAiotcParams, slaveLoad);
        semPost(&pMasterParams->mutex_slave);
        if(!pMasterParams->slaveLoadOk) {
            pMasterParams->slaveLoadOk = 1;
        }
        sleep(5);
    }

    app_debug("run over");

    return NULL;
}

// TODO : load应该与obj类型关联
static int _getlowestLoadSlave(node_common *p, void *arg) {
    slaveParam **ppLowestSlave = (slaveParam **)arg;
    slaveParam *pSlaveParam = (slaveParam *)p->name;

    if(*ppLowestSlave == NULL) {
        if(pSlaveParam->online && pSlaveParam->systemInit && pSlaveParam->load < SLAVE_LOAD_MAX) {
            *ppLowestSlave = pSlaveParam;
        }
    }
    else {
        slaveParam *pSlaveLast = *ppLowestSlave;
        if(pSlaveParam->load < pSlaveLast->load && pSlaveParam->online && pSlaveParam->systemInit) {
            *ppLowestSlave = pSlaveParam;
        }
    }

    return 0;
}

slaveParam *getLowestLoadSlave(objParam *pObjParam) {
    slaveParam *pLowestSlave = NULL;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    semWait(&pMasterParams->mutex_slave);
    traverseQueue(&(pMasterParams->slaveQueue), &pLowestSlave, _getlowestLoadSlave);
    semPost(&pMasterParams->mutex_slave);

    return pLowestSlave;
}

static int slaveIsNormal(int id, slaveParam *pSlaveParam) {
    int normal = 1;
    struct timeval tv;

    if(pSlaveParam->offline || pSlaveParam->systemInit == 0 || pSlaveParam->load >= SLAVE_LOAD_MAX) {
        normal = -1;
        if(pSlaveParam->offline) {
            gettimeofday(&tv, NULL);
            int sec = (int)tv.tv_sec - pSlaveParam->offline;
            if(sec < 120) {
                printf("objId:%d, old slave %s is offline, wait %d seconds ...\n", 
                        id, pSlaveParam->ip, 120 - sec);
                normal = 0;
            }
        }
    }

    return normal;
}

// slave离线的等待一定时间,如果还未上线,重新分配相关obj slave
// old slave负载有空闲优先
// 其次分配到最空闲slave
static int mobjManager(node_common *p, void *arg) {
    int redirect = 0;
    objParam *pObjParam = (objParam *)p->name;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;

    if(pObjParam->slave != NULL && pObjParam->attachSlave == 0) {
        int normal;
        slaveParam *pSlave = (slaveParam *)pObjParam->slave;
        normal = slaveIsNormal(pObjParam->id, pSlave);
        if(normal == 1) {
            redirect = 1;
            pObjParam->attachSlave = 1;
            app_debug("redirect obj %d to old slave : %s", pObjParam->id, pSlave->ip);
        }
        else if(normal == -1) {
            pObjParam->slave = NULL;
        }
    }
    else if(pObjParam->slave == NULL) {
        slaveParam *pSlave = getLowestLoadSlave(pObjParam);
        if(pSlave != NULL) {
            redirect = 1;
            pObjParam->slave = pSlave;
            pObjParam->attachSlave = 1;
            dbUpdate(pAiotcParams->dbArgs, "obj", 0, "$set", 
                    "original.id", &pObjParam->id, NULL, "original.slave.ip", NULL, pSlave->ip);
            app_debug("redirect obj %d to new slave : %s", pObjParam->id, pSlave->ip);
        }
        else {
            printf("obj manager, id:%d, get lowest slave failed\n", pObjParam->id);
        }
    }

    if(redirect) {
        char url[256];
        char *originaldata;
        slaveParam *pSlaveParam = (slaveParam *)pObjParam->slave;

        originaldata = delObjJson(pObjParam->originaldata, "slave", NULL, NULL);
        if(originaldata != NULL) {
            snprintf(url, sizeof(url), "http://%s:%d/obj/add", pSlaveParam->ip, pSlaveParam->restPort);
            httpPost(url, originaldata, NULL, 3);
            free(originaldata);
        }
    }

    return 0;
}

static void *master_objmanager_thread(void *arg) {
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    while(!pMasterParams->slaveLoadOk) {
        sleep(1);
    }
    printf("slave load ok\n");

    while(pAiotcParams->running) {
        semWait(&pMasterParams->mutex_mobj);
        traverseQueue(&pMasterParams->mobjQueue, NULL, mobjManager);
        semPost(&pMasterParams->mutex_mobj);
        sleep(5);
    }

    app_debug("run over");

    return NULL;
}

static int mstartTask(aiotcParams *pAiotcParams) {
    pthread_t pid;

    if(pthread_create(&pid, NULL, slave_load_thread, pAiotcParams) != 0) {
        app_err("pthread_create slave load thread err");
    }
    else {
        pthread_detach(pid);
    }

    if(pthread_create(&pid, NULL, master_objmanager_thread, pAiotcParams) != 0) {
        app_err("pthread_create master obj manager thread err");
    }
    else {
        pthread_detach(pid);
    }

    if(pthread_create(&pid, NULL, master_restapi_thread, pAiotcParams) != 0) {
        app_err("pthread_create master rest api thread err");
    }
    else {
        pthread_detach(pid);
    }

    return 0;
}

static int masterInit(aiotcParams *pAiotcParams) {
    dbOpen(pAiotcParams->dbArgs);
    initMasterFromDB(pAiotcParams);

    return 0;
}

static int masterUninit(aiotcParams *pAiotcParams) {
    dbClose(pAiotcParams->dbArgs);
    return 0;
}

int masterProcess(void *arg) {
    aiotcParams *pAiotcParams = (aiotcParams *)arg;

    masterInit(pAiotcParams);
    mstartTask(pAiotcParams);

    while(pAiotcParams->running) {
        sleep(2);
    }

    masterUninit(pAiotcParams);
    app_debug("run over");

    return 0;
}

