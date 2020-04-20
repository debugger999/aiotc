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

#ifndef __AIOTC_MISC_H__
#define __AIOTC_MISC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef int (*CommonObjFunc)(void *buf, void *arg);
typedef int (*COMMONCALLBACK)(void *argA, void *argB, void *argC);

typedef struct {
    void *arga;
    void *argb;
    void *argc;
    void *argd;
    void *arge;
    COMMONCALLBACK func;
} CommonParams;

int dirCheck(const char *dir);

#endif
