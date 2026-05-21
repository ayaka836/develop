#include "kvprot_dt_fixture.h"

TEST_F(KvprotDtTest, ScatInterfaceRegistrationAndDriverLifecycle)
{
    scat_tgt_driver_s driver;

    EXPECT_EQ(RETURN_OK, tgtKvScatIntfInit());
    EXPECT_EQ(1U, g_dt.scat_register_count);
    EXPECT_EQ(SCAT_PROTOCOL_TYPE_KV, g_dt.scat_intf.protocol_type);
    ASSERT_NE(nullptr, g_dt.scat_intf.add_scat_tgt_driver);
    ASSERT_NE(nullptr, g_dt.scat_intf.remove_scat_tgt_driver);
    ASSERT_NE(nullptr, g_dt.scat_intf.get_tgt_cmd);
    ASSERT_NE(nullptr, g_dt.scat_intf.parse_tgt_cmd);
    ASSERT_NE(nullptr, g_dt.scat_intf.rx_data);
    ASSERT_NE(nullptr, g_dt.scat_intf.tgt_cmd_done);
    ASSERT_NE(nullptr, g_dt.scat_intf.get_sense);

    EXPECT_EQ(nullptr, tgtKvGetScatDrvOps());
    EXPECT_EQ(RETURN_ERROR, tgtAddKvTargetDriver(nullptr));

    (void)memset(&driver, 0, sizeof(driver));
    driver.send_resp = kvprot_dt_driver_send_resp;
    driver.xfer_ready = kvprot_dt_driver_xfer_ready;
    EXPECT_EQ(RETURN_OK, tgtAddKvTargetDriver(&driver));
    ASSERT_NE(nullptr, tgtKvGetScatDrvOps());
    EXPECT_EQ(RETURN_ERROR, tgtAddKvTargetDriver(&driver));

    EXPECT_EQ(RETURN_OK, tgtRemoveKvTargetDriver(&driver));
    EXPECT_EQ(nullptr, tgtKvGetScatDrvOps());
    EXPECT_EQ(RETURN_OK, tgtRemoveKvTargetDriver(nullptr));

    tgtKvScatIntfExit();
    EXPECT_EQ(1U, g_dt.scat_unregister_count);
    EXPECT_EQ(SCAT_PROTOCOL_TYPE_KV, g_dt.scat_unregister_protocol);
}

TEST_F(KvprotDtTest, ScatInterfaceRegisterFailure)
{
    g_dt.scat_register_ret = RETURN_ERROR;
    EXPECT_EQ(RETURN_ERROR, tgtKvScatIntfInit());
    EXPECT_EQ(1U, g_dt.scat_register_count);
}

TEST_F(KvprotDtTest, ProtocolInitExitSuccessAndRollback)
{
    EXPECT_EQ(RETURN_OK, KvProtocolInit());
    EXPECT_EQ(1U, g_dt.scat_register_count);
    EXPECT_EQ(1U, g_dt.create_partition_count);
    EXPECT_EQ(1U, g_dt.pthread_create_count);

    KvProtocolExit();
    EXPECT_EQ(1U, g_dt.scat_unregister_count);
    EXPECT_EQ(1U, g_dt.pthread_join_count);
    EXPECT_EQ(1U, g_dt.delete_partition_count);

    kvprot_dt_reset_all();
    g_dt.create_partition_ret = -9;
    EXPECT_EQ(-9, KvProtocolInit());
    EXPECT_EQ(1U, g_dt.scat_register_count);
    EXPECT_EQ(1U, g_dt.scat_unregister_count);
    EXPECT_EQ(0U, g_dt.pthread_create_count);
}

TEST_F(KvprotDtTest, CmdInitFailurePaths)
{
    g_dt.create_partition_ret = -5;
    EXPECT_EQ(-5, tgtKvCmdInit());
    EXPECT_EQ(1U, g_dt.create_partition_count);

    kvprot_dt_reset_all();
    g_dt.pthread_create_ret = RETURN_ERROR;
    EXPECT_EQ(RETURN_ERROR, tgtKvCmdInit());
    EXPECT_EQ(1U, g_dt.create_partition_count);
    EXPECT_EQ(1U, g_dt.delete_partition_count);
}
