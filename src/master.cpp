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

static void master_request_login(struct evhttp_request *req, void *arg) {
    request_first_stage;
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

static void master_request_slave_add(struct evhttp_request *req, void *arg) {
    request_first_stage;
    CommonParams *pParams = (CommonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;

    char *slaveIp = getStrValFromJson(buf, "slaveIp", NULL, NULL);
    if(slaveIp == NULL) {
        return ;
    }
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
    dbWrite(pAiotcParams->dbArgs, "obj", "original", buf, "original.id", &id, NULL);
}

static void master_request_obj_del(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_stream_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_stream_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_preview_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_preview_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_record_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_record_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_record_play(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_capture_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_capture_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_alg_support(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_task_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void master_request_task_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
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

