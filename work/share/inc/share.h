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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <errno.h>
#include "log.h"

#define COMMON_QUEUE_MAX        10000

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

typedef int (*CommonCB)(void *arg);
typedef int (*NodeCallback)(node_common *p, void *arg);
typedef int (*CommonObjFunc)(const void *buf, void *arg);
typedef int (*COMMONCALLBACK)(void *argA, void *argB, void *argC);

int getLocalIp(char hostIp[128]);
int connectServer(char *ip, int port);
int blockSend(unsigned int connfd, char *src, int size, int max);
int blockRecv(unsigned int connfd, char *dst, int size, int max, void *arg = NULL, CommonCB callback = NULL);
int putToQueue(queue_common *queue, node_common *new_node, int max);
int putToQueueDelFirst(queue_common *queue, node_common *new_node, int max, int (*callBack)(void *arg));
int getFromQueue(queue_common *queue, node_common **new_p);
int delFromQueue(queue_common *queue, void *arg, node_common **new_p, int (*condition)(node_common *p, void *arg));
int delFromQueueByUser(queue_common *queue, void *arg, node_common **new_p, int (*condition)(node_common *p, void *arg));
int searchFromQueue(queue_common *queue, void *arg, node_common **new_p, int (*condition)(node_common *p, void *arg));
int traverseQueue(queue_common *queue, void *arg, int (*callBack)(node_common *p, void *arg));
int freeQueue(queue_common *queue, int (*callBack)(void *arg));
int putToShmQueue(void *sp, queue_common *queue, node_common *new_node, int max);
int freeShmQueue(void *sp, queue_common *queue, int (*callBack)(void *arg));
int conditionTrue(node_common *p, void *arg);

int getIntValFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3);
char *getStrValFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3);
char *getObjBufFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3);
int getIntValFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3);
char *getStrValFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3);
double getDoubleValFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3);
char *getBufFromArray(char *buf, char *nameSub1, char *nameSub2, char *nameSub3, int index);
char *getArrayBufFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3, int &size);
char *getArrayBufFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3, int &size);
char *delObjJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3);

long long int getSysFreeMem(void);
int writeFile(char *fileName, void *buf, int size);
int file_size(const char* filename);

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

// __sync_fetch_and_add(&count, 1);
inline int fetch_and_add(int* variable, int value) {
    __asm__ volatile("lock; xaddl %0, %1"
            : "+r" (value), "+m" (*variable) // input+output
            : // No input-only
            : "memory"
        );
    return value;
}

#endif
