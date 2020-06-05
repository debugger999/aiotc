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

#ifndef __AIOTC_CAMERA_H__
#define __AIOTC_CAMERA_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shm.h"

typedef struct {
    shmParam        *videoShm;
    shmParam        *captureShm;
    long long int   videoFrameId;
    long long int   captureFrameId;
    void            *arg; // objParam
} cameraParams;

#endif
