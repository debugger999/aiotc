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

#ifndef __AIOTC_MSG_H__
#define __AIOTC_MSG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MSG_BUF_LEN             256

typedef struct {
    char cmd[64];
    int (*cmdTaskFunc)(char *buf, void *arg);
} cmdTaskParams;

typedef struct {
    key_t key;
    cmdTaskParams *pUserCmdParams;
    int (*msgTaskFunc)(char *buf, void *arg);
    int running;
    void *arg;
} msgParams;

typedef struct {
    long type;
    int key;
    long long int ptr; // for msg ack
    char buf[MSG_BUF_LEN];
} msgBufParams;
#define MSG_LEN (sizeof(msgBufParams) - sizeof(long))

int msgSend(char *buf, int sendkey, int recvKey, int recvTimeout);
int createMsgThread(msgParams *pMsgParams);

#endif
