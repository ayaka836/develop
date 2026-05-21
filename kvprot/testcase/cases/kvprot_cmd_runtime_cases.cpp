#include "kvprot_dt_fixture.h"

static int32_t kvprot_dt_execute_wait_backend(KVPROT_CMD *cmd)
{
    tgtKvCmdMarkBackendPending(cmd);
    return RETURN_OK;
}

static int32_t kvprot_dt_execute_fail_without_status(KVPROT_CMD *cmd)
{
    (void)cmd;
    return RETURN_ERROR;
}

TEST_F(KvprotDtTest, RxDataNullAndSchedulePaths)
{
    scat_tgt_cmd_s *drv_cmd = nullptr;
    KVPROT_CMD *cmd = nullptr;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());
    kvprot_dt_add_driver(true, true);

    tgtTargetRxKvData(nullptr);

    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    drv_cmd->upper = nullptr;
    tgtTargetRxKvData(drv_cmd);
    kvprot_dt_free_structure(cmd, 0);

    kvprot_dt_set_delete_cmd_flags(KVPROT_CMD_F_NEED_SCHEDULE);
    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(1U, g_dt.dplwt_get_count);
    EXPECT_EQ(1U, g_dt.dplwt_create_count);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);

    kvprot_dt_set_delete_cmd_flags(KVPROT_CMD_F_NEED_RX_DATA | KVPROT_CMD_F_NEED_SCHEDULE);
    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    tgtTargetRxKvData(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_OK, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);
}

TEST_F(KvprotDtTest, ScheduleFailurePaths)
{
    scat_tgt_cmd_s *drv_cmd = nullptr;
    KVPROT_CMD *cmd = nullptr;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());
    kvprot_dt_add_driver(true, true);
    kvprot_dt_set_delete_cmd_flags(KVPROT_CMD_F_NEED_SCHEDULE);

    g_dt.dplwt_get_ret = RETURN_ERROR;
    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);

    g_dt.dplwt_get_ret = RETURN_OK;
    g_dt.dplwt_create_ret = RETURN_ERROR;
    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);
}

TEST_F(KvprotDtTest, ExecuteParsedCmdWaitBackendAndFailureCompletion)
{
    scat_tgt_cmd_s *drv_cmd = nullptr;
    KVPROT_CMD *cmd = nullptr;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());
    kvprot_dt_add_driver(true, true);

    kvprot_dt_set_delete_cmd_flags(0);
    kvprot_dt_set_delete_execute(kvprot_dt_execute_wait_backend);
    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(0U, g_dt.send_resp_count);
    EXPECT_EQ(KVPROT_CMD_STATE_WAIT_BACKEND, cmd->state);
    EXPECT_TRUE(cmd->in_timeout_list);

    tgtKvCmdBackendDone(cmd, KVPROT_CMD_STATUS_OK);
    EXPECT_EQ(1U, g_dt.send_resp_count);
    tgtKvCmdBackendDone(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(1U, g_dt.send_resp_count);
    tgtTargetTgtCmdDone(drv_cmd);

    kvprot_dt_set_delete_cmd_flags(0);
    kvprot_dt_set_delete_execute(kvprot_dt_execute_fail_without_status);
    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);
}

TEST_F(KvprotDtTest, CompleteBackendDoneAndSensePaths)
{
    scat_tgt_cmd_s *drv_cmd = nullptr;
    KVPROT_CMD *cmd = nullptr;
    KVPROT_CMD stack_cmd;
    scat_cmd_cqe_s cqe;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());
    kvprot_dt_add_driver(true, true);

    tgtKvCmdComplete(nullptr, KVPROT_CMD_STATUS_OK);
    tgtKvCmdBackendDone(nullptr, KVPROT_CMD_STATUS_OK);
    tgtKvCmdMarkBackendPending(nullptr);

    (void)memset(&stack_cmd, 0, sizeof(stack_cmd));
    stack_cmd.drv_cmd.upper = &stack_cmd;
    spin_lock_init(&stack_cmd.lock);
    tgtKvCmdComplete(&stack_cmd, KVPROT_CMD_STATUS_OK);
    EXPECT_EQ(1U, g_dt.send_resp_count);
    EXPECT_EQ(KVPROT_CMD_STATE_COMPLETE, stack_cmd.state);
    tgtKvCmdComplete(&stack_cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(1U, g_dt.send_resp_count);

    drv_cmd = tgtGetKvTargetCmd(nullptr, nullptr);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    cmd->backend_done = false;
    cmd->scat_done = true;
    tgtKvCmdBackendDone(cmd, KVPROT_CMD_STATUS_OK);
    EXPECT_EQ((uint32_t)KVPROT_CMD_STATUS_OK, g_dt.last_resp_status);
    EXPECT_EQ(1U, g_dt.free_structure_count);

    (void)memset(&cqe, 0, sizeof(cqe));
    tgtGetKvSense(DRV_SCAT_NO_ERROR, &cqe, nullptr);
    EXPECT_EQ(KVPROT_CMD_STATUS_OK, cqe.status);
    tgtGetKvSense((scat_error_code_e)(DRV_SCAT_NO_ERROR + 1), &cqe, nullptr);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cqe.status);
    tgtGetKvSense(DRV_SCAT_NO_ERROR, nullptr, nullptr);
}

TEST_F(KvprotDtTest, TimeoutScanTimesOutAndUnlinksSkippedCommands)
{
    KVPROT_CMD timeout_cmd;
    KVPROT_CMD skipped_cmd;
    uint32_t i;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());
    kvprot_dt_add_driver(true, true);

    (void)memset(&timeout_cmd, 0, sizeof(timeout_cmd));
    timeout_cmd.drv_cmd.upper = &timeout_cmd;
    timeout_cmd.sqe.opcode = KVPROT_CMD_OPCODE_DELETE;
    timeout_cmd.sqe.cmd_id = 0x66U;
    timeout_cmd.sqe.nsid = 0x77U;
    spin_lock_init(&timeout_cmd.lock);
    tgtKvCmdMarkBackendPending(&timeout_cmd);
    EXPECT_TRUE(timeout_cmd.in_timeout_list);

    for (i = 0; i < KVPROT_DELETE_BDM_TIMEOUT_MS / KVPROT_CMD_TIMEOUT_SCAN_INTERVAL_MS - 1U; i++) {
        tgtKvCmdTimeoutScan();
    }
    EXPECT_EQ(0U, g_dt.send_resp_count);
    tgtKvCmdTimeoutScan();
    EXPECT_EQ(1U, g_dt.send_resp_count);
    EXPECT_EQ(KVPROT_CMD_STATUS_TIMEOUT, timeout_cmd.cqe.status);
    EXPECT_EQ(KVPROT_CMD_STATE_TIMEOUT, timeout_cmd.state);
    EXPECT_FALSE(timeout_cmd.in_timeout_list);

    (void)memset(&skipped_cmd, 0, sizeof(skipped_cmd));
    skipped_cmd.backend_done = true;
    skipped_cmd.state = KVPROT_CMD_STATE_WAIT_BACKEND;
    spin_lock_init(&skipped_cmd.lock);
    kvprot_dt_timeout_wheel_add(&skipped_cmd);
    EXPECT_TRUE(skipped_cmd.in_timeout_list);
    for (i = 0; i < KVPROT_DELETE_BDM_TIMEOUT_MS / KVPROT_CMD_TIMEOUT_SCAN_INTERVAL_MS; i++) {
        tgtKvCmdTimeoutScan();
    }
    EXPECT_FALSE(skipped_cmd.in_timeout_list);
    EXPECT_EQ(1U, g_dt.send_resp_count);
}

TEST_F(KvprotDtTest, SendResponseWithoutDriverAndRemoveNulls)
{
    KVPROT_CMD cmd;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.drv_cmd.upper = &cmd;
    spin_lock_init(&cmd.lock);
    tgtKvCmdComplete(&cmd, KVPROT_CMD_STATUS_TIMEOUT);
    EXPECT_EQ(KVPROT_CMD_STATE_TIMEOUT, cmd.state);
    EXPECT_EQ(0U, g_dt.send_resp_count);

    tgtTargetTgtCmdDone(nullptr);
    kvprot_dt_timeout_wheel_add(nullptr);
    kvprot_dt_timeout_wheel_remove(nullptr);
}
