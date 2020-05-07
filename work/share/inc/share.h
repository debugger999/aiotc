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

#ifndef __AIOTC_SHARE_H__
#define __AIOTC_SHARE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define COMMON_QUEUE_MAX        10000
#define MSG_BUF_LEN             256

typedef struct nodecommon {
    char name[256];
    int val;
    void *arg;
    struct nodecommon *next;
} node_common;

typedef struct {
    node_common *head;
    node_common *tail;
    int useMax;
    int queLen;
} queue_common;

typedef struct {
    key_t key;
    int running;
    int (*msgTaskFunc)(char *buf, void *arg);
    void *arg;
} msgParams;

typedef struct {
    long type;
    long long int ptr; // for msg ack
    char buf[MSG_BUF_LEN];
} msgBufParams;
#define MSG_LEN (sizeof(msgBufParams) - sizeof(long))

int getLocalIp(char hostIp[128]);
int connectServer(char *ip, int port);
int blockSend(unsigned int connfd, char *src, int size);
int blockRecv(unsigned int connfd, char *dst, int size, int timeOutSec);
int putToQueue(queue_common *queue, node_common *new_node, int max);
int putToQueueDelFirst(queue_common *queue, node_common *new_node, int max,int (*callBack)(void *arg));
int getFromQueue(queue_common *queue, node_common **new_p);
int delFromQueue(queue_common *queue, void *arg, node_common **new_p, int (*condition)(node_common *p, void *arg));
int searchFromQueue(queue_common *queue, void *arg, node_common **new_p, int (*condition)(node_common *p, void *arg));
int traverseQueue(queue_common *queue, void *arg, int (*callBack)(node_common *p, void *arg));
int freeQueue(queue_common *queue, int (*callBack)(void *arg));
int putToShmQueue(void *sp, queue_common *queue, node_common *new_node, int max);
int freeShmQueue(void *sp, queue_common *queue, int (*callBack)(void *arg));
int conditionTrue(node_common *p, void *arg);

int getIntValFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3);
char *getStrValFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3);
int getIntValFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3);
char *getStrValFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3);
double getDoubleValFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3);
char *getArrayBufFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3, int &size);
long long int getSysFreeMem(void);

inline void semWait(sem_t *mutex) {
    int tryCnt = 0;
    while(sem_wait(mutex) == -1 && tryCnt < 10000) {
        if(errno != EINTR) {
            app_warring("sem wait failed, %d:%s", errno, strerror(errno));
            break;
        }
        if(tryCnt == 0) {
            app_warring("sem wait failed, %d:%s", errno, strerror(errno));
        }
        tryCnt ++;
    }
}

inline void semPost(sem_t *mutex) {
    if(sem_post(mutex) == -1) {
        app_warring("sem post failed, %d:%s", errno, strerror(errno));
    }
}

#endif
