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

#ifndef __AIOTC_REST_H__
#define __AIOTC_REST_H__

#include "platform.h"
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <event2/dns.h>

#define POST_BUF_MAX        1024

#define request_first_stage                         \
do {                                                \
    if(req == NULL) {                               \
        app_warning("req is null");                 \
        return ;                                    \
    }                                               \
    if(req != (struct evhttp_request *)(-1)) {      \
        urlMap *pUrlMap = (urlMap *)arg;            \
        request_cb(req, pUrlMap->cb, pUrlMap->arg); \
        return ;                                    \
    }                                               \
} while(0);

typedef struct {
    void *tokenArgs;
    void *arg; // aiotcParams
} restParams;

typedef struct {
    const char *url;
    void (*cb)(struct evhttp_request *, void *);
    void *arg;
} urlMap;

typedef struct {
    char *buf;
    int max;
    void *arg;
} httpAckParams;

int restProcess(void *args);
int http_task(urlMap *url_map, int port, aiotcParams *pAiotcParams);
int request_cb(struct evhttp_request *req, void (*httpTask)(struct evhttp_request *, void *), void *arg);
int httpPost(char *url, char *data, httpAckParams *pAckParams, int timeoutSec);
int httpGet(char *url, httpAckParams *pAckParams, int timeoutSec);

#endif
