/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2026. All rights reserved.
 * Description: common helper functions for kv protocol
 */

#include "kvprot_common.h"

int32_t KvprotInitOps(const KVPROT_INIT_OPS *ops, uint32_t count, const char *module_name)
{
    int32_t ret = RETURN_OK;
    uint32_t i = 0;

    if (ops == NULL) {
        KVPROT_ERROR("Init %s failed, ops is null.", module_name);
        return RETURN_ERROR;
    }

    for (; i < count; i++) {
        if (ops[i].init == NULL) {
            continue;
        }

        ret = ops[i].init();
        if (ret != RETURN_OK) {
            KVPROT_ERROR("Init %s failed:%d.", ops[i].desc, ret);
            break;
        }
    }

    if (ret == RETURN_OK) {
        KVPROT_INFO("Init %s success.", module_name);
        return RETURN_OK;
    }

    while (i > 0) {
        i--;
        if (ops[i].exit != NULL) {
            ops[i].exit();
        }
    }

    return ret;
}

void KvprotExitOps(const KVPROT_INIT_OPS *ops, uint32_t count, const char *module_name)
{
    uint32_t i = count;

    if (ops == NULL) {
        KVPROT_ERROR("Exit %s failed, ops is null.", module_name);
        return;
    }

    while (i > 0) {
        i--;
        if (ops[i].exit != NULL) {
            ops[i].exit();
        }
    }

    KVPROT_INFO("Exit %s success.", module_name);
}
