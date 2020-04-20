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

typedef struct {
    int shmId;
    void *shmAddr;
    long long int shmTotalSize;
    long long int shmUsedOffset;

    ncx_slab_pool_t *headsp;

    int running;
    void *arg; // aiotcParams
} shmParams;

int shmInit(configParams *pConfigParams, shmParams *pShmParams);
int shmDestroy(void *ptr);
void *shmMalloc(ncx_slab_pool_t *pool, size_t size);
void shmFree(ncx_slab_pool_t *pool, void *ptr);

#endif
