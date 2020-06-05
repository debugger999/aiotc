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

#ifndef __AIOTC_FFMPEG_H__
#define __AIOTC_FFMPEG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
}

#define SEGMENT_TIME_SEC    "2"
#define HLS_LIST_SIZE       "2"
#define HLS_LIST_NUM        2

typedef struct {
    AVFormatContext     *ifmt_ctx;
    AVFormatContext     *ofmt_ctx_v;
    AVOutputFormat      *ofmt_v;
    AVIOContext         *avio;
} previewFFmpeg;

int ffmpegInit(void);
int previewInit(void *obj);
int previewUnInit(void *obj);
int previewLoop(void *obj);

#endif
