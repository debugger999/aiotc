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
#include "share.h"
#include "db.h"

int configInit(configParams *pConfigParams) {
    const char *config = AIOTC_CFG;

    pConfigParams->masterEnable = getIntValFromFile(config, "master", "enable", NULL);
    pConfigParams->masterRestPort = getIntValFromFile(config, "master", "restPort", NULL);
    pConfigParams->masterStreaminPort = getIntValFromFile(config, "master", "streaminPort", NULL);
    pConfigParams->masterStreaminProcNum = getIntValFromFile(config, "master", "streaminProcNum", NULL);
    pConfigParams->slaveRestPort = getIntValFromFile(config, "slave", "restPort", NULL);
    pConfigParams->slaveStreaminPort = getIntValFromFile(config, "slave", "streaminPort", NULL);
    pConfigParams->slaveStreaminProcNum = getIntValFromFile(config, "slave", "streaminProcNum", NULL);
    pConfigParams->shmKey = getIntValFromFile(config, "shm", "key", NULL);
    pConfigParams->shmHeadSize = getIntValFromFile(config, "shm", "headSize", NULL);
    pConfigParams->videoMax = getIntValFromFile(config, "video", "max", NULL);
    pConfigParams->videoFrameSizeMax = getIntValFromFile(config, "video", "frameSizeMax", NULL);
    pConfigParams->videoDefaultPixW = getIntValFromFile(config, "video", "defaultPixW", NULL);
    pConfigParams->videoDefaultPixH = getIntValFromFile(config, "video", "defaultPixH", NULL);
    pConfigParams->videoQueLen = getIntValFromFile(config, "video", "queueLen", NULL);
    pConfigParams->captureMax = getIntValFromFile(config, "capture", "max", NULL);
    pConfigParams->captureFrameSizeMax = getIntValFromFile(config, "capture", "frameSizeMax", NULL);
    pConfigParams->captureQueLen = getIntValFromFile(config, "capture", "queueLen", NULL);
    pConfigParams->captureSaveDays = getIntValFromFile(config, "capture", "picSaveDays", NULL);

    return 0;
}

int dbInit(aiotcParams *pAiotcParams) {
    getLocalIp(pAiotcParams->localIp);
    return 0;
}

