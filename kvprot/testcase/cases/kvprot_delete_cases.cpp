#include "kvprot_dt_fixture.h"

TEST_F(KvprotDtTest, DeleteValidateExecuteAndResultBitmap)
{
    KVPROT_CMD cmd;
    uint8_t result_buf[2] = {0};
    uint8_t data_buf[KVPROT_DELETE_KEY_SECTION_LEN * 2U] = {0};

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.sqe.batch_num = 0;
    cmd.sqe.data_len = KVPROT_DELETE_KEY_SECTION_LEN;
    EXPECT_EQ(RETURN_ERROR, tgtKvDeleteValidate(&cmd));
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd.cqe.status);

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.sqe.batch_num = 1;
    cmd.sqe.data_len = 0;
    EXPECT_EQ(RETURN_ERROR, tgtKvDeleteValidate(&cmd));

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.sqe.batch_num = 2;
    cmd.sqe.data_len = KVPROT_DELETE_KEY_SECTION_LEN;
    EXPECT_EQ(RETURN_ERROR, tgtKvDeleteValidate(&cmd));

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.sqe.batch_num = 2;
    cmd.sqe.data_len = KVPROT_DELETE_KEY_SECTION_LEN * 2U;
    EXPECT_EQ(RETURN_OK, tgtKvDeleteValidate(&cmd));

    EXPECT_FALSE(kvprot_dt_delete_result_key_failed(result_buf, 0));
    result_buf[0] = 0x85U;
    result_buf[1] = 0x01U;
    EXPECT_TRUE(kvprot_dt_delete_result_key_failed(result_buf, 0));
    EXPECT_TRUE(kvprot_dt_delete_result_key_failed(result_buf, 2));
    EXPECT_TRUE(kvprot_dt_delete_result_key_failed(result_buf, 7));
    EXPECT_TRUE(kvprot_dt_delete_result_key_failed(result_buf, 8));
    EXPECT_EQ(4U, kvprot_dt_delete_failed_key_count(result_buf, 9));

    EXPECT_EQ(RETURN_OK, tgtKvCmdInit());
    kvprot_dt_add_driver(true, true);

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.sqe.batch_num = 1;
    cmd.sqe.data_len = KVPROT_DELETE_KEY_SECTION_LEN;
    EXPECT_EQ(RETURN_ERROR, tgtKvDeleteExecute(&cmd));
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd.cqe.status);

    (void)memset(&cmd, 0, sizeof(cmd));
    cmd.sqe.batch_num = 2;
    cmd.sqe.data_len = KVPROT_DELETE_KEY_SECTION_LEN * 2U;
    cmd.data_buf = data_buf;
    cmd.drv_cmd.private_cmd.kv_cmd.result_page = result_buf;
    cmd.drv_cmd.private_cmd.kv_cmd.result_len = sizeof(result_buf);
    spin_lock_init(&cmd.lock);
    EXPECT_EQ(RETURN_ERROR, tgtKvDeleteExecute(&cmd));
    EXPECT_EQ(KVPROT_CMD_STATUS_INTERNAL_ERROR, cmd.cqe.status);
}
