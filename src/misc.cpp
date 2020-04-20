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

static int createDir(const char *sPathName) {
    char dirName[4096];
    strcpy(dirName, sPathName);

    int len = strlen(dirName);
    if(dirName[len-1]!='/')
        strcat(dirName, "/");

    len = strlen(dirName);
    int i=0;
    for(i=1; i<len; i++)
    {
        if(dirName[i]=='/')
        {
            dirName[i] = 0;
            if(access(dirName, R_OK)!=0)
            {
                if(mkdir(dirName, 0755)==-1)
                {
                    //TODO:errno:ENOSPC(No space left on device)
                    app_err("path %s error %s", dirName, strerror(errno));
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

