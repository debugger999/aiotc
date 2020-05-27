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

#include "rest.h"
#include "db.h"
#include "misc.h"
#include "system.h"
#include "typedef.h"
#include "obj.h"
#include "task.h"
#include "slave.h"
#include "pids.h"

static void closeCallback(struct evhttp_connection* connection, void* arg) {
    if(arg != NULL) {
        event_base_loopexit((struct event_base*)arg, NULL);
    }
}

static void readCallback(struct evhttp_request* req, void* arg) {
    ev_ssize_t len;
    struct evbuffer *evbuf;
    unsigned char *buf;
    httpAckParams *pAckParams = (httpAckParams *)arg;
    int max = pAckParams->max;
    struct event_base *base = (struct event_base *)pAckParams->arg;

    if(req == NULL) {
        app_warring("req is null");
        return ;
    }
    evbuf = evhttp_request_get_input_buffer(req);
    len = evbuffer_get_length(evbuf);
    if(len < max) {
        buf = evbuffer_pullup(evbuf, len);
        if(buf != NULL) {
            memcpy(pAckParams->buf, buf, len);
            pAckParams->buf[len] = '\0';
        }
    }
    else {
        printf("warring : http read size exception, %ld:%d\n", len, max);
    }
    evbuffer_drain(evbuf, len);

    event_base_loopexit(base, NULL);
}

static void errCallback(enum evhttp_request_error error, void* arg) {
    httpAckParams *pAckParams = (httpAckParams *)arg;
    struct event_base *base = NULL;

    if(pAckParams != NULL) {
        base = (struct event_base *)pAckParams->arg;
        if(base) {
            event_base_loopexit(base, NULL);
        }
    }
}

static int httpClient(enum evhttp_cmd_type httpCmd, char *url, char *data, 
                            httpAckParams *pAckParams, int timeoutSec) {
    int ret = -1;
    int port;
    char buf[64];
    int len = 0;
    char requesturl[256];
    const char *host, *path, *query;
    struct timeval timeOut;
    struct event_base *base = NULL;
    struct evdns_base *dnsbase =NULL;
    struct evhttp_uri *http_uri = NULL;
    struct evhttp_connection *evcon = NULL;
    struct evhttp_request *request = NULL;
    struct evkeyvalq *output_headers = NULL;
    struct evbuffer *output_buffer = NULL;

    http_uri = evhttp_uri_parse(url);
    if (NULL == http_uri) {
        ret = URL_ERR;
        printf("parse url failed, url:%s\n", url);
        goto end;
    }
    host = evhttp_uri_get_host(http_uri);
    if (NULL == host) {
        ret = URL_ERR;
        printf("parse host failed, url:%s\n", url);
        goto end;
    }
    port = evhttp_uri_get_port(http_uri);
    if (port == -1) {
        ret = URL_ERR;
        printf("parse port failed, url:%s\n", url);
        goto end;
    }

    path = evhttp_uri_get_path(http_uri);
    if(path == NULL) {
        ret = URL_ERR;
        printf("get path failed, url:%s\n", url);
        goto end;
    }
    if(strlen(path) == 0) {
        path = "/";
    }
    memset(requesturl, 0, sizeof(requesturl));

    query = evhttp_uri_get_query(http_uri);
    if(NULL == query) {
        snprintf(requesturl, sizeof(requesturl) - 1, "%s", path);
    } else {
        snprintf(requesturl, sizeof(requesturl) - 1, "%s?%s", path, query);
    }
    requesturl[sizeof(requesturl) - 1] = '\0';
    
    base = event_base_new();
    if (NULL == base) {
        printf("create event base failed, url:%s\n", url);
        goto end;
    }
    dnsbase = evdns_base_new(base, EVDNS_BASE_INITIALIZE_NAMESERVERS);
    if (NULL == dnsbase) {
        printf("create dns base failed!\n");
        goto end;
    }
    evcon = evhttp_connection_base_new(base, dnsbase, host, port);
    if (NULL == evcon) {
        printf("base new failed, url:%s\n", url);
        goto end;
    }
    evhttp_connection_set_retries(evcon, 3);
    evhttp_connection_set_timeout(evcon, timeoutSec);
    evhttp_connection_set_closecb(evcon, closeCallback, base);
    pAckParams->arg = base;
    request = evhttp_request_new(readCallback, pAckParams);
    if(request == NULL) {
        printf("request new failed, url:%s\n", url);
        goto end;
    }
    evhttp_request_set_error_cb(request, errCallback);

    output_headers = evhttp_request_get_output_headers(request);
    evhttp_add_header(output_headers, "Host", host);
    evhttp_add_header(output_headers, "Connection", "keep-alive");
    evhttp_add_header(output_headers, "Content-Type", "application/json");
    if(data != NULL) {
        len = strlen(data);
        output_buffer = evhttp_request_get_output_buffer(request);
        evbuffer_add(output_buffer, data, len);
    }

    evutil_snprintf(buf, sizeof(buf)-1, "%lu", (long unsigned int)len);
    evhttp_add_header(output_headers, "Content-Length", buf);

    //app_debug("url:%s, requesturl:%s", url, requesturl);
    ret = evhttp_make_request(evcon, request, httpCmd, requesturl);
    if (ret != 0) {
        printf("make request failed\n");
        goto end;
    }
    timeOut.tv_sec = timeoutSec;
    timeOut.tv_usec = 0;
    event_base_loopexit(base, &timeOut);
    ret = event_base_dispatch(base);
    if(ret != 0) {
        printf("dispatch failed\n");
        goto end;
    }
    ret = 0;

end:
    if(evcon != NULL) {
        evhttp_connection_free(evcon);
    }
    if(dnsbase != NULL) {
        evdns_base_free(dnsbase, 0);
    }
    if(base != NULL) {
        event_base_free(base);
    }
    if(http_uri != NULL) {
        evhttp_uri_free(http_uri);
    }
    //if(request != NULL) {
    //    evhttp_request_free(request);
    //}

    return ret;
}

int httpPost(char *url, char *data, httpAckParams *pAckParams, int timeoutSec) {
    char ack[256] = {0};
    httpAckParams ackParam;

    ackParam.buf = ack;
    ackParam.max = sizeof(ack);
    if(pAckParams == NULL) {
        pAckParams = &ackParam;
    }
    httpClient(EVHTTP_REQ_POST, url, data, pAckParams, timeoutSec);

    return 0;
}

int httpGet(char *url, httpAckParams *pAckParams, int timeoutSec) {
    return httpClient(EVHTTP_REQ_GET, url, NULL, pAckParams, timeoutSec);
}

static int sendHttpReply(struct evhttp_request *req, int code, char *buf) {
    struct evbuffer *evb;

    evb = evbuffer_new();
    if(buf != NULL) {
        evbuffer_add_printf(evb, "%s", buf);
    }
    else {
        evbuffer_add_printf(evb, "{\"code\":0,\"msg\":\"success\",\"data\":{}}");
    }

    evhttp_send_reply(req, code, "OK", evb);
    evbuffer_free(evb);

    return 0;
}

int request_cb(struct evhttp_request *req, void (*httpTask)(struct evhttp_request *, void *), void *arg) {
    char *url;
    int code = HTTP_OK;
    char *pbody = NULL;
    const char *cmdtype = NULL;
    struct evkeyvalq *headers;
    struct evkeyval *header;
    struct evbuffer *buf;
    char cbuf[POST_BUF_MAX];

    switch (evhttp_request_get_command(req)) {
        case EVHTTP_REQ_GET: cmdtype = "GET"; break;
        case EVHTTP_REQ_POST: cmdtype = "POST"; break;
        case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
        case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
        case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
        case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
        case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
        case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
        case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
        default: cmdtype = "unknown"; break;
    }
    url = (char *)evhttp_request_get_uri(req);
    if(cmdtype != NULL) {
        //printf("received a %s request for %s\n", cmdtype, url);
    }

    headers = evhttp_request_get_input_headers(req);
    for (header = headers->tqh_first; header;
        header = header->next.tqe_next) {
        //TODO : check token with Authorization
        //printf("head %s:%s\n", header->key, header->value);
    }

    memset(cbuf, 0, POST_BUF_MAX);
    buf = evhttp_request_get_input_buffer(req);
    while (evbuffer_get_length(buf)) {
        int n;
        n = evbuffer_remove(buf, cbuf, POST_BUF_MAX);
        if (n >= POST_BUF_MAX) {
            app_warring("content length is too large, %d", n);
        }
    }
    cbuf[POST_BUF_MAX - 1] = '\0';

    if(strcmp(cmdtype, "GET")) {
        app_debug("pid:%d, %s:%s", getpid(), url, cbuf);
    }
    if(httpTask != NULL) {
        commonParams params;
        params.arga = cbuf;
        params.argb = &pbody;
        params.argc = arg;
        params.argd = &code;
        httpTask((struct evhttp_request *)(-1), &params);
    }

    sendHttpReply(req, code, pbody);
    if(pbody != NULL) {
        free(pbody);
    }

    return 0;
}

static void request_login(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_logout(struct evhttp_request *req, void *arg) {
}

static void request_system_init(struct evhttp_request *req, void *arg) {
    request_first_stage;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;

    if(pConfigParams->masterEnable != 2) {
        systemInits(buf, pAiotcParams);
    }
    if(!pSlaveParams->systemInit) {
        pSlaveParams->systemInit = 1;
    }
}

static void request_system_status(struct evhttp_request *req, void *arg) {
    request_first_stage;
    commonParams *pParams = (commonParams *)arg;
    char **ppbody = (char **)pParams->argb;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    // TODO : load
    pSlaveParams->load = pObjParams->objQueue.queLen*100 / pConfigParams->slaveObjMax;
    *ppbody = (char *)malloc(256);
     snprintf(*ppbody, 256, "{\"code\":0,\"msg\":\"success\",\"data\":"
             "{\"systemInit\":%d,\"load\":%d}}", pSlaveParams->systemInit, pSlaveParams->load);
}

static void request_obj_add(struct evhttp_request *req, void *arg) {
    request_first_stage;
    commonParams params;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    char **ppbody = (char **)pParams->argb;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    params.arga = &pObjParams->mutex_obj;
    params.argb = &pObjParams->objQueue;
    addObj(buf, pAiotcParams, pConfigParams->slaveObjMax, &params);

    pSlaveParams->load = pObjParams->objQueue.queLen*100 / pConfigParams->slaveObjMax;
    *ppbody = (char *)malloc(256);
    snprintf(*ppbody, 256, "{\"code\":0,\"msg\":\"success\",\"data\":{\"load\":%d}}", pSlaveParams->load);
}

static void request_obj_del(struct evhttp_request *req, void *arg) {
    request_first_stage;
    commonParams params;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;

    params.arga = &pObjParams->mutex_obj;
    params.argb = &pObjParams->objQueue;
    delObj(buf, pAiotcParams, &params);
}

static void request_stream_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    int id = getIntValFromJson(buf, "id", NULL, NULL);

    semWait(&pObjParams->mutex_obj);
    searchFromQueue(&pObjParams->objQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        pTaskParams->livestream = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pObjParams->mutex_obj);
}

static void request_stream_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    int id = getIntValFromJson(buf, "id", NULL, NULL);

    semWait(&pObjParams->mutex_obj);
    searchFromQueue(&pObjParams->objQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        pTaskParams->livestream = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pObjParams->mutex_obj);
}

static void request_preview_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    char *type = NULL;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    int id = getIntValFromJson(buf, "id", NULL, NULL);

    type = getStrValFromJson(buf, "type", NULL, NULL);
    if(id < 0 || type == NULL) {
        goto end;
    }
    semWait(&pObjParams->mutex_obj);
    searchFromQueue(&pObjParams->objQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        strncpy(pTaskParams->preview, type, sizeof(pTaskParams->preview));
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pObjParams->mutex_obj);

end:
    if(type != NULL) {
        free(type);
    }
}

static void request_preview_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    int id = getIntValFromJson(buf, "id", NULL, NULL);

    semWait(&pObjParams->mutex_obj);
    searchFromQueue(&pObjParams->objQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        memset(pTaskParams->preview, 0, sizeof(pTaskParams->preview));
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pObjParams->mutex_obj);
}

static void request_record_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    int id = getIntValFromJson(buf, "id", NULL, NULL);

    semWait(&pObjParams->mutex_obj);
    searchFromQueue(&pObjParams->objQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        pTaskParams->record = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pObjParams->mutex_obj);
}

static void request_record_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    int id = getIntValFromJson(buf, "id", NULL, NULL);

    semWait(&pObjParams->mutex_obj);
    searchFromQueue(&pObjParams->objQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        pTaskParams->record = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pObjParams->mutex_obj);
}

static void request_record_play(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_capture_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    int id = getIntValFromJson(buf, "id", NULL, NULL);

    semWait(&pObjParams->mutex_obj);
    searchFromQueue(&pObjParams->objQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        pTaskParams->capture = 1;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pObjParams->mutex_obj);
}

static void request_capture_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;
    int id = getIntValFromJson(buf, "id", NULL, NULL);

    semWait(&pObjParams->mutex_obj);
    searchFromQueue(&pObjParams->objQueue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        pTaskParams->capture = 0;
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(&pObjParams->mutex_obj);
}

static void request_alg_support(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_task_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
    char *algName = NULL;
    commonParams params;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    algName = getStrValFromJson(buf, "alg", NULL, NULL);
    if(id < 0 || algName == NULL) {
        goto end;
    }

    memset(&params, 0, sizeof(params));
    params.arga = &pObjParams->mutex_obj;
    params.argb = &pObjParams->objQueue;
    addAlg(buf, id, algName, pAiotcParams, &params);

end:
    if(algName != NULL) {
        free(algName);
    }
}

static void request_task_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
    char *algName = NULL;
    commonParams params;
    commonParams *pParams = (commonParams *)arg;
    char *buf = (char *)pParams->arga;
    aiotcParams *pAiotcParams = (aiotcParams *)pParams->argc;
    objParams *pObjParams = (objParams *)pAiotcParams->objArgs;

    int id = getIntValFromJson(buf, "id", NULL, NULL);
    algName = getStrValFromJson(buf, "alg", NULL, NULL);
    if(id < 0 || algName == NULL) {
        goto end;
    }

    memset(&params, 0, sizeof(params));
    params.arga = &pObjParams->mutex_obj;
    params.argb = &pObjParams->objQueue;
    delAlg(buf, id, algName, pAiotcParams, &params);

end:
    if(algName != NULL) {
        free(algName);
    }
}

/*
static void request_gencb(struct evhttp_request *req, void *arg) {
    app_warring("");
    request_cb(req, NULL, NULL);
}
*/

static urlMap rest_url_map[] = {
    {"/system/login",       request_login},
    {"/system/logout",      request_logout},
    {"/system/init",        request_system_init},
    {"/system/slave",       request_system_status},
    {"/obj/add/tcp",        request_obj_add},
    {"/obj/add/ehome",      request_obj_add},
    {"/obj/add/gat1400",    request_obj_add},
    {"/obj/add/rtsp",       request_obj_add},
    {"/obj/add/gb28181",    request_obj_add},
    {"/obj/add/sdk",        request_obj_add},
    {"/obj/add",            request_obj_add},
    {"/obj/del",            request_obj_del},
    {"/obj/stream/start",   request_stream_start},
    {"/obj/stream/stop",    request_stream_stop},
    {"/obj/preview/start",  request_preview_start},
    {"/obj/preview/stop",   request_preview_stop},
    {"/obj/record/start",   request_record_start},
    {"/obj/record/stop",    request_record_stop},
    {"/obj/record/play",    request_record_play},
    {"/obj/capture/start",  request_capture_start},
    {"/obj/capture/stop",   request_capture_stop},
    {"/alg/support",        request_alg_support},
    {"/task/start",         request_task_start},
    {"/task/stop",          request_task_stop},
    {NULL, NULL}
};

int http_task(urlMap *url_map, int port, aiotcParams *pAiotcParams) {
    struct event_base *base;
    struct evhttp *http;
    struct evhttp_bound_socket *handle;

    base = event_base_new();
    if(!base) {
        app_err("Couldn't create an event_base: exiting");
        return -1;
    }

    /* Create a new evhttp object to handle requests. */
    http = evhttp_new(base);
    if (!http) {
        app_err("couldn't create evhttp. Exiting.");
        return -1;
    }

    int i;
    urlMap *pUrlMap;
    for(i = 0; ; i ++) {
        pUrlMap = url_map + i;
        pUrlMap->arg = pAiotcParams;
        if(pUrlMap->cb != NULL) {
            evhttp_set_cb(http, pUrlMap->url, pUrlMap->cb, pUrlMap);
        }
        else {
            break;
        }
    }
    //evhttp_set_gencb(http, request_gencb, NULL);

    /* Now we tell the evhttp what port to listen on */
    handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
    if (!handle) {
        app_err("couldn't bind to port %d", (int)port);
        return -1;
    }

    event_base_dispatch(base);

    return 0;
}

static void *rest_api_thread(void *arg) {
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    ev_uint16_t port = pConfigParams->slaveRestPort;

    //sleep(20);
    app_debug("%s:%d starting ...", pAiotcParams->localIp, port);
    http_task(rest_url_map, port, pAiotcParams);

    app_debug("run over");

    return NULL;
}

static int startRestTask(aiotcParams *pAiotcParams) {
    pthread_t pid;

    if(pthread_create(&pid, NULL, rest_api_thread, pAiotcParams) != 0) {
        app_err("pthread_create rest api thread err");
    }
    else {
        pthread_detach(pid);
    }

    return 0;
}


int restProcess(void *arg) {
    pidOps *pOps = (pidOps *)arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;
    slaveParams *pSlaveParams = (slaveParams *)pAiotcParams->slaveArgs;

    pOps = getRealOps(pOps);
    if(pOps == NULL) {
        return -1;
    }
    pOps->running = 1;

    pSlaveParams->systemInit = 0;
    startRestTask(pAiotcParams);

    while(pOps->running) {
        sleep(2);
    }
    pOps->running = 0;

    app_debug("pid:%d, run over", pOps->pid);

    return 0;
}

