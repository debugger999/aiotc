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
#include "alg.h"

static int conditionBySlaveIp(node_common *p, void *arg) {
    char *slaveIp = (char *)arg;
    slaveParams *pSlaveParams = (slaveParams *)p->name;
    return !strcmp(slaveIp, pSlaveParams->ip);
}

static int addSlave(char *buf, aiotcParams *pAiotcParams) {
    node_common node;
    char *slaveIp = NULL, *internetIp = NULL;
    slaveParams *pSlaveParams = (slaveParams *)node.name;
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
    pSlaveParams->restPort = restPort;
    pSlaveParams->streamPort = streamPort;
    strncpy(pSlaveParams->ip, slaveIp, sizeof(pSlaveParams->ip));
    strncpy(pSlaveParams->internetIp, internetIp, sizeof(pSlaveParams->internetIp));

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

static int addObj(char *buf, aiotcParams *pAiotcParams) {
    node_common node;
    taskParams *pTaskParams;
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
    pTaskParams = (taskParams *)pObjParam->task;
    if(sem_init(&pTaskParams->mutex_alg, 1, 1) < 0) {
      app_err("sem init failed");
      goto end;
    }
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
            taskParams *pTaskParams = (taskParams *)pObjParam->task;
            sem_destroy(&pTaskParams->mutex_alg);
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

static int addAlg(int id, char *algName, aiotcParams *pAiotcParams) {
    node_common node;
    node_common *p = NULL;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    memset(&node, 0, sizeof(node));
    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        algParams *pAlgParams = (algParams *)node.name;
        strncpy(pAlgParams->name, algName, sizeof(pAlgParams->name));
        semWait(&pTaskParams->mutex_alg);
        putToShmQueue(pShmParams->headsp, &pTaskParams->algQueue, &node, 100);
        semPost(&pTaskParams->mutex_alg);
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pMasterParams->mutex_mobj);
    
    return 0;
}

static int conditionByAlgName(node_common *p, void *arg) {
    char *algName = (char *)arg;
    algParams *pAlgParams = (algParams *)p->name;
    return !strncmp(algName, pAlgParams->name, sizeof(pAlgParams->name));
}

static int delAlg(int id, char *algName, aiotcParams *pAiotcParams) {
    node_common *p = NULL, *palg = NULL;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    semWait(&pMasterParams->mutex_mobj);
    searchFromQueue(&pMasterParams->mobjQueue, &id, &p, conditionByObjId);
    if(p == NULL) {
        printf("objId %d not exsit\n", id);
        semPost(&pMasterParams->mutex_mobj);
        return -1;
    }
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    semWait(&pTaskParams->mutex_alg);
    delFromQueue(&pTaskParams->algQueue, algName, &palg, conditionByAlgName);
    semPost(&pTaskParams->mutex_alg);
    if(palg != NULL) {
        if(palg->arg != NULL) {
            shmFree(pShmParams->headsp, palg->arg);
        }
        shmFree(pShmParams->headsp, palg);
    }
    semPost(&pMasterParams->mutex_mobj);
    
    return 0;
}

static int systemInit(char *buf, aiotcParams *pAiotcParams) {
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    systemParams *pSystemParams = (systemParams *)pAiotcParams->systemArgs;

    if(pSystemParams->sysOrgData != NULL) {
        shmFree(pShmParams->headsp, pSystemParams->sysOrgData);
    }
    pSystemParams->sysOrgData = (char *)shmMalloc(pShmParams->headsp, strlen(buf) + 1);
    if(pSystemParams->sysOrgData == NULL) {
        app_err("shm malloc failed, %ld", strlen(buf) + 1);
        return -1;
    }
    strcpy(pSystemParams->sysOrgData, buf);

    return 0;
}

static int systemInitCB(char *buf, void *arg) {
    char *original = NULL;
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    
    original = getObjBufFromJson(buf, "original", NULL, NULL);
    if(original == NULL) {
        goto end;
    }
    systemInit(original, pAiotcParams);

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
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    
    original = getObjBufFromJson(buf, "original", NULL, NULL);
    if(original == NULL) {
        goto end;
    }
    addObj(original, pAiotcParams);

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

    systemInit(buf, pAiotcParams);
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
    addAlg(id, algName, pAiotcParams);
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
    delAlg(id, algName, pAiotcParams);
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

static int slaveLoad(node_common *p, void *arg) {
    char url[256];
    char ack[256] = {0};
    httpAckParams ackParam;
    slaveParams *pSlaveParams = (slaveParams *)p->name;

    ackParam.buf = ack;
    ackParam.max = sizeof(ack);
    snprintf(url, sizeof(ack), "http://%s:%d/system/slave/load", pSlaveParams->ip, pSlaveParams->restPort);
    httpGet(url, &ackParam, 3);

    int status = strstr(ack,"load") != NULL ? 1 : 0;
    if(status) {
        pSlaveParams->load = getIntValFromJson(ack, "data", "load", NULL);
        if(!pSlaveParams->online) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            pSlaveParams->online = (int)tv.tv_sec;
            app_debug("detected online, %s:%d", pSlaveParams->ip, pSlaveParams->restPort);
        }
        if(pSlaveParams->offline) {
            pSlaveParams->offline = 0;
        }
    }
    else {
        if(!pSlaveParams->offline) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            pSlaveParams->offline = (int)tv.tv_sec;
            app_warring("detected offline, %s:%d", pSlaveParams->ip, pSlaveParams->restPort);
        }
        if(pSlaveParams->online) {
            pSlaveParams->online = 0;
        }
    }
    //printf("slave, ip:%s, online:%d, offline:%d, load:%d, ack:%s\n", 
    //        pSlaveParams->ip, pSlaveParams->online, pSlaveParams->offline, pSlaveParams->load, ack);

    return 0;
}

static void *slave_load_thread(void *arg) {
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    while(pAiotcParams->running) {
        semWait(&pMasterParams->mutex_slave);
        traverseQueue(&pMasterParams->slaveQueue, pAiotcParams, slaveLoad);
        semPost(&pMasterParams->mutex_slave);
        sleep(1);
    }

    app_debug("run over");

    return NULL;
}

// TODO : load应该与obj类型关联
static int _getlowestLoadSlave(node_common *p, void *arg) {
    slaveParams **ppLowestSlave = (slaveParams **)arg;
    slaveParams *pSlaveParams = (slaveParams *)p->name;

    if(*ppLowestSlave == NULL) {
        if(pSlaveParams->online && pSlaveParams->systemInit && pSlaveParams->load < 100) {
            *ppLowestSlave = pSlaveParams;
        }
    }
    else {
        /*
        slaveParams *pSlaveParamsLast = (slaveParams *)pCamera->arg2;
        loadParams *pLoadParamsLast = &(pSlaveParamsLast->loadParam);
        if(pLoadParams->objNum < pLoadParamsLast->objNum && pLoadParams->videoLoad < 100 && 
                pLoadParams->captureLoad < 100 && pSlaveParams->status && pSlaveParams->systemInit) {
            pCamera->arg2 = pSlaveParams;
        }
        */
    }

    return 0;
}

slaveParams *getLowestLoadSlave(objParam *pObjParam) {
    slaveParams *pLowestSlave = NULL;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    semWait(&pMasterParams->mutex_slave);
    traverseQueue(&(pMasterParams->slaveQueue), &pLowestSlave, _getlowestLoadSlave);
    semPost(&pMasterParams->mutex_slave);

    return pLowestSlave;
}

// slave离线的等待一定时间,如果还未上线,重新分配相关obj slave
// old slave负载有空闲优先
// 其次分配到最空闲slave
static int objManager(node_common *p, void *arg) {
    objParam *pObjParam = (objParam *)p->name;

    if(pObjParam->slave != NULL && pObjParam->attachSlave == 0) { // TODO
    }
    else if(pObjParam->slave == NULL) {
        pObjParam->slave = getLowestLoadSlave(pObjParam);
    }
    //printf("obj, type:%s, subtype:%s, id:%d\n", pObjParam->type, pObjParam->subtype, pObjParam->id);

    return 0;
}

static void *master_objmanager_thread(void *arg) {
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    while(pAiotcParams->running) {
        semWait(&pMasterParams->mutex_mobj);
        traverseQueue(&pMasterParams->mobjQueue, pAiotcParams, objManager);
        semPost(&pMasterParams->mutex_mobj);
        sleep(1);
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

