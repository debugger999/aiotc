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

#ifndef __AIOTC_MQ_H__
#define __AIOTC_MQ_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amqp.h"
#include "amqp_tcp_socket.h"

typedef struct {
    char host[128];
    int  port;
    char userName[64];
    char passWord[64];
    char exchange[256];
    char objExchange[256];
    char routingKey[256];
} mqOutParams;

int initMqParams(mqOutParams *pMqParams, char *buf);
int mqOpenConnect(mqOutParams *pmqOutParams, amqp_connection_state_t *ppConn, int timeoutsec);
int mqCloseConnect(amqp_connection_state_t conn);
int sendMsg2Mq(mqOutParams *pmqOutParams, amqp_connection_state_t *pp_mq_conn, char *json);

#endif
