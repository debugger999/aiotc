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

#ifndef __AIOTC_SHM_H__
#define __AIOTC_SHM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ncx_slab.h"
#include "db.h"
#include "share.h"

typedef struct {
    int             type;
    char            *ptr;
    int             size;
    long long int   frameId;
    int             user;
    void            *arg;
} shmFrame;

typedef struct {
    int                 id;
    char                type[32];
    void                *shmAddr;
    ncx_slab_pool_t     *sp;

    sem_t               *mutex_shm;
    queue_common        *queue;
    int                 used;

    void                *arg; // objParam
} shmParam;

typedef struct {
    int             shmId;
    void            *shmAddr;
    long long int   shmTotalSize;

    ncx_slab_pool_t *headsp;

    int             num;
    shmParam        *pShmArray;

    void            *arg; // aiotcParams
} shmParams;

int shmInit(configParams *pConfigParams, shmParams *pShmParams);
int initShmArray(shmParams *pShmParams);
int shmDestroy(void *ptr);

inline void *shmMalloc(ncx_slab_pool_t *pool, size_t size) {
    return ncx_slab_alloc(pool, size);
}

inline void shmFree(ncx_slab_pool_t *pool, void *ptr) {
    ncx_slab_free(pool, ptr); 
}

#endif
