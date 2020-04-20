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

#ifndef __AIOTC_TYPEDEF_H__
#define __AIOTC_TYPEDEF_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SUCCESS             0
#define TIME_OUT            1
#define ERROR               -1
#define URL_ERR             -2
#define FULL_LOAD           11

#if defined(AIOTC_XXXXX)
#else
typedef  unsigned int       DWORD;
typedef  unsigned short     WORD;
typedef  unsigned short     USHORT;
typedef  short              SHORT;
typedef  int                LONG;
typedef  unsigned char      BYTE;
typedef  unsigned int       UINT;
typedef  void*              LPVOID;
typedef  void*              HANDLE;
typedef  unsigned int*      LPDWORD; 
typedef  unsigned long long UINT64;
typedef  signed long long   INT64;
#endif

#endif


