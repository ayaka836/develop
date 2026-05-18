/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2026. All rights reserved.
 * Description: common macros for kv protocol
 */

#ifndef KVPROT_COMMON_H
#define KVPROT_COMMON_H

#include "lvos.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RETURN_OK
#define RETURN_OK 0
#endif

#ifndef RETURN_ERROR
#define RETURN_ERROR (-1)
#endif

#ifndef ARRAY_LEN
#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef KVPROT_INFO
#define KVPROT_INFO(fmt, ...)  DBG_LogInfo(DBG_LOGID_BUTT, "[KVPROT] " fmt, ##__VA_ARGS__)
#endif

#ifndef KVPROT_ERROR
#define KVPROT_ERROR(fmt, ...) DBG_LogError(DBG_LOGID_BUTT, "[KVPROT] " fmt, ##__VA_ARGS__)
#endif

#ifndef KVPROT_GET_TIME_MS
#define KVPROT_GET_TIME_MS() 0ULL
#endif

typedef struct tagKVPROT_INIT_OPS {
    const char *desc;
    int32_t (*init)(void);
    void (*exit)(void);
} KVPROT_INIT_OPS;

int32_t KvprotInitOps(const KVPROT_INIT_OPS *ops, uint32_t count, const char *module_name);
void KvprotExitOps(const KVPROT_INIT_OPS *ops, uint32_t count, const char *module_name);

#ifdef __cplusplus
}
#endif

#endif
