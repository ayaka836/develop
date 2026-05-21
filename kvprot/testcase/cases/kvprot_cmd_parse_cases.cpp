#include "kvprot_dt_fixture.h"

TEST_F(KvprotDtTest, GetTargetCmdSuccessAllocFailureAndDoneFree)
{
    scat_cmd_cqe_s cqe;
    scat_tgt_cmd_s *drv_cmd = nullptr;
    KVPROT_CMD *cmd = nullptr;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());

    (void)memset(&cqe, 0, sizeof(cqe));
    drv_cmd = tgtGetKvTargetCmd(nullptr, &cqe);
    ASSERT_NE(nullptr, drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_OK, cqe.status);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    ASSERT_NE(nullptr, cmd);
    EXPECT_EQ(cmd, drv_cmd->upper);
    EXPECT_TRUE(cmd->backend_done);
    EXPECT_FALSE(cmd->scat_done);

    tgtTargetTgtCmdDone(drv_cmd);
    EXPECT_EQ(1U, g_dt.free_structure_count);

    g_dt.fail_structure_alloc_call = g_dt.allocate_structure_count + 1U;
    (void)memset(&cqe, 0, sizeof(cqe));
    drv_cmd = tgtGetKvTargetCmd(nullptr, &cqe);
    EXPECT_EQ(nullptr, drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_BUSY, cqe.status);
}

TEST_F(KvprotDtTest, ParseRejectsBadDrvUnsupportedAndInvalidDelete)
{
    scat_tgt_cmd_s *drv_cmd = nullptr;
    KVPROT_CMD *cmd = nullptr;
    KVPROT_CMD_SQE sqe;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());
    kvprot_dt_add_driver(true, true);

    tgtParseKvTargetCmd(nullptr);

    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    drv_cmd->upper = nullptr;
    tgtParseKvTargetCmd(drv_cmd);
    kvprot_dt_free_structure(cmd, 0);

    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    (void)memset(&sqe, 0, sizeof(sqe));
    sqe.opcode = 0xFEU;
    sqe.cmd_id = 1U;
    sqe.nsid = 2U;
    (void)memcpy(&drv_cmd->scat_cmd, &sqe, sizeof(sqe));
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(1U, g_dt.send_resp_count);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);

    drv_cmd = kvprot_dt_alloc_drv_cmd(0, 0);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);

    drv_cmd = kvprot_dt_alloc_drv_cmd(2, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);
}

TEST_F(KvprotDtTest, ParseAllocationAndXferErrorPaths)
{
    scat_tgt_cmd_s *drv_cmd = nullptr;
    KVPROT_CMD *cmd = nullptr;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());
    kvprot_dt_add_driver(true, true);

    g_dt.fail_page_alloc_call = 1U;
    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_ALLOC_FAIL, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);

    g_dt.fail_page_alloc_call = g_dt.allocate_page_count + 2U;
    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_ALLOC_FAIL, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);

    EXPECT_EQ(RETURN_OK, tgtRemoveKvTargetDriver(nullptr));
    drv_cmd = kvprot_dt_alloc_drv_cmd(1, KVPROT_DELETE_KEY_SECTION_LEN);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);
}

TEST_F(KvprotDtTest, ParseSuccessRxDeleteOkAndFailedKeys)
{
    scat_tgt_cmd_s *drv_cmd = nullptr;
    KVPROT_CMD *cmd = nullptr;
    uint8_t *result_buf = nullptr;

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());
    kvprot_dt_add_driver(true, true);

    drv_cmd = kvprot_dt_alloc_drv_cmd(2, KVPROT_DELETE_KEY_SECTION_LEN * 2U);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    EXPECT_EQ(1U, g_dt.xfer_ready_count);
    EXPECT_EQ(drv_cmd, g_dt.last_xfer_cmd);
    EXPECT_EQ(0U, g_dt.send_resp_count);
    ASSERT_NE(nullptr, cmd->data_buf);
    ASSERT_NE(nullptr, cmd->drv_cmd.private_cmd.kv_cmd.result_page);

    tgtTargetRxKvData(drv_cmd);
    EXPECT_EQ(1U, g_dt.send_resp_count);
    EXPECT_EQ(KVPROT_CMD_STATUS_OK, cmd->cqe.status);
    EXPECT_TRUE(cmd->backend_done);
    EXPECT_TRUE(cmd->resp_sent);
    tgtTargetTgtCmdDone(drv_cmd);
    EXPECT_EQ(2U, g_dt.free_page_count);

    drv_cmd = kvprot_dt_alloc_drv_cmd(9, KVPROT_DELETE_KEY_SECTION_LEN * 9U);
    ASSERT_NE(nullptr, drv_cmd);
    cmd = kvprot_dt_upper_cmd(drv_cmd);
    tgtParseKvTargetCmd(drv_cmd);
    result_buf = (uint8_t *)cmd->drv_cmd.private_cmd.kv_cmd.result_page;
    ASSERT_NE(nullptr, result_buf);
    result_buf[0] = 0x81U;
    result_buf[1] = 0x01U;
    tgtTargetRxKvData(drv_cmd);
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd->cqe.status);
    tgtTargetTgtCmdDone(drv_cmd);
}
