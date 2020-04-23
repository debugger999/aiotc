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

#ifndef __AIOTC_LOG_H__
#define __AIOTC_LOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/time.h>

#define DEBUG_LOG_FILE                  "log/aiotc.log"
#define DEBUG_LOG_BAK_FILE              "log/aiotc.log.bak"
#define LOG_SIZE_MAX                    (1024*1024*20)

#define APREFIX_NONE   "\033[0m"
#define APREFIX_RED    "\033[0;31m"
#define APREFIX_GREEN  "\033[0;32m"
#define APREFIX_YELLOW "\033[1;33m"

#define app_debug(format, args...) \
    do { \
        struct timeval tv; \
        struct tm _time; \
        printf(APREFIX_GREEN"DEBUG : FILE -> %s, LINE -> %d, %s, "  format APREFIX_NONE"\n", \
                __FILE__, __LINE__, __func__, ## args); \
        gettimeofday(&tv, NULL); \
        localtime_r(&tv.tv_sec, &_time); \
        FILE *fp = fopen(DEBUG_LOG_FILE, "a+"); \
        if(fp != NULL) { \
            fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d.%03d, debug, %s:%d,%s, " format "\n", \
                    _time.tm_year + 1900, _time.tm_mon + 1, _time.tm_mday, _time.tm_hour, _time.tm_min, \
                    _time.tm_sec,(int)tv.tv_usec/1000, __FILE__, __LINE__, __func__, ##args); \
            fclose(fp); \
        } \
        struct stat f_stat; \
        if(stat(DEBUG_LOG_FILE, &f_stat) == 0) { \
            if(f_stat.st_size > LOG_SIZE_MAX) { \
				rename(DEBUG_LOG_FILE, DEBUG_LOG_BAK_FILE);\
            } \
        } \
    } while(0)

#define app_warring(format, args...) \
    do { \
        struct timeval tv; \
        struct tm _time; \
        printf(APREFIX_YELLOW"WARRING : FILE -> %s, LINE -> %d, %s, "  format APREFIX_NONE"\n", \
                __FILE__, __LINE__, __func__, ## args); \
        gettimeofday(&tv, NULL); \
        localtime_r(&tv.tv_sec, &_time); \
        FILE *fp = fopen(DEBUG_LOG_FILE, "a+"); \
        if(fp != NULL) { \
            fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d, warring, pid:%d %s:%d,%s, " format "\n", \
                    _time.tm_year + 1900, _time.tm_mon + 1, _time.tm_mday, _time.tm_hour, _time.tm_min, \
                    _time.tm_sec,getpid(), __FILE__, __LINE__, __func__, ##args); \
            fclose(fp); \
        } \
        struct stat f_stat; \
        if(stat(DEBUG_LOG_FILE, &f_stat) == 0) { \
            if(f_stat.st_size > LOG_SIZE_MAX) { \
				rename(DEBUG_LOG_FILE, DEBUG_LOG_BAK_FILE);\
            } \
        } \
    } while(0)

#define app_err(format, args...) \
    do { \
        struct timeval tv; \
        struct tm _time; \
        printf(APREFIX_RED"ERR : FILE -> %s, LINE -> %d, %s, "  format APREFIX_NONE"\n", \
                __FILE__, __LINE__, __func__, ## args); \
        gettimeofday(&tv, NULL); \
        localtime_r(&tv.tv_sec, &_time); \
        FILE *fp = fopen(DEBUG_LOG_FILE, "a+"); \
        if(fp != NULL) { \
            fprintf(fp, "%d-%02d-%02d %02d:%02d:%02d, error, pid:%d %s:%d,%s, " format "\n", \
                    _time.tm_year + 1900, _time.tm_mon + 1, _time.tm_mday, _time.tm_hour, _time.tm_min, \
                    _time.tm_sec, getpid(), __FILE__, __LINE__, __func__, ##args); \
            fclose(fp); \
        } \
        struct stat f_stat; \
        if(stat(DEBUG_LOG_FILE, &f_stat) == 0) { \
            if(f_stat.st_size > LOG_SIZE_MAX) { \
				rename(DEBUG_LOG_FILE, DEBUG_LOG_BAK_FILE);\
            } \
        } \
    } while(0)

#endif

