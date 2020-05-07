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

    app_debug("%s:%s", url, cbuf);
    if(httpTask != NULL) {
        CommonParams params;
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
}

static void request_slave_add(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_slave_del(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_obj_add(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_obj_del(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_stream_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_stream_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_preview_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_preview_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_record_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_record_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_record_play(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_capture_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_capture_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_alg_support(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_task_start(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

static void request_task_stop(struct evhttp_request *req, void *arg) {
    request_first_stage;
}

/*
static void request_gencb(struct evhttp_request *req, void *arg) {
    app_warring("##test");
    request_cb(req, NULL, NULL);
}
*/

static urlMap rest_url_map[] = {
    {"/system/login",       request_login},
    {"/system/logout",      request_logout},
    {"/system/init",        request_system_init},
    {"/system/slave/add",   request_slave_add},
    {"/system/slave/del",   request_slave_del},
    {"/obj/add/tcp",        request_obj_add},
    {"/obj/add/ehome",      request_obj_add},
    {"/obj/add/gat1400",    request_obj_add},
    {"/obj/add/rtsp",       request_obj_add},
    {"/obj/add/gb28181",    request_obj_add},
    {"/obj/add/sdk",        request_obj_add},
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
    aiotcParams *pAiotcParams = (aiotcParams *)arg;

    startRestTask(pAiotcParams);
    while(pAiotcParams->running) {
        sleep(2);
    }

    app_debug("run over");

    return 0;
}

