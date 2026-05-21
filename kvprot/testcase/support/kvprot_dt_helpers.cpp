#include "kvprot_dt_fixture.h"

static void kvprot_dt_reset_all(void)
{
    (void)memset(&g_dt, 0, sizeof(g_dt));
    g_dt.scat_register_ret = RETURN_OK;
    g_dt.create_partition_ret = RETURN_OK;
    g_dt.created_part_id = 0x5A5AU;
    g_dt.vc_id = 7U;
    g_dt.normal_part.part_id = 99U;
    g_dt.dplwt_get_ret = RETURN_OK;
    g_dt.dplwt_create_ret = RETURN_OK;
    g_dt.dplwt_run_inline = true;
    g_dt.pthread_create_ret = RETURN_OK;

    (void)memset(&g_kvprot_scat_ctx, 0, sizeof(g_kvprot_scat_ctx));
    (void)memset(&g_kvprot_cmd_timeout_ctx, 0, sizeof(g_kvprot_cmd_timeout_ctx));
    (void)memset(g_kvprot_cmd_entry_list, 0, sizeof(g_kvprot_cmd_entry_list));
    g_kvprot_cmd_part_id = 0;
    g_kvprot_cmd_entry_set[0].flags = KVPROT_CMD_F_NEED_RX_DATA;
    g_kvprot_cmd_entry_set[0].ops.validate = tgtKvDeleteValidate;
    g_kvprot_cmd_entry_set[0].ops.execute = tgtKvDeleteExecute;
}

static void kvprot_dt_driver_send_resp(scat_tgt_cmd_s *drv_cmd)
{
    KVPROT_CMD *cmd = nullptr;

    g_dt.send_resp_count++;
    g_dt.last_resp_cmd = drv_cmd;
    if (drv_cmd != nullptr) {
        cmd = (KVPROT_CMD *)drv_cmd->upper;
    }
    if (cmd != nullptr) {
        g_dt.last_resp_status = cmd->cqe.status;
    }
}

static void kvprot_dt_driver_xfer_ready(scat_tgt_cmd_s *drv_cmd)
{
    g_dt.xfer_ready_count++;
    g_dt.last_xfer_cmd = drv_cmd;
}

static void kvprot_dt_add_driver(bool with_send_resp, bool with_xfer_ready)
{
    scat_tgt_driver_s driver;

    (void)memset(&driver, 0, sizeof(driver));
    if (with_send_resp) {
        driver.send_resp = kvprot_dt_driver_send_resp;
    }
    if (with_xfer_ready) {
        driver.xfer_ready = kvprot_dt_driver_xfer_ready;
    }
    EXPECT_EQ(RETURN_OK, tgtAddKvTargetDriver(&driver));
}

static void kvprot_dt_fill_delete_sqe(KVPROT_CMD_SQE *sqe, uint16_t batch_num, uint32_t data_len)
{
    (void)memset(sqe, 0, sizeof(*sqe));
    sqe->opcode = KVPROT_CMD_OPCODE_DELETE;
    sqe->cmd_id = 0x1234U;
    sqe->nsid = 0xABCDEFU;
    sqe->batch_num = batch_num;
    sqe->data_len = data_len;
}

static scat_tgt_cmd_s *kvprot_dt_alloc_drv_cmd(uint16_t batch_num, uint32_t data_len)
{
    KVPROT_CMD_SQE sqe;
    scat_cmd_cqe_s cqe;
    scat_tgt_cmd_s *drv_cmd = nullptr;

    (void)memset(&cqe, 0, sizeof(cqe));
    drv_cmd = tgtGetKvTargetCmd(nullptr, &cqe);
    if (drv_cmd == nullptr) {
        return nullptr;
    }

    kvprot_dt_fill_delete_sqe(&sqe, batch_num, data_len);
    (void)memcpy(&drv_cmd->scat_cmd, &sqe, sizeof(sqe));
    return drv_cmd;
}

static KVPROT_CMD *kvprot_dt_upper_cmd(scat_tgt_cmd_s *drv_cmd)
{
    return (drv_cmd == nullptr) ? nullptr : (KVPROT_CMD *)drv_cmd->upper;
}

static void kvprot_dt_set_delete_cmd_flags(uint32_t flags)
{
    g_kvprot_cmd_entry_set[0].flags = flags;
}

static void kvprot_dt_set_delete_execute(int32_t (*execute)(KVPROT_CMD *cmd))
{
    g_kvprot_cmd_entry_set[0].ops.execute = execute;
}

static void kvprot_dt_timeout_wheel_add(KVPROT_CMD *cmd)
{
    tgtKvCmdTimeoutWheelAdd(cmd);
}

static void kvprot_dt_timeout_wheel_remove(KVPROT_CMD *cmd)
{
    tgtKvCmdTimeoutWheelRemove(cmd);
}

static bool kvprot_dt_delete_result_key_failed(const uint8_t *result_buf, uint32_t key_idx)
{
    return tgtKvDeleteResultKeyFailed(result_buf, key_idx);
}

static uint32_t kvprot_dt_delete_failed_key_count(const uint8_t *result_buf, uint16_t key_num)
{
    return tgtKvDeleteGetFailedKeyCnt(result_buf, key_num);
}
