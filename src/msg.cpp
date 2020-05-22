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

#include "msg.h"
#include "share.h"

int msgSend(char *buf, int sendkey, int recvKey, int recvTimeout) {
    int ret, msgId;
    int attempNum = 3;
    int *ack = NULL;
    msgBufParams msgBuf;

    msgId = msgget(sendkey, 0666|IPC_CREAT);
    if(msgId == -1) {
        app_err("create msg failed, key:%x, %s", sendkey, strerror(errno));
        return -1;
    }

    msgBuf.ptr = 0;
    if(recvKey != 0) {
        ack = (int *)malloc(sizeof(int));
        if(ack == NULL) {
            app_err("malloc failed");
            return -1;
        }
        *ack = 0;
        msgBuf.ptr = (long long int)ack;
    }
    msgBuf.type = 1L;
    msgBuf.key = recvKey;
    strncpy(msgBuf.buf, buf, MSG_BUF_LEN);
    msgBuf.buf[MSG_BUF_LEN - 1] = '\0';
    do {
        ret = msgsnd(msgId, &msgBuf, MSG_BUF_LEN, IPC_NOWAIT);
        if(ret == 0) {
            break;
        }
        else if(errno == EAGAIN) {
            printf("msgsnd failed, key:%x, %s, attemp again\n", sendkey, strerror(errno));
            usleep(100000);
        }
        else {
            app_err("msgsnd failed, key:%x, %s", sendkey, strerror(errno));
            break;
        }
    } while(attempNum --);

    if(recvKey != 0) {
        int wait = recvTimeout*10;
        do {
            if(*ack) {
                break;
            }
            usleep(100000);
        } while(wait --);
    }

    if(ack != NULL) {
        free(ack);
    }

    return 0;
}

static int msgSendAck(int key, long long int ptr) {
    int ret, msgId;
    msgBufParams msgBuf;

    msgId = msgget(key, 0666|IPC_CREAT);
    if(msgId == -1) {
        app_err("create msg failed, key:%x, %s", key, strerror(errno));
        return -1;
    }

    memset(&msgBuf, 0, sizeof(msgBuf));
    msgBuf.type = 1L;
    msgBuf.ptr = ptr;
    ret = msgsnd(msgId, &msgBuf, MSG_BUF_LEN, IPC_NOWAIT);
    if(ret != 0) {
        app_err("msgsnd failed, key:%x, %s", key, strerror(errno));
    }

    return 0;
}

static int msgTask(char *buf, void *arg) {
    int i;
    char *cmd = NULL;
    cmdTaskParams *pCmdParams;
    msgParams *pMsgParams = (msgParams *)arg;

    cmd = getStrValFromJson(buf, (char *)"cmd", NULL, NULL);
    if(cmd == NULL) {
        app_err("get cmd failed");
        return -1;
    }

    for(i = 0; ; i ++) {
        pCmdParams = pMsgParams->pUserCmdParams + i;
        if(!strncmp(pCmdParams->cmd, "null", 64)) {
            app_warring("unsupport cmd : %s", cmd);
            break;
        }
        if(!strncmp(pCmdParams->cmd, cmd, 64)) {
            pCmdParams->cmdTaskFunc(buf, pMsgParams->arg);
            break;
        }
    }

    free(cmd);

    return 0;
}

static void *msgThread(void *arg) {
    int ret, ret2, errcnt, msgId;
    msgParams *pMsgParams = (msgParams *)arg;

    msgBufParams *pMsgBuf = (msgBufParams *)malloc(sizeof(msgBufParams));
    if(pMsgBuf == NULL) {
        app_err("malloc msgBufParams failed");
        return NULL;
    }

    msgId = msgget(pMsgParams->key, 0666|IPC_CREAT);
    if(msgId == -1) {
        app_err("create msg failed, key:%x, %s", pMsgParams->key, strerror(errno));
        return NULL;
    }

    errcnt = 0;
    while(pMsgParams->running) {
        memset(pMsgBuf, 0, sizeof(msgBufParams));
        ret = msgrcv(msgId, pMsgBuf, MSG_BUF_LEN, 0, 0);
        if(ret < 0) {
            if(errno == EINTR) {
                printf("msgrcv EINTR, key:%x\n", pMsgParams->key);
                continue;
            }
            if(errno == EIDRM) {
                app_warring("msgrcv EIDRM, pid:%d, key:%x, %d:%s, restart ...", getpid(), pMsgParams->key, errno, strerror(errno));
                exit(-1);
            }
            if(errcnt ++ == 0) {
                app_warring("msgrcv failed, pid:%d, key:%x, %d:%s", getpid(), pMsgParams->key, errno, strerror(errno));
            }
            else {
                printf("msgrcv failed, key:%x, %s\n", pMsgParams->key, strerror(errno));
            }
        }
        else {
            errcnt = 0;
            if(pMsgBuf->ptr != 0 && pMsgBuf->key == 0) {
                int *ack = (int *)pMsgBuf->ptr;
                *ack = 1;
                continue;
            }
            ret2 = msgTask(pMsgBuf->buf, arg);
            if(pMsgBuf->key != 0) {
                msgSendAck(pMsgBuf->key, pMsgBuf->ptr);
            }
            if(ret2 == -3) {
                break;
            }
        }
    }

    if(msgctl(msgId, IPC_RMID, NULL) < 0) {
        app_err("msgctl IPC_RMID failed, key:%x, %s", pMsgParams->key, strerror(errno));
    }

    free(pMsgBuf);

    app_debug("msg key %x, run over", pMsgParams->key);

    return NULL;
}

int createMsgThread(msgParams *pMsgParams) {
    pthread_t pid;
    if(pthread_create(&pid, NULL, msgThread, pMsgParams) != 0) {
        app_err("pthread_create msgThread failed");
    }
    else {
        pthread_detach(pid);
    }

    return 0;
}

