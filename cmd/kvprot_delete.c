#include "kvprot_cmd.h"
#include "kvprot_common.h"

static int32_t tgtKvDeleteKeyStub(uint32_t nsid, const void *key, uint32_t key_len)
{
    (void)nsid;
    (void)key;
    (void)key_len;

    return RETURN_OK;
}

static bool tgtKvTimeIsValid(uint64_t time_ms)
{
    return time_ms != 0;
}

static bool tgtKvIsTimeout(uint64_t start_ms, uint64_t end_ms, uint64_t timeout_ms)
{
    if (!tgtKvTimeIsValid(start_ms) || !tgtKvTimeIsValid(end_ms) || timeout_ms == 0) {
        return false;
    }

    return (end_ms - start_ms) > timeout_ms;
}

static int32_t tgtKvDeleteKeyWithTimeout(KVPROT_CMD *cmd, const void *key, uint32_t key_len, bool *is_timeout)
{
    uint64_t start_ms = KVPROT_GET_TIME_MS();
    int32_t ret = tgtKvDeleteKeyStub(cmd->sqe.nsid, key, key_len);
    uint64_t end_ms = KVPROT_GET_TIME_MS();

    *is_timeout = tgtKvIsTimeout(start_ms, end_ms, KVPROT_DELETE_BDM_TIMEOUT_MS);
    if (*is_timeout) {
        KVPROT_ERROR("Delete key timeout, nsid:%u cost:%llu timeout:%u.",
            cmd->sqe.nsid, end_ms - start_ms, KVPROT_DELETE_BDM_TIMEOUT_MS);
        return RETURN_ERROR;
    }

    return ret;
}

int32_t tgtKvDeleteExecute(KVPROT_CMD *cmd)
{
    uint32_t delete_cnt = 0;
    uint32_t key_idx;
    int32_t ret;
    bool is_timeout = false;

    if (cmd->data_buf == NULL ||
        cmd->drv_cmd.private_cmd.kv_cmd.result_page == NULL) {
        KVPROT_ERROR("Delete cmd failed, data buffer or result page is null.");
        cmd->cqe.status = KVPROT_CMD_STATUS_INTERNAL_ERROR;
        return RETURN_ERROR;
    }

    for (key_idx = 0; key_idx < cmd->sqe.batch_num; key_idx++) {
        uint8_t *key_addr = (uint8_t *)cmd->data_buf + KVPROT_DELETE_KEY_SECTION_LEN * key_idx;

        ret = tgtKvDeleteKeyWithTimeout(cmd, key_addr, KVPROT_DELETE_KEY_SECTION_LEN, &is_timeout);
        if (is_timeout) {
            cmd->cqe.status = KVPROT_CMD_STATUS_TIMEOUT;
            return RETURN_ERROR;
        }

        if (ret == RETURN_OK) {
            delete_cnt++;
            continue;
        }

        ((uint8_t *)cmd->drv_cmd.private_cmd.kv_cmd.result_page)[key_idx / KVPROT_RESULT_BUF_KEY_NUM_PER_BYTE] |=
            (1 << ((key_idx % KVPROT_RESULT_BUF_KEY_NUM_PER_BYTE) * KVPROT_RESULT_BUF_KEY_SECTION_LEN_BIT));
        KVPROT_ERROR("Delete key failed, ret:%d nsid:%u key_idx:%u.", ret, cmd->sqe.nsid, key_idx);
    }

    cmd->cqe.status = (delete_cnt == cmd->sqe.batch_num) ?
        KVPROT_CMD_STATUS_OK : KVPROT_CMD_STATUS_INTERNAL_ERROR;

    return delete_cnt == cmd->sqe.batch_num ? RETURN_OK : RETURN_ERROR;
}
