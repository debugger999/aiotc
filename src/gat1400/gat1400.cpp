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

#include "platform.h"
#include "system.h"
#include "pids.h"
#include "msg.h"
#include "obj.h"
#include "task.h"
#include "camera.h"
#include "work.h"
#include "mq.h"
#include "misc.h"
#include "gat1400.h"
#include "gat1400sdk.h"

static long long gat1400Timestamp(const char *time) {
    struct tm _time;
    time_t timeStamp;

    if (time == NULL || time[0] == '\0') {
        return 0;
    }
    strptime(time, "%Y%m%d%H%M%S", &_time);
    timeStamp = mktime(&_time);

    return timeStamp;
}

static int ga1400traverseQueue(queue_common_1400 *queue, void *arg, int (*callBack)(node_common_1400 *p, void *arg)) {
    node_common_1400 *p = queue->head;
    while(p != NULL) {
        if(callBack(p, arg) != 0) {
            break;
        }
        p = p->next;
    }
    return 0;
}
    
// img->Type -- 14:全景, 11:人脸, 10:人员图, 01:车辆大图, 02:车辆彩色小图, 12:非机动车图
static int parseImage(node_common_1400 *p, void *arg) {
    char filename[512];
    outJsonParams *pJsonParams = (outJsonParams *)arg;
    objParam *pObjParam = (objParam *)pJsonParams->arg;
    SubImageInfo *img = (SubImageInfo *)p->arg;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    workParams *pWorkParams = (workParams *)pAiotcParams->workArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    systemParams *pSystemParams = (systemParams *)pAiotcParams->systemArgs;
    gat1400ObjParams *pGat1400ObjParams = (gat1400ObjParams *)pTaskParams->captureArgs;
    int randnum = rand();

    //printf("gat1400 capture, id:%d, type:%s, %dx%d, size:%d\n", 
    //        pObjParam->id, img->Type, img->Width, img->Height, img->FileSize);

    if(pJsonParams->timestamp == 0) {
        pJsonParams->timestamp = gat1400Timestamp(img->ShotTime);
    }
    snprintf(filename, sizeof(filename), "%s/webpages/img/%s/%d/%d_%lld_0_%s_%d.jpg", 
            pConfigParams->workdir, pWorkParams->date, pObjParam->id, 
            pGat1400ObjParams->frameId, pJsonParams->timestamp, img->Type, randnum);
    writeFile(filename, img->Data, img->FileSize);
    snprintf(filename, sizeof(filename), "http://%s:%d/img/%s/%d/%d_%lld_0_%s_%d.jpg", 
            pAiotcParams->localIp, pSystemParams->httpPort, pWorkParams->date, 
            pObjParam->id, pGat1400ObjParams->frameId, pJsonParams->timestamp, img->Type, randnum);
    pJsonParams->id = pObjParam->id;
    if(!strncmp(img->Type, "14", sizeof(img->Type))) {
        strncpy(pJsonParams->sceneUrl, filename, sizeof(pJsonParams->sceneUrl));
    }
    else if(!strncmp(img->Type, "11", sizeof(img->Type))) {
        strncpy(pJsonParams->faceUrl, filename, sizeof(pJsonParams->faceUrl));
    }
    else if(!strncmp(img->Type, "10", sizeof(img->Type))) {
        strncpy(pJsonParams->personBodyUrl, filename, sizeof(pJsonParams->personBodyUrl));
    }
    else if(!strncmp(img->Type, "01", sizeof(img->Type))) {
        strncpy(pJsonParams->sceneUrl, filename, sizeof(pJsonParams->sceneUrl));
    }
    else if(!strncmp(img->Type, "02", sizeof(img->Type))) {
        strncpy(pJsonParams->vehBodyUrl, filename, sizeof(pJsonParams->vehBodyUrl));
    }
    else if(!strncmp(img->Type, "12", sizeof(img->Type))) {
        strncpy(pJsonParams->personBodyUrl, filename, sizeof(pJsonParams->personBodyUrl));
    }
    else {
        printf("gat1400, unsupport img type:%s\n", img->Type);
    }

    return 0;
}

static int parseFaceList(node_common_1400 *p, void *arg) {
    outJsonParams *pJsonParams = (outJsonParams *)arg;
    Face *face = (Face *)p->arg;

    pJsonParams->faceRect.x = face->LeftTopX/1000.0;
    pJsonParams->faceRect.y = face->LeftTopY/1000.0;
    pJsonParams->faceRect.w = (face->RightBtmX - face->LeftTopX)/1000.0;
    pJsonParams->faceRect.h = (face->RightBtmY - face->LeftTopY)/1000.0;
    pJsonParams->timestamp = gat1400Timestamp(face->ShotTime);
    ga1400traverseQueue(&(face->SubImageList), pJsonParams, parseImage);

    return 0;
}

static int parsePersonList(node_common_1400 *p, void *arg) {
    outJsonParams *pJsonParams = (outJsonParams *)arg;
    Person *person = (Person *)p->arg;

    pJsonParams->personBodyRect.x = person->LeftTopX/1000.0;
    pJsonParams->personBodyRect.y = person->LeftTopY/1000.0;
    pJsonParams->personBodyRect.w = (person->RightBtmX - person->LeftTopX)/1000.0;
    pJsonParams->personBodyRect.h = (person->RightBtmY - person->LeftTopY)/1000.0;
    pJsonParams->timestamp = gat1400Timestamp(person->ShotTime);
    ga1400traverseQueue(&(person->SubImageList), pJsonParams, parseImage);

    return 0;
}

static int parseVehList(node_common_1400 *p, void *arg) {
    outJsonParams *pJsonParams = (outJsonParams *)arg;
    MotorVehicle *veh = (MotorVehicle *)p->arg;

    pJsonParams->vehBodyRect.x = veh->LeftTopX/1000.0;
    pJsonParams->vehBodyRect.y = veh->LeftTopY/1000.0;
    pJsonParams->vehBodyRect.w = (veh->RightBtmX - veh->LeftTopX)/1000.0;
    pJsonParams->vehBodyRect.h = (veh->RightBtmY - veh->LeftTopY)/1000.0;
    ga1400traverseQueue(&(veh->SubImageList), pJsonParams, parseImage);

    return 0;
}

static int parseNonMotorList(node_common_1400 *p, void *arg) {
    outJsonParams *pJsonParams = (outJsonParams *)arg;
    NonMotorVehicle *nonmotor = (NonMotorVehicle *)p->arg;

    pJsonParams->personBodyRect.x = nonmotor->LeftTopX/1000.0;
    pJsonParams->personBodyRect.y = nonmotor->LeftTopY/1000.0;
    pJsonParams->personBodyRect.w = (nonmotor->RightBtmX - nonmotor->LeftTopX)/1000.0;
    pJsonParams->personBodyRect.h = (nonmotor->RightBtmY - nonmotor->LeftTopY)/1000.0;
    ga1400traverseQueue(&(nonmotor->SubImageList), pJsonParams, parseImage);

    return 0;
}

static void gat1400_capture_callback(CallData *data, void *userArg) {
    objParam *pObjParam = (objParam *)userArg;
    ObjList *pData = (ObjList *)data->data;
    int (*parseObjList)(node_common_1400 *, void *) = NULL;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    workParams *pWorkParams = (workParams *)pAiotcParams->workArgs;
    gat1400ObjParams *pGat1400ObjParams = (gat1400ObjParams *)pTaskParams->captureArgs;
    outJsonParams *pJsonParams = &pGat1400ObjParams->jsonParams;

    printf("gat1400 capture, id:%d, type:%d\n", pObjParam->id, data->datatype);

    memset(pJsonParams, 0, sizeof(outJsonParams));
    strncpy(pJsonParams->msgType, "common", sizeof(pJsonParams->msgType));
    pJsonParams->id = pObjParam->id;
    pJsonParams->arg = pObjParam;
    pGat1400ObjParams->frameId ++;

    if(data->datatype == GAT1400_FACE) {
        parseObjList = parseFaceList;
    }
    else if(data->datatype == GAT1400_PERSON) {
        parseObjList = parsePersonList;
    }
    else if(data->datatype == GAT1400_VEH) {
        parseObjList = parseVehList;
    }
    else if(data->datatype == GAT1400_NONMOTOR) {
        parseObjList = parseNonMotorList;
    }
    else {
        printf("gat1400 capture, id:%d, unsupport data type:%d\n", pObjParam->id, data->datatype);
        return ;
    }

    ga1400traverseQueue(&pData->objList, pJsonParams, parseObjList);
    char *json = makeJson(pJsonParams);
    if(json != NULL) {
        //printf("json:%s\n", json);
        copyToMqQueue(json, pWorkParams);
        free(json);
    }
}

static int gat1400_start(const void *buf, void *arg) {
    int mode;
    objParam *pObjParam = (objParam *)arg;
    char *buff = pObjParam->originaldata;
    pidOps *pOps = (pidOps *)pObjParam->reserved;
    taskOps *pTaskOps = (taskOps *)pOps->procTaskOps;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    gat1400Params *pGat1400Params = (gat1400Params *)pTaskOps->arg;
    char *deviceId = NULL, *platformId = NULL;

    if(pGat1400Params->handle == NULL) {
        app_warning("id:%d, please init first", pObjParam->id);
        goto end;
    }
    mode = getIntValFromJson(buff, "data", "mode", NULL);
    deviceId = getStrValFromJson(buff, "data", "deviceId", NULL);
    platformId = getStrValFromJson(buff, "data", "platformId", NULL);
    if(deviceId == NULL || platformId == NULL) {
        app_warning("id:%d, get obj params failed", pObjParam->id);
        goto end;
    }

    if(pTaskParams->captureArgs == NULL) {
        pTaskParams->captureArgs = malloc(sizeof(gat1400ObjParams));
        if(pTaskParams->captureArgs == NULL) {
            app_err("malloc failed");
            goto end;
        }
        memset(pTaskParams->captureArgs, 0, sizeof(gat1400ObjParams));
    }

    gat1400_start_capture(pGat1400Params->handle, deviceId, gat1400_capture_callback, 
            pObjParam, mode, platformId,"FACE", "APE");

end:
    if(deviceId != NULL) {
        free(deviceId);
    }
    if(platformId != NULL) {
        free(platformId);
    }

    return 0;
}

static int gat1400_stop(const void *buf, void *arg) {
    char *deviceId = NULL;
    objParam *pObjParam = (objParam *)arg;
    char *buff = pObjParam->originaldata;
    pidOps *pOps = (pidOps *)pObjParam->reserved;
    taskOps *pTaskOps = (taskOps *)pOps->procTaskOps;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    gat1400Params *pGat1400Params = (gat1400Params *)pTaskOps->arg;

    if(pGat1400Params->handle == NULL) {
        app_warning("id:%d, please init first", pObjParam->id);
        goto end;
    }
    deviceId = getStrValFromJson(buff, "data", "deviceId", NULL);
    if(deviceId == NULL) {
        app_warning("id:%d, get obj params failed", pObjParam->id);
        goto end;
    }

    gat1400_stop_capture(pGat1400Params->handle, deviceId);
    if(pTaskParams->captureArgs != NULL) {
        free(pTaskParams->captureArgs);
        pTaskParams->captureArgs = NULL;
    }

end:
    if(deviceId != NULL) {
        free(deviceId);
    }

    return 0;
}

static taskOps gat1400TaskOps = {
    .type = NULL,
    .init = NULL,
    .uninit = NULL,
    .start = gat1400_start,
    .stop = gat1400_stop,
    .ctrl = NULL
};

static int initGat1400Params(pidOps *pOps) {
    char *str, *buf;
    const char *config = AIOTC_CFG;
    taskOps *pTaskOps = (taskOps *)pOps->procTaskOps;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;
    gat1400Params *pGat1400Params = (gat1400Params *)pTaskOps->arg;
    systemParams *pSystemParams = (systemParams *)pAiotcParams->systemArgs;

    buf = pSystemParams->sysOrgData;
    if(buf == NULL) {
        app_warning("please system init first");
        return -1;
    }

    str = getStrValFromJson(buf, "gat1400Params", "localGatServerId", NULL);
    if(str != NULL) {
        strncpy(pGat1400Params->localGatServerId, str, sizeof(pGat1400Params->localGatServerId));
        free(str);
    }
    str = getStrValFromJson(buf, "gat1400Params", "localGatHostIp", NULL);
    if(str != NULL) {
        strncpy(pGat1400Params->localGatHostIp, str, sizeof(pGat1400Params->localGatHostIp));
        free(str);
    }
    pGat1400Params->localGatPort = getIntValFromJson(buf, "gat1400Params", "localGatPort", NULL);
    if(pGat1400Params->localGatPort < 0) {
        pGat1400Params->localGatPort = 7100;
    }

    pGat1400Params->threadNum = getIntValFromFile(config, "proc", "gat1400", "threadNum");
    if(pGat1400Params->threadNum < 0) {
        pGat1400Params->threadNum = 1;
    }
    pGat1400Params->authEnable = getIntValFromFile(config, "proc", "gat1400", "authEnable");
    if(pGat1400Params->authEnable) {
        str = getStrValFromFile(config, "proc", "gat1400", "username");
        if(str != NULL) {
            strncpy(pGat1400Params->userName, str, sizeof(pGat1400Params->userName));
            free(str);
        }
        str = getStrValFromFile(config, "proc", "gat1400", "password");
        if(str != NULL) {
            strncpy(pGat1400Params->password, str, sizeof(pGat1400Params->password));
            free(str);
        }
    }

    pGat1400Params->handle = gat1400_init(pGat1400Params->threadNum, pGat1400Params->localGatPort, 
            pGat1400Params->localGatServerId, pAiotcParams->localIp, pGat1400Params->localGatHostIp, 
            pGat1400Params->authEnable, pGat1400Params->userName, pGat1400Params->password);
    if(pGat1400Params->handle == NULL) {
        app_err("gat1400 init failed, port:%d , serverId%s", 
                pGat1400Params->localGatPort, pGat1400Params->localGatServerId);
        return -1;
    }
    app_debug("gat1400 init ok, host:%s, %s:%d, serverId:%s, auth:%d, %s:%s", 
            pGat1400Params->localGatHostIp, pAiotcParams->localIp, pGat1400Params->localGatPort, 
            pGat1400Params->localGatServerId, pGat1400Params->authEnable, 
            pGat1400Params->userName, pGat1400Params->password);

    int size = 0;
    char *platform = getArrayBufFromJson(buf, "gat1400Params", "platform", NULL, size);
    if (platform != NULL) {
        for (int i = 0; i < size && i < GAT1400_PLATFROM_MAX; i++) {
            char *arrbuf = getBufFromArray(platform, NULL, NULL, NULL, i);
            if(arrbuf == NULL) {
                break;
            }
            str = getStrValFromJson(arrbuf, "id", NULL, NULL);
            if (str != NULL) {
                strncpy(pGat1400Params->platform[i].platformId, str, 
                        sizeof(pGat1400Params->platform[i].platformId));
                free(str);
            }
            str = getStrValFromJson(arrbuf, "ip", NULL, NULL);
            if (str != NULL) {
                strncpy(pGat1400Params->platform[i].platformIp, str, 
                        sizeof(pGat1400Params->platform[i].platformIp));
                free(str);
            }
            str = getStrValFromJson(arrbuf, "username", NULL, NULL);
            if (str != NULL) {
                strncpy(pGat1400Params->platform[i].platformUsername, str, 
                        sizeof(pGat1400Params->platform[i].platformUsername));
                free(str);
            }
            str = getStrValFromJson(arrbuf, "password", NULL, NULL);
            if (str != NULL) {
                strncpy(pGat1400Params->platform[i].platformPassword, str, 
                        sizeof(pGat1400Params->platform[i].platformPassword));
                free(str);
            }
            str = getStrValFromJson(arrbuf, "extra", "manufactor", NULL);
            if (str != NULL) {
                strncpy(pGat1400Params->platform[i].platformName, str, 
                        sizeof(pGat1400Params->platform[i].platformName));
                free(str);
            }
            pGat1400Params->platform[i].platformPort = getIntValFromJson(arrbuf, "port", NULL, NULL);
            if(pGat1400Params->platform[i].platformPort < 0) {
                pGat1400Params->platform[i].platformPort = 7100;
            }
            gat1400_add_platform(pGat1400Params->handle, pGat1400Params->platform[i].platformId, 
                    pGat1400Params->platform[i].platformIp, pGat1400Params->platform[i].platformPort, 
                    pGat1400Params->platform[i].platformUsername, pGat1400Params->platform[i].platformPassword, 
                    pGat1400Params->platform[i].platformName);
            free(arrbuf);
        }
        free(platform);
    }

    return 0;
}

static int gat1400Init(pidOps *pOps) {
    taskOps *pTaskOps = (taskOps *)pOps->procTaskOps;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    pOps->taskMax = pConfigParams->slaveObjMax;
    if(pOps->taskMax <= 0) {
        app_warning("get task max failed, %s-%s-%s", pOps->name, pOps->subName, pOps->taskName);
        pOps->taskMax = 1;
    }

    pTaskOps->arg = malloc(sizeof(gat1400Params));
    memset(pTaskOps->arg, 0, sizeof(gat1400Params));
    initGat1400Params(pOps);
    dirCheck("data");

    return 0;
}

static int gat1400ProcBeat(node_common *p, void *arg) {
    pidOps *pOps = (pidOps *)arg;
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;

    if(pTaskParams->capture) {
        pTaskParams->captureBeat = (int)pOps->tv.tv_sec;
    }
    else if(pTaskParams->capture == 0 && pTaskParams->captureTaskBeat == 0) {
        delObjFromPidQue(pObjParam->id, pOps, 0);
        pTaskParams->captureBeat = 0;
    }

    return 0;
}

static int gat1400TaskBeat(node_common *p, void *arg) {
    pidOps *pOps = (pidOps *)arg;
    objParam *pObjParam = (objParam *)p->name;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    taskOps *pTaskOps = (taskOps *)pOps->procTaskOps;
    int nowSec = (int)pOps->tv.tv_sec;

    if(pTaskParams->capture) {
        if(pTaskParams->captureTaskBeat == 0) {
            pTaskOps->start("capture", pObjParam);
            pTaskParams->captureTaskBeat = nowSec;
        }
        else if(nowSec - pTaskParams->captureTaskBeat > TASK_BEAT_TIMEOUT) {
            //printf("id:%d, %s, detected exception, restart it ...\n", pObjParam->id, pOps->taskName);
            pTaskParams->captureTaskBeat = nowSec;
        }
    }
    else if(pTaskParams->captureTaskBeat > 0) {
        pTaskOps->stop("capture", pObjParam);
        pTaskParams->captureTaskBeat = 0;
    }

    return 0;
}

int gat1400Process(void *arg) {
    pidOps *pOps = (pidOps *)arg;
    aiotcParams *pAiotcParams = (aiotcParams *)pOps->arg;

    pOps = getRealOps(pOps);
    if(pOps == NULL) {
        return -1;
    }
    pOps->running = 1;

    initTaskOps(pOps, &gat1400TaskOps);
    gat1400Init(pOps);
    createBeatTask(pOps, gat1400ProcBeat, 5);
    createBeatTask(pOps, gat1400TaskBeat, TASK_BEAT_SLEEP);
    while(pAiotcParams->running && pOps->running) {
        sleep(2);
    }
    pOps->running = 0;

    app_debug("pid:%d, run over", pOps->pid);

    return 0;
}

