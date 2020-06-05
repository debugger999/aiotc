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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "misc.h"
#include "db.h"
#include "obj.h"
#include "camera.h"
#include "task.h"
#include "shm.h"
#include "preview.h"
#include "ffmpeg.h"

int ffmpegInit(void) {
    av_register_all();
    av_log_set_level(AV_LOG_WARNING);

    return 0;
}

static int delLastTsFile(char *m3u8dir)  {
    char filename[512];
    DIR *dirp;
    struct dirent *dp;

    dirp = opendir(m3u8dir);
    if(dirp != NULL) {
        while ((dp = readdir(dirp)) != NULL) {
            if(!strcmp(dp->d_name,".") || !strcmp(dp->d_name,".."))
                continue;
            snprintf(filename, sizeof(filename), "%s/%s", m3u8dir, dp->d_name);
            if(!access(filename, F_OK)) {
                remove(filename);
            }
        }
        closedir(dirp);
    }

    return 0;
}

static int freePrewFFmpeg(previewFFmpeg *pPrewFFmpeg, int videoindex) {
    if(videoindex != -1) {
        av_write_trailer(pPrewFFmpeg->ofmt_ctx_v);
    }
    if(pPrewFFmpeg->ifmt_ctx != NULL) {
        avformat_close_input(&(pPrewFFmpeg->ifmt_ctx));
        pPrewFFmpeg->ifmt_ctx = NULL;
    }
    if(pPrewFFmpeg->ofmt_ctx_v && !(pPrewFFmpeg->ofmt_v->flags & AVFMT_NOFILE)) {
        avio_close(pPrewFFmpeg->ofmt_ctx_v->pb);
    }
    if(pPrewFFmpeg->ofmt_ctx_v != NULL) {
        avformat_free_context(pPrewFFmpeg->ofmt_ctx_v);
        pPrewFFmpeg->ofmt_ctx_v = NULL;
    }
    if(pPrewFFmpeg->avio != NULL) {
        av_freep(&(pPrewFFmpeg->avio->buffer));
        av_freep(&(pPrewFFmpeg->avio));
        pPrewFFmpeg->avio = NULL;
    }

    return 0;
}

static int read_video_packet(void *opaque, uint8_t *buf, int size) {
    int len = 0;
    node_common *p;
    int valid, useMax;
    shmFrame *pShmFrame;
    long long int frameId = 0;
    objParam *pObjParam = (objParam *)opaque;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    cameraParams *pCameraParams = (cameraParams *)pObjParam->objArg;
    shmParam *pVideoShm = (shmParam *)pCameraParams->videoShm;
    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;

    do {
        valid = copyFromShmWithFrameId(pVideoShm, frameId, &p, &useMax);
        if(!valid) {
            usleep(30000);
            continue;
        }

        pShmFrame = (shmFrame *)p->name;
        if(pPreviewParams->findIDR) {
            if(len + pShmFrame->size < size) {
                memcpy(buf + len, pShmFrame->ptr, pShmFrame->size);
                len += pShmFrame->size;
            }
            else {
                printf("preview, read video size exception, id:%d, %d:%d\n", pObjParam->id, len, pShmFrame->size);
                if(__sync_add_and_fetch(&pShmFrame->used, 1) >= useMax) {
                    shmFree(pVideoShm->sp, pShmFrame->ptr); 
                    shmFree(pVideoShm->sp, p); 
                }
                break;
            }
        }
        else {
            if((pShmFrame->ptr[4] & 0x1f) != 1) {
                if(len + pShmFrame->size < size) {
                    memcpy(buf + len, pShmFrame->ptr, pShmFrame->size);
                    len += pShmFrame->size;
                }
                pPreviewParams->findIDR = 1;
                app_debug("id:%d, find IDR ok", pObjParam->id);
            }
        }

        if(__sync_add_and_fetch(&pShmFrame->used, 1) >= useMax) {
            shmFree(pVideoShm->sp, pShmFrame->ptr); 
            shmFree(pVideoShm->sp, p); 
        }

        usleep(20000);
    } while(len == 0 && pPreviewParams->running);

    return len;
}

int previewInit(void *obj) {
    char errstr[256];
    char pathBuf[256];
    int i, ret, init = -1;
    unsigned char *aviobuffer;
    const char *out_filename_v;
    previewFFmpeg *pPrewFFmpeg;
    objParam *pObjParam = (objParam *)obj;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    int aviobufSize = pConfigParams->videoFrameSizeMax*2;

    if(pPreviewParams->prewFFmpeg == NULL) {
        pPreviewParams->prewFFmpeg = malloc(sizeof(previewFFmpeg));
        if(pPreviewParams->prewFFmpeg == NULL) {
            app_err("malloc failed");
            return -1;
        }
    }
    memset(pPreviewParams->prewFFmpeg, 0, sizeof(previewFFmpeg));
    pPrewFFmpeg = (previewFFmpeg *)pPreviewParams->prewFFmpeg;

    pPreviewParams->videoindex = -1;
    if(!strncmp(pTaskParams->preview, "hls", sizeof(pTaskParams->preview))) {
        snprintf(pathBuf, sizeof(pathBuf), "%s/webpages/m3u8/stream%d", pConfigParams->workdir, pObjParam->id);
        if(dirCheck(pathBuf) < 0) {
            app_err("directory check %s err", pathBuf);
            goto end;
        }
        delLastTsFile(pathBuf);
        strncat(pathBuf, "/play.m3u8", sizeof(pathBuf));
    }
    else if(!strncmp(pTaskParams->preview, "http-flv", sizeof(pTaskParams->preview))) {
        snprintf(pathBuf, sizeof(pathBuf), "rtmp://127.0.0.1:1935/myapp/stream%d", pObjParam->id);
    }
    else {
        app_warring("unsupport preview type:%s, %d", pTaskParams->preview, pObjParam->id);
        goto end;
    }
    out_filename_v = pathBuf;

    printf("preview, %d, waiting stream ...\n", pObjParam->id);
    while((pPreviewParams->streamOk == 0)) {
        if(!pPreviewParams->running){
            printf("preview, %d, waiting stream break ...\n", pObjParam->id);
            goto end;
        }
        usleep(100000);
    }
    printf("preview, %d, waiting stream ok\n", pObjParam->id);

    aviobuffer = (unsigned char *)av_mallocz(aviobufSize);
    if(aviobuffer == NULL) {
        app_err("av_mallocz %d failed", aviobufSize);
        goto end;
    }
    pPrewFFmpeg->ifmt_ctx = avformat_alloc_context();
    if(pPrewFFmpeg->ifmt_ctx == NULL) {
        app_err( "Could not alloc input context, %d", pObjParam->id);
        goto end;
    }
    pPrewFFmpeg->avio = avio_alloc_context(aviobuffer, aviobufSize, 0, pObjParam, read_video_packet, NULL, NULL);
    if(pPrewFFmpeg->avio == NULL) {
        app_err("avio_alloc_context %d failed, %d", aviobufSize, pObjParam->id);
        goto end;
    }
    pPrewFFmpeg->ifmt_ctx->pb = pPrewFFmpeg->avio;

    if ((ret = avformat_open_input(&(pPrewFFmpeg->ifmt_ctx), NULL, NULL, NULL)) < 0) {
        av_strerror(ret, errstr, sizeof(errstr));
        app_err( "Could not open input file, %d, %s", pObjParam->id, errstr);
        goto end;
    }
    if ((ret = avformat_find_stream_info(pPrewFFmpeg->ifmt_ctx, 0)) < 0) {
        app_err( "Failed to retrieve input stream information, %d", pObjParam->id);
        goto end;
    }

    if(!strncmp(pTaskParams->preview, "hls", sizeof(pTaskParams->preview))) {
        avformat_alloc_output_context2(&(pPrewFFmpeg->ofmt_ctx_v), NULL, pTaskParams->preview, out_filename_v);
        if (!(pPrewFFmpeg->ofmt_ctx_v)) {
            app_err( "Could not create output context, %d", pObjParam->id);
            goto end;
        }
        av_opt_set(pPrewFFmpeg->ofmt_ctx_v->priv_data, "segment_time", SEGMENT_TIME_SEC, 0);
        av_opt_set(pPrewFFmpeg->ofmt_ctx_v->priv_data, "hls_list_size", HLS_LIST_SIZE, 0);
    }
    else if(!strncmp(pTaskParams->preview, "http-flv", sizeof(pTaskParams->preview))) {
        avformat_alloc_output_context2(&(pPrewFFmpeg->ofmt_ctx_v), NULL, "flv", out_filename_v);
        if (!(pPrewFFmpeg->ofmt_ctx_v)) {
            app_err( "Could not create output context, %d", pObjParam->id);
            goto end;
        }
        av_opt_set(pPrewFFmpeg->ofmt_ctx_v->priv_data, "live", NULL, 0);
    }
    pPrewFFmpeg->ofmt_v = pPrewFFmpeg->ofmt_ctx_v->oformat;

    for (i = 0; i < (int)pPrewFFmpeg->ifmt_ctx->nb_streams; i++) {
        //Create output AVStream according to input AVStream
        AVFormatContext *ofmt_ctx;
        AVStream *in_stream = pPrewFFmpeg->ifmt_ctx->streams[i];
        AVStream *out_stream = NULL;

        if(pPrewFFmpeg->ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            pPreviewParams->videoindex = i;
            out_stream = avformat_new_stream(pPrewFFmpeg->ofmt_ctx_v, in_stream->codec->codec);
            ofmt_ctx = pPrewFFmpeg->ofmt_ctx_v;
        }
        else{
            app_warring("id:%d, streams:%d, unsupport type : %d, break", pObjParam->id, 
                    pPrewFFmpeg->ifmt_ctx->nb_streams, pPrewFFmpeg->ifmt_ctx->streams[i]->codec->codec_type);
            goto end;
        }

        if (!out_stream) {
            app_err("Failed allocating output stream, %d", pObjParam->id);
            goto end;
        }
        //Copy the settings of AVCodecContext
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            app_err( "Failed to copy context from input to output stream codec context, %d", pObjParam->id);
            goto end;
        }
        out_stream->codec->codec_tag = 0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        }
    }

    av_dump_format(pPrewFFmpeg->ifmt_ctx, 0, NULL, 0);
    av_dump_format(pPrewFFmpeg->ofmt_ctx_v, 0, out_filename_v, 1);
    if (!(pPrewFFmpeg->ofmt_v->flags & AVFMT_NOFILE)) {
        if (avio_open(&(pPrewFFmpeg->ofmt_ctx_v->pb), out_filename_v, AVIO_FLAG_WRITE) < 0) {
            app_err( "Could not open output file '%s', %d", out_filename_v, pObjParam->id);
            goto end;
        }
    }
    if (pPreviewParams->videoindex != -1 && avformat_write_header(pPrewFFmpeg->ofmt_ctx_v, NULL) < 0) {
        app_err( "Error occurred when opening video output file, %d", pObjParam->id);
        goto end;
    }
    init = 0;

end:
    if(init != 0) {
        freePrewFFmpeg(pPrewFFmpeg, pPreviewParams->videoindex);
    }

    return init;
}

int previewUnInit(void *obj) {
    objParam *pObjParam = (objParam *)obj;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;
    previewFFmpeg *pPrewFFmpeg = (previewFFmpeg *)pPreviewParams->prewFFmpeg;

    if(pPrewFFmpeg == NULL) {
        return -1;
    }
    freePrewFFmpeg(pPrewFFmpeg, pPreviewParams->videoindex);

    char pathBuf[256];
    snprintf(pathBuf, sizeof(pathBuf), "%s/webpages/m3u8/stream%d", pConfigParams->workdir, pObjParam->id);
    delLastTsFile(pathBuf);

    return 0;
}

int previewLoop(void *obj) {
    AVPacket pkt;
    AVFormatContext *ofmt_ctx;
    AVStream *in_stream, *out_stream;
    AVFormatContext *ifmt_ctx, *ofmt_ctx_v;
    int frame_index = 0;
    int exception_index = 0;
    objParam *pObjParam = (objParam *)obj;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    previewParams *pPreviewParams = (previewParams *)pTaskParams->previewArgs;
    previewFFmpeg *pPrewFFmpeg = (previewFFmpeg *)pPreviewParams->prewFFmpeg;
    int videoindex = pPreviewParams->videoindex;

    ifmt_ctx = pPrewFFmpeg->ifmt_ctx;
    ofmt_ctx_v = pPrewFFmpeg->ofmt_ctx_v;
    while(pPreviewParams->running) {
        if(av_read_frame(ifmt_ctx, &pkt) < 0) {
            printf("id:%d, read frame failed, continue\n", pObjParam->id);
            usleep(200000);
            continue;
        }
        in_stream  = ifmt_ctx->streams[pkt.stream_index];

        if(pkt.pts == AV_NOPTS_VALUE) {
            AVRational time_base1=in_stream->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(in_stream->r_frame_rate);
            pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
            pkt.dts=pkt.pts;
            pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
        }

        if(pkt.stream_index == videoindex) {
            out_stream = ofmt_ctx_v->streams[0];
            ofmt_ctx=ofmt_ctx_v;
            if(exception_index > 0) {
                exception_index = 0;
            }
        }
        else{
            printf("exception videoindex, %d:%d, %d\n", pkt.stream_index, videoindex, pObjParam->id);
            exception_index ++;
            if(exception_index > 500) {
                app_warring("exception videoindex, %d:%d, %d", pkt.stream_index, videoindex, pObjParam->id);
                pPreviewParams->running = 0;
            }
            continue;
        }

        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, 
                out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, 
                out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        pkt.stream_index=0;
        if(av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
            printf("Error muxing packet, id:%d\n", pObjParam->id);
            break;
        }
        av_free_packet(&pkt);

        frame_index++;
        if(pPreviewParams->checkFunc != NULL && frame_index % 100 == 0) {
            pPreviewParams->checkFunc(NULL, pObjParam);
        }
    }

    return 0;
}

