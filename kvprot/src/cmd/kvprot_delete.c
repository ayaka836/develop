#include "kvprot_cmd.h"
#include "kvprot_common.h"

static int32_t tgtKvDeleteBatchStub(uint32_t nsid, const void *key_buf, uint32_t key_len,
    uint16_t key_num, void *result_buf, uint32_t result_len)
{
    (void)nsid;
    (void)key_buf;
    (void)key_len;
    (void)key_num;
    (void)result_buf;
    (void)result_len;

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

static int32_t tgtKvDeleteBatchWithTimeout(KVPROT_CMD *cmd, bool *is_timeout)
{
    void *result_buf = cmd->drv_cmd.private_cmd.kv_cmd.result_page;
    uint32_t result_len = cmd->drv_cmd.private_cmd.kv_cmd.result_len;
    uint64_t start_ms = KVPROT_GET_TIME_MS();
    int32_t ret = tgtKvDeleteBatchStub(cmd->sqe.nsid, cmd->data_buf, KVPROT_DELETE_KEY_SECTION_LEN,
        cmd->sqe.batch_num, result_buf, result_len);
    uint64_t end_ms = KVPROT_GET_TIME_MS();

    *is_timeout = tgtKvIsTimeout(start_ms, end_ms, KVPROT_DELETE_BDM_TIMEOUT_MS);
    if (*is_timeout) {
        KVPROT_ERROR("Delete key batch timeout, nsid:%u batch_num:%u cost:%llu timeout:%u.",
            cmd->sqe.nsid, cmd->sqe.batch_num, end_ms - start_ms, KVPROT_DELETE_BDM_TIMEOUT_MS);
        return RETURN_ERROR;
    }

    return ret;
}

static bool tgtKvDeleteResultKeyFailed(const uint8_t *result_buf, uint32_t key_idx)
{
    uint8_t mask = (uint8_t)(1U << ((key_idx % KVPROT_RESULT_BUF_KEY_NUM_PER_BYTE) *
        KVPROT_RESULT_BUF_KEY_SECTION_LEN_BIT));

    return (result_buf[key_idx / KVPROT_RESULT_BUF_KEY_NUM_PER_BYTE] & mask) != 0;
}

static uint32_t tgtKvDeleteGetFailedKeyCnt(const uint8_t *result_buf, uint16_t key_num)
{
    uint32_t failed_cnt = 0;
    uint32_t key_idx;

    for (key_idx = 0; key_idx < key_num; key_idx++) {
        if (tgtKvDeleteResultKeyFailed(result_buf, key_idx)) {
            failed_cnt++;
            KVPROT_ERROR("Delete key failed, key_idx:%u.", key_idx);
        }
    }

    return failed_cnt;
}

int32_t tgtKvDeleteExecute(KVPROT_CMD *cmd)
{
    uint32_t failed_cnt;
    int32_t ret;
    bool is_timeout = false;
    uint8_t *result_buf = NULL;

    if (cmd->data_buf == NULL ||
        cmd->drv_cmd.private_cmd.kv_cmd.result_page == NULL) {
        KVPROT_ERROR("Delete cmd failed, data buffer or result page is null.");
        cmd->cqe.status = KVPROT_CMD_STATUS_INTERNAL_ERROR;
        return RETURN_ERROR;
    }

    ret = tgtKvDeleteBatchWithTimeout(cmd, &is_timeout);
    if (is_timeout) {
        cmd->cqe.status = KVPROT_CMD_STATUS_TIMEOUT;
        return RETURN_ERROR;
    }

    if (ret != RETURN_OK) {
        KVPROT_ERROR("Delete key batch failed, ret:%d nsid:%u batch_num:%u.",
            ret, cmd->sqe.nsid, cmd->sqe.batch_num);
        cmd->cqe.status = KVPROT_CMD_STATUS_INTERNAL_ERROR;
        return RETURN_ERROR;
    }

    result_buf = (uint8_t *)cmd->drv_cmd.private_cmd.kv_cmd.result_page;
    failed_cnt = tgtKvDeleteGetFailedKeyCnt(result_buf, cmd->sqe.batch_num);
    cmd->cqe.status = (failed_cnt == 0) ? KVPROT_CMD_STATUS_OK : KVPROT_CMD_STATUS_INTERNAL_ERROR;

    return (failed_cnt == 0) ? RETURN_OK : RETURN_ERROR;
}
