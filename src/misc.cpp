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

#include "misc.h"
#include "master.h"
#include "system.h"
#include "shm.h"
#include "obj.h"
#include "task.h"
#include "rest.h"
#include "alg.h"
#include "camera.h"
#include "work.h"

static int createDir(const char *sPathName) {
    char dirName[4096];
    strcpy(dirName, sPathName);

    int len = strlen(dirName);
    if(dirName[len-1]!='/')
        strcat(dirName, "/");

    len = strlen(dirName);
    int i=0;
    for(i=1; i<len; i++) {
        if(dirName[i]=='/') {
            dirName[i] = 0;
            if(access(dirName, R_OK)!=0) {
                if(mkdir(dirName, 0755)==-1) {
                    //TODO:errno:ENOSPC(No space left on device)
                    app_err("create path %s failed, %s", dirName, strerror(errno));
                    return -1;
                }
            }
            dirName[i] = '/';
        }
    }
    return 0;
}

int dirCheck(const char *dir) {
    DIR *pdir = opendir(dir);
    if(pdir == NULL)
        return createDir(dir);
    else
        return closedir(pdir);   
}

int checkObjDir(objParam *pObjParam) {
    char path[256];
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    workParams *pWorkParams = (workParams *)pAiotcParams->workArgs;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    if(pWorkParams->date[0] == '\0') {
        char date[32];
        struct tm _time;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &_time);
        snprintf(date, sizeof(date), "%d%02d%02d", _time.tm_year + 1900, _time.tm_mon + 1, _time.tm_mday);
        strncpy(pWorkParams->date, date, sizeof(date));
    }

    snprintf(path, sizeof(path), "%s/webpages/img/%s/%d", 
            pConfigParams->workdir, pWorkParams->date, pObjParam->id);
    dirCheck(path);

    return 0;
}

int conditionByObjId(node_common *p, void *arg) {
    int id = *(int *)arg;
    objParam *pObjParam = (objParam *)p->name;
    return id == pObjParam->id;
}

int conditionBySlaveIp(node_common *p, void *arg) {
    char *slaveIp = (char *)arg;
    slaveParam *pSlaveParam = (slaveParam *)p->name;
    return !strcmp(slaveIp, pSlaveParam->ip);
}

int systemInits(char *buf, aiotcParams *pAiotcParams) {
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;
    systemParams *pSystemParams = (systemParams *)pAiotcParams->systemArgs;

    if(pSystemParams->sysOrgData != NULL) {
        shmFree(pShmParams->headsp, pSystemParams->sysOrgData);
    }
    pSystemParams->sysOrgData = (char *)shmMalloc(pShmParams->headsp, strlen(buf) + 1);
    if(pSystemParams->sysOrgData == NULL) {
        app_err("shm malloc failed, %ld", strlen(buf) + 1);
        return -1;
    }
    strcpy(pSystemParams->sysOrgData, buf);

    return 0;
}

static int initObjArg(char *type, objParam *pObjParam) {
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    if(!strcmp(type, "camera")) {
        pObjParam->objArg = shmMalloc(pShmParams->headsp, sizeof(cameraParams));
        if(pObjParam->objArg == NULL) {
            app_err("malloc failed");
            return -1;
        }
        memset(pObjParam->objArg, 0, sizeof(cameraParams));
    }
    else {
        app_warning("unsupport obj type : %s", type);
        return -1;
    }

    return 0;
}

static int initObjTask(char *buf, objParam *pObjParam) {
    int livestream, capture, record;
    char *slaveIp = NULL, *preview = NULL;
    taskParams *pTaskParams = (taskParams *)pObjParam->task;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    masterParams *pMasterParams = (masterParams *)pAiotcParams->masterArgs;

    slaveIp = getStrValFromJson(buf, "slave", "ip", NULL);
    livestream = getIntValFromJson(buf, "task", "stream", NULL);
    capture = getIntValFromJson(buf, "task", "capture", NULL);
    record = getIntValFromJson(buf, "task", "record", NULL);
    preview = getStrValFromJson(buf, "task", "preview", NULL);
    if(livestream >= 0) {
        pTaskParams->livestream = livestream;
    }
    if(capture >= 0) {
        pTaskParams->capture = capture;
    }
    if(record >= 0) {
        pTaskParams->record = record;
    }
    if(preview != NULL && strlen(preview) > 0) {
        strncpy(pTaskParams->preview, preview, sizeof(pTaskParams->preview));
        free(preview);
    }
    if(slaveIp != NULL) {
        node_common *p = NULL;
        semWait(&pMasterParams->mutex_slave);
        searchFromQueue(&pMasterParams->slaveQueue, slaveIp, &p, conditionBySlaveIp);
        semPost(&pMasterParams->mutex_slave);
        if(p != NULL) {
            pObjParam->slave = p->name;
        }
        free(slaveIp);
    }

    return 0;
}

int addObj(char *buf, aiotcParams *pAiotcParams, int max, void *arg) {
    int ret;
    node_common node;
    taskParams *pTaskParams;
    char *name = NULL, *type = NULL, *subtype = NULL;
    objParam *pObjParam = (objParam *)node.name;
    commonParams *pParams = (commonParams *)arg;
    sem_t *mutex = (sem_t *)pParams->arga;
    queue_common *queue = (queue_common *)pParams->argb;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    memset(&node, 0, sizeof(node));
    name = getStrValFromJson(buf, "name", NULL, NULL);
    type = getStrValFromJson(buf, "type", NULL, NULL);
    subtype = getStrValFromJson(buf, "data", "subtype", NULL);
    pObjParam->id = getIntValFromJson(buf, "id", NULL, NULL);
    if(pObjParam->id < 0 || name == NULL || type == NULL || subtype == NULL) {
        goto end;
    }
    strncpy(pObjParam->name, name, sizeof(pObjParam->name));
    strncpy(pObjParam->type, type, sizeof(pObjParam->type));
    strncpy(pObjParam->subtype, subtype, sizeof(pObjParam->subtype));

    pObjParam->task = shmMalloc(pShmParams->headsp, sizeof(taskParams));
    pObjParam->originaldata = (char *)shmMalloc(pShmParams->headsp, strlen(buf) + 1);
    if(pObjParam->task == NULL || pObjParam->originaldata == NULL) {
        app_err("shm malloc failed");
        goto end;
    }
    memset(pObjParam->task, 0, sizeof(taskParams));
    strcpy((char *)pObjParam->originaldata, buf);
    pTaskParams = (taskParams *)pObjParam->task;
    if(sem_init(&pTaskParams->mutex_alg, 1, 1) < 0) {
      app_err("sem init failed");
      goto end;
    }
    pObjParam->arg = pAiotcParams;
    initObjTask(buf, pObjParam);
    if(initObjArg(type, pObjParam) != 0) {
        goto end;
    }

    semWait(mutex);
    ret = putToShmQueue(pShmParams->headsp, queue, &node, max);
    semPost(mutex);
    if(ret != 0) {
        shmFree(pShmParams->headsp, pObjParam->originaldata);
    }
    checkObjDir(pObjParam);

end:
    if(name != NULL) {
        free(name);
    }
    if(type != NULL) {
        free(type);
    }
    if(subtype != NULL) {
        free(subtype);
    }
    return 0;
}

int delObj(char *buf, aiotcParams *pAiotcParams, void *arg) {
    int id;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    sem_t *mutex = (sem_t *)pParams->arga;
    queue_common *queue = (queue_common *)pParams->argb;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    //TODO : stop something first
    id = getIntValFromJson(buf, "id", NULL, NULL);
    semWait(mutex);
    delFromQueue(queue, &id, &p, conditionByObjId);
    semPost(mutex);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        if(pObjParam->task != NULL) {
            taskParams *pTaskParams = (taskParams *)pObjParam->task;
            semWait(&pTaskParams->mutex_alg);
            freeShmQueue(pShmParams->headsp, &pTaskParams->algQueue, NULL);
            semPost(&pTaskParams->mutex_alg);
            sem_destroy(&pTaskParams->mutex_alg);
            shmFree(pShmParams->headsp, pObjParam->task);
        }
        if(pObjParam->originaldata != NULL) {
            shmFree(pShmParams->headsp, pObjParam->originaldata);
        }
        if(pObjParam->objArg != NULL) {
            shmFree(pShmParams->headsp, pObjParam->objArg);
        }
        if(p->arg != NULL) {
            shmFree(pShmParams->headsp, p->arg);
        }
        shmFree(pShmParams->headsp, p);
    }

    return 0;
}

int httpPostSlave(const char *url, char *buf, objParam *pObjParam) {
    char urladdr[256];
    slaveParam *pSlaveParam = (slaveParam *)pObjParam->slave;

    if(pObjParam->slave != NULL && pObjParam->attachSlave) {
        snprintf(urladdr, sizeof(urladdr), "http://%s:%d%s", pSlaveParam->ip, pSlaveParam->restPort, url);
        httpPost(urladdr, buf, NULL, 3);
    }

    return 0;
}

int addAlg(char *buf, int id, char *algName, aiotcParams *pAiotcParams, void *arg) {
    node_common node;
    node_common *p = NULL;
    commonParams *pParams = (commonParams *)arg;
    sem_t *mutex = (sem_t *)pParams->arga;
    queue_common *queue = (queue_common *)pParams->argb;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    memset(&node, 0, sizeof(node));
    semWait(mutex);
    searchFromQueue(queue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        algParams *pAlgParams = (algParams *)node.name;
        strncpy(pAlgParams->name, algName, sizeof(pAlgParams->name));
        semWait(&pTaskParams->mutex_alg);
        putToShmQueue(pShmParams->headsp, &pTaskParams->algQueue, &node, 100);
        semPost(&pTaskParams->mutex_alg);
        if(pParams->argc != NULL) {
            httpPostSlave("/task/start", buf, pObjParam);
        }
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(mutex);
    
    return 0;
}

int conditionByAlgName(node_common *p, void *arg) {
    char *algName = (char *)arg;
    algParams *pAlgParams = (algParams *)p->name;
    return !strncmp(algName, pAlgParams->name, sizeof(pAlgParams->name));
}

int delAlg(char *buf, int id, char *algName, aiotcParams *pAiotcParams, void *arg) {
    node_common *p = NULL, *palg = NULL;
    commonParams *pParams = (commonParams *)arg;
    sem_t *mutex = (sem_t *)pParams->arga;
    queue_common *queue = (queue_common *)pParams->argb;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    semWait(mutex);
    searchFromQueue(queue, &id, &p, conditionByObjId);
    if(p != NULL) {
        objParam *pObjParam = (objParam *)p->name;
        taskParams *pTaskParams = (taskParams *)pObjParam->task;
        semWait(&pTaskParams->mutex_alg);
        delFromQueue(&pTaskParams->algQueue, algName, &palg, conditionByAlgName);
        semPost(&pTaskParams->mutex_alg);
        if(palg != NULL) {
            if(palg->arg != NULL) {
                shmFree(pShmParams->headsp, palg->arg);
            }
            shmFree(pShmParams->headsp, palg);
        }
        if(pParams->argc != NULL) {
            httpPostSlave("/task/stop", buf, pObjParam);
        }
    }
    else {
        printf("objId %d not exsit\n", id);
    }
    semPost(mutex);
    
    return 0;
}

static int checkMsgKey(char *key_str, void *arg) {
    aiotcParams *pAiotcParams = (aiotcParams *)arg;
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;

    long key = strtol(key_str, NULL, 10);
    long tmp = key - pConfigParams->msgKeyStart;
    if(tmp>=0 && tmp<=pConfigParams->msgKeyMax)
        return 0;

    return -1;
}

static void clearMsg(char *id_str) {
    int id = atoi(id_str);
    msgctl(id, IPC_RMID, NULL);
}

//1:yes 0:no
static int check_is_num_str(char *str) {
    char s = 0;
    int len = strlen(str);

    if(len == 0)
        return 0;

    while((s = (* (str++)) )
            !='\0' )
    {
        if(!isdigit(s))
        {
            return 0; 
        }
    }
    return 1;
}

static void dealBuffer(char *buffer, int (*check)(char *, void *), void(*clear)(char *), void *arg) {
    char *p = buffer;
    char preflag = 0; //1:number or letters
    char *key_str = NULL;
    char *id_str = NULL;
    while(*p!='\0' && *p!='\n')
    {
        if(!isalnum(*p))
        {
            if(preflag==1)
            {
                *p = '\0';
                if(key_str!=NULL)
                {
                    if(!check_is_num_str(key_str))
                    {
                        key_str = NULL;
                        break;
                    }
                }    
                else if(id_str!=NULL)
                {
                    if(!check_is_num_str(id_str))
                        id_str = NULL;
                    break;
                }
            }
            preflag=0;
        }else{
            if(preflag==0)
            {
                if(key_str == NULL)
                    key_str = p;
                else if(id_str == NULL)
                    id_str = p;
            }
            preflag = 1;
        }
        p++;
    }
    if(key_str!=NULL && id_str!=NULL)
    {
        if(!check(key_str, arg))
        {
            //printf("clean key:%s , id:%s\n",key_str,id_str);
            clear(id_str);
        } 
    }
}

int clearSystemIpc(aiotcParams *pAiotcParams) {
    char buf[1024] = {0};
    FILE *fp = fopen("/proc/sysvipc/msg","r");
    if(fp != NULL) {
        while(fgets(buf, sizeof(buf)-1, fp)!=NULL) {
            dealBuffer(buf, checkMsgKey, clearMsg, pAiotcParams);
        }
        fclose(fp);
    } 
    return 0;
}

int allocVideoShm(objParam *pObjParam) {
    cameraParams *pCameraParams = (cameraParams *)pObjParam->objArg;
    aiotcParams *pAiotcParams = (aiotcParams *)pObjParam->arg;
    shmParams *pShmParams = (shmParams *)pAiotcParams->shmArgs;

    if(pCameraParams->videoShm != NULL) {
        return 0;
    }
    pCameraParams->videoShm = getFreeShm(pShmParams, "video");
    if(pCameraParams->videoShm == NULL) {
        app_warning("get free shm failed, id:%d", pObjParam->id);
        return -1;
    }
    app_debug("get free shm success, id:%d, shmid:%d", pObjParam->id, pCameraParams->videoShm->id);

    return 0;
}

