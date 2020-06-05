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

#ifndef __AIOTC_TASK_H__
#define __AIOTC_TASK_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "share.h"

#define TASK_BEAT_SLEEP     1
#define TASK_BEAT_TIMEOUT   30

typedef struct {
    const char    *type;
    CommonObjFunc init;
    CommonObjFunc uninit;
    CommonObjFunc start;
    CommonObjFunc stop;
    CommonObjFunc ctrl;
    void          *arg; // expand anything, global hls params, http-flv patams etc ...
} taskOps;

typedef struct {
    int livestream;
    int liveBeat;       // for proc running
    int liveTaskBeat;   // for obj task running
    int liveRestart;
    void *liveArgs;

    int capture;
    int captureBeat;
    int captureTaskBeat;
    int captureRestart;
    void *captureArgs;

    int record;
    int recordBeat;
    int recordTaskBeat;
    int recordRestart;
    void *recordArgs;

    char preview[32];
    int previewBeat;
    int previewTaskBeat;
    int previewRestart;
    void *previewArgs;

    sem_t mutex_alg;
    queue_common algQueue; // algParams
} taskParams;

#endif
