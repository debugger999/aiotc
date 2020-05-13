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
    char *str;
    const char *config = AIOTC_CFG;

    pConfigParams->masterEnable = getIntValFromFile(config, "master", "enable", NULL);
    pConfigParams->masterRestPort = getIntValFromFile(config, "master", "restPort", NULL);
    pConfigParams->masterStreaminPort = getIntValFromFile(config, "master", "streaminPort", NULL);
    pConfigParams->masterStreaminProcNum = getIntValFromFile(config, "master", "streaminProcNum", NULL);
    pConfigParams->masterObjMax = getIntValFromFile(config, "master", "objMax", NULL);
    pConfigParams->slaveRestPort = getIntValFromFile(config, "slave", "restPort", NULL);
    pConfigParams->slaveStreaminPort = getIntValFromFile(config, "slave", "streaminPort", NULL);
    pConfigParams->slaveStreaminProcNum = getIntValFromFile(config, "slave", "streaminProcNum", NULL);
    pConfigParams->slaveObjMax = getIntValFromFile(config, "slave", "objMax", NULL);
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

    str = getStrValFromFile(config, "db", "type", NULL);
    if(str != NULL) {
        if(!strcmp(str, "mongodb")) {
            strncpy(pConfigParams->dbType, str, sizeof(pConfigParams->dbType));
        }
        else {
            app_warring("unsupport db type : %s", str);
        }
        free(str);
    }
    str = getStrValFromFile(config, "db", pConfigParams->dbType, "host");
    if(str != NULL) {
        strncpy(pConfigParams->dbHost, str, sizeof(pConfigParams->dbHost));
        free(str);
    }
    str = getStrValFromFile(config, "db", pConfigParams->dbType, "user");
    if(str != NULL) {
        strncpy(pConfigParams->dbUser, str, sizeof(pConfigParams->dbUser));
        free(str);
    }
    str = getStrValFromFile(config, "db", pConfigParams->dbType, "password");
    if(str != NULL) {
        strncpy(pConfigParams->dbPassword, str, sizeof(pConfigParams->dbPassword));
        free(str);
    }
    str = getStrValFromFile(config, "db", pConfigParams->dbType, "dbName");
    if(str != NULL) {
        strncpy(pConfigParams->dbName, str, sizeof(pConfigParams->dbName));
        free(str);
    }
    pConfigParams->dbPort = getIntValFromFile(config, "db", pConfigParams->dbType, "port");

    return 0;
}

int dbInit(aiotcParams *pAiotcParams) {
    configParams *pConfigParams = (configParams *)pAiotcParams->configArgs;
    dbParams *pDBParams = (dbParams *)pAiotcParams->dbArgs;

    strncat(pDBParams->uri_string, "mongodb://", sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string));
    if(strlen(pConfigParams->dbUser) > 0 && strlen(pConfigParams->dbPassword) > 0 &&
            strncmp(pConfigParams->dbUser, "null", sizeof(pConfigParams->dbUser))!= 0 &&
            strncmp(pConfigParams->dbPassword, "null", sizeof(pConfigParams->dbPassword)) != 0) {
        strncat(pDBParams->uri_string, pConfigParams->dbUser, sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string));
        strncat(pDBParams->uri_string, ":", sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string));
        strncat(pDBParams->uri_string, pConfigParams->dbPassword, sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string));
        strncat(pDBParams->uri_string, "@", sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string));
    }
    strncat(pDBParams->uri_string, pConfigParams->dbHost, sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string));
    strncat(pDBParams->uri_string, ":", sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string));
    snprintf(pDBParams->uri_string + strlen(pDBParams->uri_string), 
            sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string), "%d", pConfigParams->dbPort);
    if(strlen(pConfigParams->dbName) > 0 && strncmp(pConfigParams->dbName, "null", sizeof(pConfigParams->dbName)) != 0) {
        strncat(pDBParams->uri_string, "/", sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string));
        strncat(pDBParams->uri_string, pConfigParams->dbName, sizeof(pDBParams->uri_string)-strlen(pDBParams->uri_string));
    }

    getLocalIp(pAiotcParams->localIp);

    return 0;
}

int dbOpen(void *dbArgs) {
    bson_error_t error;
    dbParams *pDBParams = (dbParams *)dbArgs;
    char *uri_string = pDBParams->uri_string;

    mongoc_init();
    strncpy(pDBParams->type, "mongodb", sizeof(pDBParams->type));

    pDBParams->uri = mongoc_uri_new_with_error(uri_string, &error);
    if(pDBParams->uri == NULL) {
        app_err("parase %s err", uri_string);
        goto err;
    }
    pDBParams->client = mongoc_client_new_from_uri(pDBParams->uri);
    if(pDBParams->client == NULL) {
        app_err("new client failed, %s", uri_string);
        goto err;
    }
    mongoc_client_set_error_api(pDBParams->client, 2);
    mongoc_client_set_appname(pDBParams->client, "connect-example");
    pDBParams->database = mongoc_client_get_database(pDBParams->client, DB_NAME);
    if(pDBParams->database == NULL) {
        app_err("client get database failed, %s", uri_string);
        goto err;
    }
    app_debug("open %s success", uri_string);

    return 0;

err:
    dbClose(dbArgs);
    return -1;
}

static int checkDel(mongoc_collection_t *collection, 
        const char *selectName, const void *selIntVal, const void *selStrVal) {
    bool r;
    bson_error_t error;
    bson_t *selector = NULL;

    if(selIntVal != NULL) {
        selector = BCON_NEW(selectName, "{", "$eq", BCON_INT32(*(int *)selIntVal), "}");
    }
    else if(selStrVal != NULL) {
        selector = BCON_NEW(selectName, "{", "$eq", BCON_UTF8((char *)selStrVal), "}");
    }
    else {
        goto end;
    }
    r = mongoc_collection_delete_one(collection, selector, NULL, NULL, &error);
    if(!r) {
       printf("delet failed, %s, %s\n", selectName, error.message);
       goto end;
    }

end:
    if(selector != NULL) {
        bson_destroy (selector);
    }

    return 0;
}

int dbWrite(void *dbArgs, const char *table, const char *name, char *json, 
        const char *selectName, const void *selIntVal, const void *selStrVal) {
    bson_error_t error;
    char *buf = NULL;
    bson_t *insert = NULL;
    mongoc_collection_t *collection = NULL;
    dbParams *pDBParams = (dbParams *)dbArgs;

    collection = mongoc_client_get_collection(pDBParams->client, DB_NAME, table);
    if(collection == NULL) {
        app_err("get collection failed, %s", table);
        goto end;
    }
    if(selectName != NULL) {
        checkDel(collection, selectName, selIntVal, selStrVal);
    }

    if(name != NULL) {
        buf = (char *)malloc(((strlen(json)+strlen(name)+1)/1024*1024) + 1024);
        sprintf(buf, "{\"%s\":%s}", name, json);
        json = buf;
    }

    insert = bson_new_from_json((const uint8_t *)json, -1, &error);
    if(insert == NULL) {
        app_err("bson from json failed, %s, %s\n", json, error.message);
        goto end;
    }
    if(!mongoc_collection_insert_one(collection, insert, NULL, NULL, &error)) {
        app_err("insert failed, %s, %s\n", json, error.message);
        goto end;
    }

end:
    if(buf != NULL) {
        free(buf);
    }
    if(insert != NULL) {
        bson_destroy(insert);
    }
    if(collection != NULL) {
        mongoc_collection_destroy(collection);
    }
    return 0;
}

int dbTraverse(void *dbArgs, const char *table, void *arg, int (*callBack)(char *buf, void *arg)) {
    char *str;
    bson_t query;
    const bson_t *doc;
    mongoc_cursor_t *cursor = NULL;
    mongoc_collection_t *collection = NULL;
    dbParams *pDBParams = (dbParams *)dbArgs;

    bson_init(&query);
    collection = mongoc_client_get_collection(pDBParams->client, DB_NAME, table);
    if(collection == NULL) {
        app_err("get collection failed, %s", table);
        goto end;
    }

    cursor = mongoc_collection_find_with_opts(collection, &query, NULL, NULL);
    while(mongoc_cursor_next(cursor, &doc)) {
        str = bson_as_json (doc, NULL);
        //printf ("%s\n", str);
        if(callBack != NULL) {
            callBack(str, arg);
        }
        bson_free (str);
    }

end:
    if(cursor != NULL) {
        mongoc_cursor_destroy(cursor);
    }
    if(collection != NULL) {
        mongoc_collection_destroy(collection);
    }
    bson_destroy(&query);

    return 0;
}

int dbExsit(void *dbArgs, const char *table, 
        const char *selectName, const void *selIntVal, const void *selStrVal) {
    char *str;
    int exsit = 0;
    const bson_t *doc;
    bson_t *selector = NULL;
    mongoc_cursor_t *cursor = NULL;
    mongoc_collection_t *collection = NULL;
    dbParams *pDBParams = (dbParams *)dbArgs;

    collection = mongoc_client_get_collection(pDBParams->client, DB_NAME, table);
    if(collection == NULL) {
        app_err("get collection failed, %s", table);
        goto end;
    }

    if(selIntVal != NULL) {
        selector = BCON_NEW(selectName, "{", "$eq", BCON_INT32(*(int *)selIntVal), "}");
    }
    else if(selStrVal != NULL) {
        selector = BCON_NEW(selectName, "{", "$eq", BCON_UTF8((char *)selStrVal), "}");
    }
    else {
        selector = bson_new();
        bson_init(selector);
    }
    cursor = mongoc_collection_find_with_opts(collection, selector, NULL, NULL);
    if(mongoc_cursor_next(cursor, &doc)) {
        str = bson_as_json (doc, NULL);
        //printf ("%s\n", str);
        bson_free (str);
        exsit = 1;
    }

end:
    if(selector != NULL) {
        bson_destroy(selector);
    }
    if(cursor != NULL) {
        mongoc_cursor_destroy(cursor);
    }
    if(collection != NULL) {
        mongoc_collection_destroy(collection);
    }

    return exsit;
}

int dbUpdate(void *dbArgs, const char *table, bool upsert, const char *cmd,
        const char *selectName, const void *selIntVal, const void *selStrVal, 
        const char *updateName, const void *updIntVal, const void *updStrVal) {
    bson_error_t error;
    mongoc_collection_t *collection = NULL;
    bson_t *update = NULL, *opts = NULL, *selector = NULL;
    dbParams *pDBParams = (dbParams *)dbArgs;

    collection = mongoc_client_get_collection(pDBParams->client, DB_NAME, table);
    if(collection == NULL) {
        app_err("get collection failed, %s", table);
        goto end;
    }

    opts = BCON_NEW ("upsert", BCON_BOOL(upsert));
    if(selIntVal != NULL) {
        selector = BCON_NEW(selectName, "{", "$eq", BCON_INT32(*(int *)selIntVal), "}");
    }
    else if(selStrVal != NULL) {
        selector = BCON_NEW(selectName, "{", "$eq", BCON_UTF8((char *)selStrVal), "}");
    }
    else {
        selector = bson_new();
        bson_init(selector);
    }

    if(updIntVal != NULL) {
        update = BCON_NEW(cmd, "{", updateName, BCON_INT32(*(int *)updIntVal), "}");
    }
    else if(updStrVal != NULL) {
        update = BCON_NEW(cmd, "{", updateName, BCON_UTF8((char *)updStrVal), "}");
    }
    else {
        app_warring("update val is null");
        goto end;
    }

    if(!mongoc_collection_update_one(collection, selector, update, opts, NULL, &error)) {
       app_err("update failed, %s, %s, %s\n", table, updateName, error.message);
       goto end;
    }

end:
    if(opts != NULL) {
        bson_destroy(opts);
    }
    if(selector != NULL) {
        bson_destroy(selector);
    }
    if(update != NULL) {
        bson_destroy(update);
    }
    if(collection != NULL) {
        mongoc_collection_destroy(collection);
    }
    return 0;
}

int dbUpdateIntById(char *buf, void *dbArgs, const char *name, int val) {
    int id = getIntValFromJson(buf, "id", NULL, NULL);
    if(id < 0) {
        return -1;
    }
    dbUpdate(dbArgs, "obj", 0, "$set", "original.id", &id, NULL, name, &val, NULL);

    return 0;
}

int dbDel(void *dbArgs, const char *table, const char *selectName, const void *selIntVal, const void *selStrVal) {
    mongoc_collection_t *collection = NULL;
    dbParams *pDBParams = (dbParams *)dbArgs;

    collection = mongoc_client_get_collection(pDBParams->client, DB_NAME, table);
    if(collection == NULL) {
        app_err("get collection failed, %s", table);
        goto end;
    }
    if(selectName != NULL) {
        checkDel(collection, selectName, selIntVal, selStrVal);
    }

end:
    if(collection != NULL) {
        mongoc_collection_destroy(collection);
    }
    return 0;
}

int dbClose(void *dbArgs) {
    dbParams *pDBParams = (dbParams *)dbArgs;

    if(pDBParams->database != NULL) {
        mongoc_database_destroy(pDBParams->database);
        pDBParams->database = NULL;
    }
    if(pDBParams->uri != NULL) {
        mongoc_uri_destroy(pDBParams->uri);
        pDBParams->uri = NULL;
    }
    if(pDBParams->client != NULL) {
        mongoc_client_destroy(pDBParams->client);
        pDBParams->client = NULL;
    }
    mongoc_cleanup();

    return 0;
}

