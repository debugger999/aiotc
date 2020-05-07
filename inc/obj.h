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

#ifndef __AIOTC_OBJ_H__
#define __AIOTC_OBJ_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "share.h"

typedef struct {
    int id;
    char name[32];
    char type[16];
    char subtype[16];
    void *task; // taskParams
    void *slave;
    char *originaldata;
    void *arg; // aiotcParams
} objParam;

typedef struct {
    sem_t mutex_obj;
    queue_common objQueue;
    void *arg; // aiotcParams
} objParams;

#endif
