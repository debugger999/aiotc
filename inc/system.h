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

#ifndef __AIOTC_SYSTEM_H__
#define __AIOTC_SYSTEM_H__

#include "platform.h"
#include "share.h"

typedef struct {
    char *sysOrgData;

    sem_t mutex_slave;
    queue_common slaveQueue;

    sem_t mutex_obj;
    queue_common objQueue;

    void *arg; // aiotcParams
} systemParams;

#endif
