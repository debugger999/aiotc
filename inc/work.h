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

#ifndef __AIOTC_WORK_H__
#define __AIOTC_WORK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mq.h"

typedef struct {
    float x;
    float y;
    float w;
    float h;
} Rectt;

typedef struct {
    char            msgType[32];
    int             id;
    long long int   timestamp;

    char            sceneUrl[512];

    char            faceUrl[512];
    Rectt           faceRect;
    float           faceQuality;
    char            personBodyUrl[512];
    Rectt           personBodyRect;

    char            plateNo[64];
    char            plateColor[64];
    char            plateUrl[512];
    Rectt           plateRect;
    char            vehBodyUrl[512];
    Rectt           vehBodyRect;

    void            *arg;
} outJsonParams;

typedef struct {
    mqOutParams     mqOutParam;
    sem_t           mutex_out;
    queue_common    pOutQueue;
} outParams;

typedef struct {
    char date[32];
    outParams outParam;
    void *arg; // aiotcParams
} workParams;

int workProcInit(void *arg);
int workProcess(void *arg);
int copyToMqQueue(char *json, workParams *pWorkParams);
char *makeJson(outJsonParams *pJsonParams);

#endif
