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

#include "mq.h"
#include "share.h"
#include "log.h"

int mqOpenConnect(mqOutParams *pmqOutParams, amqp_connection_state_t *ppConn, int timeoutsec) {
    int ret = 0;
    int status;
    amqp_socket_t *socket = NULL;
    int  port = pmqOutParams->port;
    char const * hostname = pmqOutParams->host;
    int channelOpened = 0, connectionOpened = 0;

    amqp_connection_state_t conn = amqp_new_connection();
    if(conn == NULL) {
        app_err(" amqp_new_connection failed");
        return -1;
    }
    
    socket = amqp_tcp_socket_new(conn);
    if(NULL == socket) {
        app_err(" amqp_tcp_socket_new failed");
        return -1;
    }
    
    if(timeoutsec>0) {
        struct timeval timeout = {timeoutsec, 0};
        status = amqp_socket_open_noblock(socket, hostname, port, &timeout);
    }
    else {
        status = amqp_socket_open(socket, hostname, port);
    }
    if(status == AMQP_STATUS_OK) {
        //printf("opening TCP socket");
        connectionOpened = 1;
    }
    
    amqp_rpc_reply_t rpc_reply = amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
                                 pmqOutParams->userName, pmqOutParams->passWord);
    if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        printf("mq login %s:%d failed, ret:%d\n", hostname, port, rpc_reply.reply_type);
        goto new_err;
    }
    
    if(amqp_channel_open(conn, 1) != NULL) {
        channelOpened = 1;
    }

    rpc_reply = amqp_get_rpc_reply(conn);
    if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        app_err("get rpc reply failed, ret:%d", rpc_reply.reply_type);
        goto new_err;
    }

    *ppConn = conn;
    return 0;
new_err:
    if(channelOpened) {
        rpc_reply = amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
        if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
            app_err("close channel failed, ret:%d", rpc_reply.reply_type);
        }
    }
    if(connectionOpened) {
        rpc_reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
        if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
            app_err("close connction failed, ret:%d", rpc_reply.reply_type);
        }
    }
    
    ret = amqp_destroy_connection(conn);
    if(ret != AMQP_STATUS_OK) {
        app_err("destroty connection failed, ret:%d", ret);
        return -1;
    }
    *ppConn = NULL;

    return -1;
}

int mqCloseConnect(amqp_connection_state_t conn) {
    int ret = 0;
    amqp_rpc_reply_t rpc_reply = amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        printf("mq, close channel failed, ret:%d\n", rpc_reply.reply_type);
    }


    rpc_reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    if(rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        app_err("close connction failed, ret:%d", rpc_reply.reply_type);
    }

    ret = amqp_destroy_connection(conn);
    if(ret != AMQP_STATUS_OK) {
        app_err("destroty connection failed, ret:%d", ret);
        return -1;
    }

    return -1;
}

static int mqSendMsg(amqp_connection_state_t conn, mqOutParams *pmqOutParams,char *msgBody) {
    char const *routingkey = pmqOutParams->routingKey;
    char const *messagebody = msgBody;
    char const *exchange = pmqOutParams->exchange;

    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */
    int ret = amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange),
                                    amqp_cstring_bytes(routingkey), 0, 0,
                                    &props, amqp_cstring_bytes(messagebody));
    if(ret != AMQP_STATUS_OK) {
        app_err("basic publish failed, ret:%d", ret);
        return -1;
    }

    return 0;
}

int sendMsg2Mq(mqOutParams *pmqOutParams, amqp_connection_state_t *pp_mq_conn, char *json) {
    if(mqSendMsg(*pp_mq_conn, pmqOutParams, json) != 0) {
        mqCloseConnect(*pp_mq_conn);
        *pp_mq_conn = NULL;
    }

    return 0;
}

int initMqParams(mqOutParams *pMqParams, char *buf) {
    int port;
    char *type = NULL, *host = NULL, *userName = NULL; 
    char *passWord = NULL, *exchange = NULL, *routingkey = NULL;

    type = getStrValFromJson(buf, "type", NULL, NULL);
    host = getStrValFromJson(buf, "host", NULL, NULL);
    port = getIntValFromJson(buf, "port", NULL, NULL);
    userName = getStrValFromJson(buf, "userName", NULL, NULL);
    passWord = getStrValFromJson(buf, "passWord", NULL, NULL);
    exchange = getStrValFromJson(buf, "exchange", NULL, NULL);
    routingkey = getStrValFromJson(buf, "routingKey", NULL, NULL);

    if(!(type != NULL && strcmp(type, "mq") == 0)) {
        goto end;
    }
    if(host != NULL && userName != NULL && passWord != NULL && exchange != NULL) {
        pMqParams->port = port;
        strncpy(pMqParams->host, host, sizeof(pMqParams->host));
        strncpy(pMqParams->userName, userName, sizeof(pMqParams->userName));
        strncpy(pMqParams->passWord, passWord, sizeof(pMqParams->passWord));
        strncpy(pMqParams->exchange, exchange, sizeof(pMqParams->exchange));
    }
    else {
        app_err("some mq params is NULL");
    }

    if(routingkey != NULL) {
        strncpy(pMqParams->routingKey, routingkey, sizeof(pMqParams->routingKey));
    }

end:
    if(type != NULL) {
        free(type);
    }
    if(host != NULL) {
        free(host);
    }
    if(userName != NULL) {
        free(userName);
    }
    if(passWord != NULL) {
        free(passWord);
    }
    if(exchange != NULL) {
        free(exchange);
    }
    if(routingkey != NULL) {
        free(routingkey);
    }

    return 0;
}

