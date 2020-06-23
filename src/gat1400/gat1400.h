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

#ifndef __AIOTC_GAT1400_H__
#define __AIOTC_GAT1400_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GAT1400_PLATFROM_MAX    10

typedef struct {
    char platformId[128];
    char platformIp[64];
    int  platformPort;
    char platformUsername[64];
    char platformPassword[64];
    char platformName[64];
} gat1400Platform;

typedef struct {
    void    *handle;
    int     threadNum;
    char    localGatServerId[128];
    char    localGatHostIp[128];
    int     localGatPort;
    int     authEnable;
    char    userName[128];
    char    password[128];
    gat1400Platform platform[GAT1400_PLATFROM_MAX];
} gat1400Params;

int gat1400Process(void *arg);

#endif
