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

static int32_t tgtKvDeleteBatchSubmit(KVPROT_CMD *cmd)
{
    void *result_buf = cmd->drv_cmd.private_cmd.kv_cmd.result_page;
    uint32_t result_len = cmd->drv_cmd.private_cmd.kv_cmd.result_len;

    return tgtKvDeleteBatchStub(cmd->sqe.nsid, cmd->data_buf, KVPROT_DELETE_KEY_SECTION_LEN,
        cmd->sqe.batch_num, result_buf, result_len);
}

int32_t tgtKvDeleteValidate(KVPROT_CMD *cmd)
{
    uint32_t expect_len;

    if (cmd->sqe.batch_num == 0) {
        KVPROT_ERROR("Delete cmd failed, batch num is 0 cmd_id:%u nsid:%u.",
            cmd->sqe.cmd_id, cmd->sqe.nsid);
        cmd->cqe.status = KVPROT_CMD_STATUS_INTERNAL_ERROR;
        return RETURN_ERROR;
    }

    if (cmd->sqe.data_len == 0) {
        KVPROT_ERROR("Delete cmd failed, data len is 0 cmd_id:%u nsid:%u.",
            cmd->sqe.cmd_id, cmd->sqe.nsid);
        cmd->cqe.status = KVPROT_CMD_STATUS_INTERNAL_ERROR;
        return RETURN_ERROR;
    }

    expect_len = (uint32_t)cmd->sqe.batch_num * KVPROT_DELETE_KEY_SECTION_LEN;
    if (cmd->sqe.data_len != expect_len) {
        KVPROT_ERROR("Delete cmd failed, data len:%u expect:%u cmd_id:%u nsid:%u.",
            cmd->sqe.data_len, expect_len, cmd->sqe.cmd_id, cmd->sqe.nsid);
        cmd->cqe.status = KVPROT_CMD_STATUS_INTERNAL_ERROR;
        return RETURN_ERROR;
    }

    return RETURN_OK;
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
    uint32_t status;
    int32_t ret;
    uint8_t *result_buf = NULL;

    if (cmd->data_buf == NULL ||
        cmd->drv_cmd.private_cmd.kv_cmd.result_page == NULL) {
        KVPROT_ERROR("Delete cmd failed, data buffer or result page is null.");
        cmd->cqe.status = KVPROT_CMD_STATUS_INTERNAL_ERROR;
        return RETURN_ERROR;
    }

    tgtKvCmdMarkBackendPending(cmd);
    ret = tgtKvDeleteBatchSubmit(cmd);
    if (ret != RETURN_OK) {
        KVPROT_ERROR("Delete key batch failed, ret:%d nsid:%u batch_num:%u.",
            ret, cmd->sqe.nsid, cmd->sqe.batch_num);
        tgtKvCmdBackendDone(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
        return RETURN_ERROR;
    }

    result_buf = (uint8_t *)cmd->drv_cmd.private_cmd.kv_cmd.result_page;
    failed_cnt = tgtKvDeleteGetFailedKeyCnt(result_buf, cmd->sqe.batch_num);
    status = (failed_cnt == 0) ? KVPROT_CMD_STATUS_OK : KVPROT_CMD_STATUS_INTERNAL_ERROR;
    tgtKvCmdBackendDone(cmd, status);

    return (failed_cnt == 0) ? RETURN_OK : RETURN_ERROR;
}
