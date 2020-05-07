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
#include "misc.h"
#include "rest.h"
#include "db.h"
#include "share.h"
#include "shm.h"
#include "system.h"
#include "obj.h"
#include "task.h"

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
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    systemParams *pSystemParams = (systemParams *)pAiotcParams->systemArgs;

    if(pSystemParams->sysOrgData != NULL) {
        shmFree(pShmParams->headsp, pSystemParams->sysOrgData);
    }
    pSystemParams->sysOrgData = (char *)shmMalloc(pShmParams->headsp, strlen(buf) + 1);
    if(pSystemParams->sysOrgData == NULL) {
        app_err("shm malloc failed, %ld", strlen(buf) + 1);
        return ;
    }
    strcpy(pSystemParams->sysOrgData, buf);

    dbWrite(pAiotcParams->dbArgs, "systemInit", "original", buf, 
                        "original.msgOutParams.type", NULL, "mq");
}

static int addSlave(char *buf, aiotcParams *pAiotcParams) {
    node_common node;
    slaveParams *pSlaveParams = (slaveParams *)node.name;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    memset(&node, 0, sizeof(node));
    char *slaveIp = getStrValFromJson(buf, "slaveIp", NULL, NULL);
    if(slaveIp == NULL) {
        return -1;
    }
    strncpy(pSlaveParams->ip, slaveIp, sizeof(pSlaveParams->ip));
    free(slaveIp);

    node.arg = shmMalloc(pShmParams->headsp, strlen(buf) + 1);
    if(node.arg == NULL) {
        app_err("shm malloc failed, %ld", strlen(buf) + 1);
        return -1;
    }
    strcpy((char *)node.arg, buf);
    semWait(&pMasterParams->mutex_slave);
    putToShmQueue(pShmParams->headsp, &pMasterParams->slaveQueue, &node, 1000);
    semPost(&pMasterParams->mutex_slave);

    return 0;
}

static int conditionBySlaveIp(node_common *p, void *arg) {
    char *slaveIp = (char *)arg;
    slaveParams *pSlaveParams = (slaveParams *)p->name;
    return !strcmp(slaveIp, pSlaveParams->ip);
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

static int addObj(char *buf, aiotcParams *pAiotcParams) {
    node_common node;
    objParam *pObjParam = (objParam *)node.name;
    char *name = NULL, *type = NULL, *subtype = NULL;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    memset(&node, 0, sizeof(node));
    name = getStrValFromJson(buf, "name", NULL, NULL);
    type = getStrValFromJson(buf, "type", NULL, NULL);
    subtype = getStrValFromJson(buf, "data", "subtype", NULL);
    pObjParam->id = getIntValFromJson(buf, "id", NULL, NULL);
    if(pObjParam->id < 0 || name == NULL || type == NULL || subtype == NULL) {
        goto end;
    }
    strncpy(pObjParam->name, name, sizeof(pObjParam->name));
    strncpy(pObjParam->type, type, sizeof(pObjParam->type));
    strncpy(pObjParam->subtype, subtype, sizeof(pObjParam->subtype));

    pObjParam->task = shmMalloc(pShmParams->headsp, sizeof(taskParams));
    pObjParam->originaldata = (char *)shmMalloc(pShmParams->headsp, strlen(buf) + 1);
    if(pObjParam->task == NULL || pObjParam->originaldata == NULL) {
        app_err("shm malloc failed");
        goto end;
    }
    memset(pObjParam->task, 0, sizeof(taskParams));
    strcpy((char *)pObjParam->originaldata, buf);
    pObjParam->arg = pAiotcParams;
    semWait(&pMasterParams->mutex_mobj);
    putToShmQueue(pShmParams->headsp, &pMasterParams->mobjQueue, &node, MASTER_OBJ_MAX);
    semPost(&pMasterParams->mutex_mobj);

end:
    if(name != NULL) {
        free(name);
    }
    if(type != NULL) {
        free(type);
    }
    if(subtype != NULL) {
        free(subtype);
    }
    return 0;
}

static int delObj(char *buf, aiotcParams *pAiotcParams) {
    int id;
    node_common *p = NULL;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    //TODO : stop something first
    id = getIntValFromJson(buf, "id", NULL, NULL);
    semWait(&pMasterParams->mutex_mobj);
    delFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    semPost(&pMasterParams->mutex_mobj);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        if(pObjParam->task != NULL) {
            shmFree(pShmParams->headsp, pObjParam->task);
        }
        if(pObjParam->originaldata != NULL) {
            shmFree(pShmParams->headsp, pObjParam->originaldata);
        }
        if(p->arg != NULL) {
            shmFree(pShmParams->headsp, p->arg);
        }
        shmFree(pShmParams->headsp, p);
    }

    return 0;
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
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    if(id < 0) {
        return ;
    }
    addObj(buf, pAiotcParams);
    dbWrite(pAiotcParams->dbArgs, "obj", "original", buf, "original.id", &id, NULL);
}

static void master_request_obj_del(struct evhttp_request *req, void *arg) {
    request_first_stage;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;

    delObj(buf, pAiotcParams);
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
        pTaskParams->livestream = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "task.stream", 1);
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
        pTaskParams->livestream = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "task.stream", 0);
}

static void master_request_preview_start(struct evhttp_request *req, void *arg) {
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
        pTaskParams->preview = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "task.preview", 1);
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
        pTaskParams->preview = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);

    dbUpdateIntById(buf, pAiotcParams->dbArgs, "task.preview", 0);
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
        pTaskParams->record = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);


    dbUpdateIntById(buf, pAiotcParams->dbArgs, "task.record", 1);
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
        pTaskParams->record = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);


    dbUpdateIntById(buf, pAiotcParams->dbArgs, "task.record", 0);
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
        pTaskParams->capture = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);


    dbUpdateIntById(buf, pAiotcParams->dbArgs, "task.capture", 1);
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
        pTaskParams->capture = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);


    dbUpdateIntById(buf, pAiotcParams->dbArgs, "task.capture", 0);
}

static void master_request_alg_support(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_task_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    char *algName = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    algName = getStrValFromJson(buf, "alg", NULL, NULL);
    if(id < 0 || algName == NULL) {
        goto end;
    }
    if(dbExsit(pAiotcParams->dbArgs, "obj", "task.alg", NULL, algName)) {
        printf("start task, obj:%d alg:%s already exsit, ignore it\n", id, algName);
        goto end;
    }
    dbUpdate(pAiotcParams->dbArgs, "obj", 0, "$push", "original.id", &id, NULL, "task.alg", NULL, algName);

end:
    if(algName != NULL) {
        free(algName);
    }
}

static void master_request_task_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    char *algName = NULL;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    algName = getStrValFromJson(buf, "alg", NULL, NULL);
    if(id < 0 || algName == NULL) {
        goto end;
    }
    dbUpdate(pAiotcParams->dbArgs, "obj", 0, "$pull", "original.id", &id, NULL, "task.alg", NULL, algName);

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

static int mstartRestTask(aiotcParams *pAiotcParams) {
    pthread_t pid;

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
    return 0;
}

static int masterUninit(aiotcParams *pAiotcParams) {
    dbClose(pAiotcParams->dbArgs);
    return 0;
}

int masterProcess(void *arg) {
    aiotcParams *pAiotcParams = (aiotcParams *)arg;

    masterInit(pAiotcParams);
    mstartRestTask(pAiotcParams);

    while(pAiotcParams->running) {
        sleep(2);
    }

    masterUninit(pAiotcParams);
    app_debug("run over");

    return 0;
}

