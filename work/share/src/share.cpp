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
#include "typedef.h"
#include "cJSON.h"
#include "shm.h"
#include "share.h"

int getLocalIp(char hostIp[128]) {
    int sock;
    struct ifconf conf;
    struct ifreq *ifr;
    char buff[BUFSIZ];
    int num;
    int i;
    unsigned int u32_addr = 0;
    char str_ip[16] = {0};

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    conf.ifc_len = BUFSIZ;
    conf.ifc_buf = buff;
    /*
       SIOCGIFCONF
       返 回系统中配置的所有接口的配置信息。
    */
    ioctl(sock, SIOCGIFCONF, &conf);
    num = conf.ifc_len / sizeof(struct ifreq);
    ifr = conf.ifc_req;

    for (i = 0; i < num; i++) {
        // SIOCGIFFLAGS 可以获取接口标志。
        ioctl(sock, SIOCGIFFLAGS, ifr);
        u32_addr = ((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr.s_addr;

        if (((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP)) {
            if(strstr(ifr->ifr_name,"docker") || 
                    strstr(ifr->ifr_name,"cicada") ||
                    strstr(ifr->ifr_name,":")) {
                continue;
            }

            inet_ntop(AF_INET, &u32_addr, str_ip, (socklen_t )sizeof(str_ip));

            if(strstr(ifr->ifr_name,"eth")||strstr(ifr->ifr_name,"em") ) {
                break;
            }
        }
        ifr++;
    }

    if(str_ip == NULL) {
        printf("str_ip is NULL\n");
        return -1;
    }
    strncpy(hostIp, str_ip, 128);

    return 0;
}

int getIntValFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3) {
    char name[256];
    int val = -1;
    cJSON *root, *pSub1, *pSub2, *pSub3;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        app_err("cJSON_Parse err, buf:%s", buf);
        goto err;
    }

    if(nameSub1 == NULL) {
        app_err("nameSub1 is null");
        goto err;
    }

    strncpy(name, nameSub1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        //printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub2 == NULL) {
        val = pSub1->valueint;
        goto err;
    }

    strncpy(name, nameSub2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        //printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub3 == NULL) {
        val = pSub2->valueint;
        goto err;
    }

    strncpy(name, nameSub3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        //printf("get json null, %s\n", name);
        goto err;
    }
    val = pSub3->valueint;

err:
    if(root != NULL) {
        cJSON_Delete(root);
    }

    return val;
}

int getIntValFromArray(char *buf, char *nameSub1, char *nameSub2, char *nameSub3, int *pInt, int max) {
    char name[256];
    int num = 0;
    cJSON *pArray = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3, *pSub;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        app_err("cJSON_Parse err, buf:%s", buf);
        goto err;
    }

    if(nameSub1 == NULL) {
        app_err("nameSub1 is null");
        goto err;
    }

    strncpy(name, nameSub1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub2 == NULL) {
        pArray = pSub1;
        goto err;
    }

    strncpy(name, nameSub2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub3 == NULL) {
        pArray = pSub2;
        goto err;
    }

    strncpy(name, nameSub3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    pArray = pSub3;

err:
    if(pArray != NULL) {
        int i;
        int size = cJSON_GetArraySize(pArray);
        for(i = 0; i < size && i < max; i ++) {
            pSub = cJSON_GetArrayItem(pArray, i);
            if(pSub != NULL) {
                pInt[i] = pSub->valueint;
                num ++;
            }
            else {
                break;
            }
        }
    }

    if(root != NULL) {
        cJSON_Delete(root);
    }

    return num;
}

char *getBufFromArray(char *buf, char *nameSub1, char *nameSub2, char *nameSub3, int index) {
    char name[256];
    char *val = NULL;
    cJSON *pArray = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3, *pSub;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        app_err("cJSON_Parse err, buf:%s", buf);
        goto err;
    }

    if(nameSub1 == NULL) {
        pArray = root;
        printf("nameSub1 is null\n");
        goto err;
    }

    strncpy(name, nameSub1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub2 == NULL) {
        pArray = pSub1;
        goto err;
    }

    strncpy(name, nameSub2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub3 == NULL) {
        pArray = pSub2;
        goto err;
    }

    strncpy(name, nameSub3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    pArray = pSub3;

err:
    if(pArray != NULL) {
        int size = cJSON_GetArraySize(pArray);
        if(size > 0 && index >= 0 && index < size) {
            pSub = cJSON_GetArrayItem(pArray, index);
            val = cJSON_Print(pSub);
        }
        else {
            //printf("get array failed, size:%d, index:%d\n", size, index);
        }
    }
    else {
        printf("pArray is null\n");
    }
    if(root != NULL) {
        cJSON_Delete(root);
    }

    return val;
}

char *getArrayBufFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3, int &size){
        char name[256];
    char *val = NULL;
    cJSON *pArray = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        app_err("cJSON_Parse err, buf:%s", buf);
        goto err;
    }

    if(nameSub1 == NULL) {
        app_err("nameSub1 is null");
        goto err;
    }

    strncpy(name, nameSub1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub2 == NULL) {
        pArray = pSub1;
        goto err;
    }

    strncpy(name, nameSub2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub3 == NULL) {
        pArray = pSub2;
        goto err;
    }

    strncpy(name, nameSub3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    pArray = pSub3;

err:
    if(pArray != NULL) {
        size = cJSON_GetArraySize(pArray);
        val = cJSON_Print(pArray);
    }
    else {
        printf("pArray is null\n");
    }
    if(root != NULL) {
        cJSON_Delete(root);
    }

    return val;
}

double getDoubleValFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3) {
    char name[256];
    double val = -1;
    cJSON *root, *pSub1, *pSub2, *pSub3;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        app_err("cJSON_Parse err, buf:%s", buf);
        goto err;
    }

    if(nameSub1 == NULL) {
        app_err("nameSub1 is null");
        goto err;
    }

    strncpy(name, nameSub1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        app_err("get json failed, %s", name);
        goto err;
    }
    if(nameSub2 == NULL) {
        val = pSub1->valuedouble;
        goto err;
    }

    strncpy(name, nameSub2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        app_err("get json failed, %s", name);
        goto err;
    }
    if(nameSub3 == NULL) {
        val = pSub2->valuedouble;
        goto err;
    }

    strncpy(name, nameSub3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        app_err("get json failed, %s", name);
        goto err;
    }
    val = pSub3->valuedouble;

err:
    if(root != NULL) {
        cJSON_Delete(root);
    }

    return val;
}

char *getStrValFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3) {
    char name[256];
    char *val = NULL, *valTmp = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        app_err("cJSON_Parse err, buf:%s", buf);
        goto err;
    }

    if(nameSub1 == NULL) {
        //printf("get json null, %s\n", name);
        goto err;
    }

    strncpy(name, nameSub1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        //printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub2 == NULL) {
        valTmp = pSub1->valuestring;
        goto err;
    }

    strncpy(name, nameSub2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        //printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub3 == NULL) {
        valTmp = pSub2->valuestring;
        goto err;
    }

    strncpy(name, nameSub3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        //printf("get json null, %s\n", name);
        goto err;
    }
    valTmp = pSub3->valuestring;

err:
    if(valTmp != NULL) {
        int len = (strlen(valTmp)+1)/1024*1024 + 1024;
        val = (char *)malloc(len);
        if(val != NULL) {
            strncpy(val, valTmp, len);
        }
        else {
            app_err("malloc val failed");
        }
    }
    if(root != NULL) {
        cJSON_Delete(root);
    }

    return val;
}

char *getObjBufFromJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3) {
    char name[256];
    char *val = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        app_err("cJSON_Parse err, buf:%s", buf);
        goto err;
    }

    if(nameSub1 == NULL) {
        app_err("nameSub1 is null");
        goto err;
    }

    strncpy(name, nameSub1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub2 == NULL) {
        val = cJSON_Print(pSub1);
        goto err;
    }

    strncpy(name, nameSub2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub3 == NULL) {
        val = cJSON_Print(pSub2);
        goto err;
    }

    strncpy(name, nameSub3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    val = cJSON_Print(pSub3);

err:
    if(root != NULL) {
        cJSON_Delete(root);
    }

    return val;
}

cJSON *getObjFromJson(char *buf, char *nameSub1, char *nameSub2, char *nameSub3) {
    char name[256];
    cJSON *val = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3;

    root = cJSON_Parse(buf);
    if(root == NULL) {
        app_err("cJSON_Parse err, buf:%s", buf);
        goto err;
    }

    if(nameSub1 == NULL) {
        app_err("nameSub1 is null");
        goto err;
    }

    strncpy(name, nameSub1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub2 == NULL) {
        val = pSub1;
        goto err;
    }

    strncpy(name, nameSub2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub3 == NULL) {
        val = pSub2;
        goto err;
    }

    strncpy(name, nameSub3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    val = pSub3;

err:
    if(root != NULL) {
        cJSON_Delete(root);
    }

    return val;
}

char *delObjJson(char *buf, const char *nameSub1, const char *nameSub2, const char *nameSub3) {
    const char *pName;
    char name[256];
    char *val = NULL;
    cJSON *pSub = NULL;
    cJSON *root, *pSub1, *pSub2, *pSub3;
    
    root = cJSON_Parse(buf);
    if(root == NULL) {
        app_err("cJSON_Parse err, buf:%s", buf);
        goto err;
    }
    
    if(nameSub1 == NULL) {
        app_err("nameSub1 is null");
        goto err;
    }
    strncpy(name, nameSub1, 256);
    pSub1 = cJSON_GetObjectItem(root, name);
    if(pSub1 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub2 == NULL) {
        pSub = root;
        pName = nameSub1;
        goto err;
    }

    strncpy(name, nameSub2, 256);
    pSub2 = cJSON_GetObjectItem(pSub1, name);
    if(pSub2 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    if(nameSub3 == NULL) {
        pSub = pSub1;
        pName = nameSub2;
        goto err;
    }

    strncpy(name, nameSub3, 256);
    pSub3 = cJSON_GetObjectItem(pSub2, name);
    if(pSub3 == NULL) {
        printf("get json null, %s\n", name);
        goto err;
    }
    pSub = pSub2;
    pName = nameSub3;

err:
    if(pSub != NULL) {
       cJSON_DeleteItemFromObject(pSub, pName);
    }
    
    val = cJSON_Print(root);
    
    if(root != NULL) {
        cJSON_Delete(root);
    }

    return val;
}

static char *readFile2Buf(const char *fileName) {
    int cfgSize;
    char *buf = NULL;

    FILE *fp = fopen(fileName, "rb");
    if(fp == NULL) {
        app_err("fopen %s failed", fileName);
        goto err;
    }
    fseek(fp, 0L, SEEK_END);
    cfgSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    buf = (char *)malloc(cfgSize/1024*1024 + 1024);
    if(buf == NULL) {
        app_err("malloc %d failed", cfgSize);
        goto err;
    }
    memset(buf, 0, cfgSize/1024*1024 + 1024);
    if(fread(buf, 1, cfgSize, fp)){
    }

err:
    if(fp != NULL) {
        fclose(fp);
    }

    return buf;
}

int getIntValFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3) {
    int val = -1;
    char *buf = readFile2Buf(fileName);
    if(buf != NULL) {
        val = getIntValFromJson(buf, nameSub1, nameSub2, nameSub3);
        free(buf);
    }
    else {
        app_warring("%s, readFile2Buf failed", fileName);
    }

    return val;
}

char *getStrValFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3) {
    char *val = NULL;
    char *buf = readFile2Buf(fileName);
    if(buf != NULL) {
        val = getStrValFromJson(buf, nameSub1, nameSub2, nameSub3);
        free(buf);
    }
    else {
        app_warring("%s, readFile2Buf failed", fileName);
    }

    return val;
}

double getDoubleValFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3) {
    double val = -1;
    char *buf = readFile2Buf(fileName);
    if(buf != NULL) {
        val = getDoubleValFromJson(buf, nameSub1, nameSub2, nameSub3);
        free(buf);
    }
    else {
        app_warring("%s, readFile2Buf failed", fileName);
    }

    return val;
}

char *getArrayBufFromFile(const char *fileName, const char *nameSub1, const char *nameSub2, const char *nameSub3, int &size){
    char *val = NULL;
    char *buf = readFile2Buf(fileName);
    if(buf != NULL) {
        val = getArrayBufFromJson(buf, nameSub1, nameSub2, nameSub3,size);
        free(buf);
    }
    else {
        app_warring("%s, readFile2Buf failed", fileName);
    }

    return val;
}

long long int getSysFreeMem(void) {
    struct sysinfo info;

    memset(&info, 0, sizeof(info));
    sysinfo(&info);

    return info.freeram;
}

int connectServer(char *ip, int port) {
    int fd = -1;
    struct sockaddr_in cliaddr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd<0) {
        app_err("create socket failed %s", strerror(errno));
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    int setret=0;
    setret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
    if(setret!=0) {
        app_err("set snd time out failed %s", strerror(errno));
        close(fd);
        return -1;
    }

    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setret=setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,(const char *)&tv, sizeof(tv));
    if(setret!=0) {
        app_err("set recv time out failed %s", strerror(errno));
        close(fd);
        return -1;
    }

    bzero(&cliaddr, sizeof(struct sockaddr_in));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = inet_addr(ip);
    cliaddr.sin_port = htons(port);
    if(connect(fd, (struct sockaddr *)&cliaddr, sizeof(struct sockaddr_in)) != 0) {
        close(fd);
        printf("connect server %s port %d failed, %d:%s\n", ip, port, errno, strerror(errno));
        if(errno == EINPROGRESS) {
        }
        if(errno == ECONNREFUSED) {
            return -2;
        }
        return -1;
    }

    return fd;
}

int blockSend(unsigned int connfd, char *src, int size) {
    int k, l_sendedsize;
    int l_connfd=connfd;
    int l_size=size;
    int ret;
    k = 0;
    l_sendedsize = 0;
    struct timeval timeout;
    fd_set rw;                      

    while(1) {
        FD_ZERO(&rw);
        FD_SET(connfd, &rw);
        timeout.tv_sec = 3;
        timeout.tv_usec =0;
        ret = select(connfd+1, NULL, &rw, NULL, &timeout);  
        if(ret == -1) {
            app_err("err: select -1");
            return -6;
        }
        else if(ret == 0) {
            if(errno == EPIPE || errno == ECONNRESET) {
                app_err("err : select timeout, fd:%d, %d:%s", connfd,  errno, strerror(errno));
                return -5;
            }
            else {
                if(errno != 0) {
                    app_err("select err, fd:%d, %d:%s", connfd, errno, strerror(errno));
                    return -4;
                }
                else {
                    //printf("select err, fd:%d, errno:%d, %s\n", connfd, errno, strerror(errno));
                    return 0;
                }
            }
        }
        else {
            if(FD_ISSET(connfd, &rw)) {
                k = send(l_connfd, src + l_sendedsize, l_size - l_sendedsize, MSG_NOSIGNAL);
                if(k<=0) {
                    if(errno == EPIPE || errno == ECONNRESET) {
                        printf("send err:%d:%s, fd:%d\n", errno, strerror(errno), connfd);
                        return -3;
                    }
                    else {
                        app_err("send err:%d:%s, fd:%d", errno, strerror(errno), connfd);
                        return -2;
                    }
                }
            } else {
                app_err("FD_ISSET err");
                return -1;
            }
        }

        l_sendedsize += k;
        if (l_sendedsize == l_size) {
            return 0;
        } 
    } 
}

int blockRecv(unsigned int connfd, char *dst, int size, int timeOutSec) {
    int k, l_recvedsize;
    int l_connfd=connfd;
    int l_size=size;
    int ret;
    struct timeval timeout;
    fd_set rw;                      

    k = 0;
    l_recvedsize = 0;
    while(1) {
        FD_ZERO(&rw);
        FD_SET(connfd, &rw);
        timeout.tv_sec = timeOutSec;
        timeout.tv_usec =0;
        ret = select(connfd+1, &rw, NULL, NULL, &timeout);  
        if(ret > 0) {
            if(FD_ISSET(connfd, &rw)) {
                k = recv(l_connfd, dst + l_recvedsize, l_size - l_recvedsize, MSG_NOSIGNAL);
                if(k == 0) {
                    app_err("recv err, peer shutdown, %d:%s", errno, strerror(errno));
                    return -1;
                }
                else if(k < 0) {
                    if(errno == EAGAIN || EINTR) {
                        printf("recv err, %d:%s\n", errno, strerror(errno));
                        return 2;
                    }
                    else {
                        app_err("recv err, %d:%s", errno, strerror(errno));
                        return -2;
                    }
                }
            } else {
                app_err("FD_ISSET err");
                return -3;
            }
        }
        else if(ret == 0) {
            if(errno != 0) {
                app_err("select err, fd:%d, errno:%d, %s", connfd, errno, strerror(errno));
                return -4;
            }
            else {
                //printf("select timeout, fd:%d, errno:%d, %s\n", connfd, errno, strerror(errno));
                return TIME_OUT;
            }
        }
        else {
            app_err("err!! select -1");
            return -5;
        }

        l_recvedsize += k;
        if (l_recvedsize == l_size) {
            return 0;
        } 
    } 
}

int putToQueue(queue_common *queue, node_common *new_node, int max) {
    if(queue->queLen < max) {
        node_common *new_p;
        new_p = (node_common *) malloc(sizeof(node_common));
        if (new_p == NULL) {
            app_err("malloc failed");
            return -1;
        }
        memcpy(new_p, new_node, sizeof(node_common));
        new_p->next = NULL;
        if (queue->head == NULL) {
            queue->head = new_p;
            queue->tail = new_p;
        } else {
            queue->tail->next = new_p;
            queue->tail = new_p;
        }
        queue->queLen ++;
    }
    else {
        //app_warring("queLen is too large, %s:%d", new_node->name, queue->queLen);
        printf("queLen is too large, %d\n", queue->queLen);
        return queue->queLen;
    }
    
    return 0;
}

int getFromQueue(queue_common *queue, node_common **new_p) {
    node_common *p = queue->head;
    if(p != NULL) {
        queue->head = p->next;
        if(queue->head == NULL) {
            queue->tail = NULL;
        }
        queue->queLen --;
        if(queue->queLen < 0) {
            app_warring("exception queLen : %d", queue->queLen);
            queue->queLen = 0;
            return -1;
        }
        *new_p = p;
    }
    
    return 0;
}

int delFromQueue(queue_common *queue, void *arg, node_common **new_p, int (*condition)(node_common *p, void *arg)) {
    int ret;
    node_common *prev = queue->head;
    node_common *p = queue->head;
    while(p != NULL) {
        ret = condition(p, arg);
        if(ret == 1) {
            if(p == queue->head && p == queue->tail) {
                queue->head = queue->tail = NULL;
            }
            else if(p == queue->head) {
                queue->head = p->next;
            }
            else if(p == queue->tail) {
                prev->next = NULL;
                queue->tail = prev;
            }
            else {
                prev->next = p->next;
            }
            if(new_p != NULL) {
                *new_p = p;
            }
            queue->queLen --;
            if(queue->queLen < 0) {
                app_warring("exception queLen : %d", queue->queLen);
                queue->queLen = 0;
            }
            break;
        }
        prev = p;
        p = p->next;
    }
    
    return 0;
}

int searchFromQueue(queue_common *queue, void *arg, node_common **new_p, int (*condition)(node_common *p, void *arg)) {
    int ret;
    node_common *p = queue->head;
    while(p != NULL) {
        ret = condition(p, arg);
        if(ret == 1) {
            *new_p = p;
            break;
        }
        p = p->next;
    }
    
    return 0;
}

int traverseQueue(queue_common *queue, void *arg, int (*callBack)(node_common *p, void *arg)) {
    node_common *p = queue->head;
    while(p != NULL) {
        if(callBack(p, arg) != 0) {
            break;
        }
        p = p->next;
    }
    
    return 0;
}

int freeQueue(queue_common *queue, int (*callBack)(void *arg)) {
    node_common *p1;
    node_common *p = queue->head;
    while(p != NULL) {
        p1 = p;
        p = p->next;
        if(callBack != NULL) {
            callBack(p1);
        }
        if(p1->arg != NULL) {
            free(p1->arg);
        }
        free(p1);
    }
    queue->head = queue->tail = NULL;
    queue->queLen = 0;

    return 0;
}

int conditionTrue(node_common *p, void *arg) {
    return 1;
}

int putToShmQueue(void *sp, queue_common *queue, node_common *new_node, int max) {
    if(queue->queLen < max) {
        node_common *new_p;
        new_p = (node_common *)shmMalloc((ncx_slab_pool_t *)sp, sizeof(node_common));
        if (new_p == NULL) {
            printf("err, %s:%d, malloc failed\n", __FILE__, __LINE__);
            return -1;
        }
        memcpy(new_p, new_node, sizeof(node_common));
        new_p->next = NULL;
        if (queue->head == NULL) {
            queue->head = new_p;
            queue->tail = new_p;
        } else {
            queue->tail->next = new_p;
            queue->tail = new_p;
        }
        queue->queLen ++;
    }
    else {
        static int cnt = 0;
        if (cnt++ % 200 == 0) {
            printf("shm, queLen is too large, %d\n", queue->queLen);
        }
    }

    return 0;
}

int freeShmQueue(void *sp, queue_common *queue, int (*callBack)(void *arg)) {
    node_common *p1;
    node_common *p = queue->head;

    while(p != NULL) {
        p1 = p;
        p = p->next;
        if(callBack != NULL) {
            callBack(p1);
        }
        if(p1->arg != NULL) {
            shmFree((ncx_slab_pool_t *)sp, p1->arg);
        }
        shmFree((ncx_slab_pool_t *)sp, p1);
    }
    queue->head = queue->tail = NULL;
    queue->queLen = 0;

    return 0;
}

