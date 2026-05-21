#ifndef KVPROT_DT_FIXTURE_H
#define KVPROT_DT_FIXTURE_H

#include <gtest/gtest.h>
#include "kvprot_dt_context.h"

static void kvprot_dt_reset_all(void);
static void kvprot_dt_driver_send_resp(scat_tgt_cmd_s *drv_cmd);
static void kvprot_dt_driver_xfer_ready(scat_tgt_cmd_s *drv_cmd);
static void kvprot_dt_add_driver(bool with_send_resp, bool with_xfer_ready);
static scat_tgt_cmd_s *kvprot_dt_alloc_drv_cmd(uint16_t batch_num, uint32_t data_len);
static KVPROT_CMD *kvprot_dt_upper_cmd(scat_tgt_cmd_s *drv_cmd);
static void kvprot_dt_free_structure(void *ptr, uint32_t part_id);
static void kvprot_dt_set_delete_cmd_flags(uint32_t flags);
static void kvprot_dt_set_delete_execute(int32_t (*execute)(KVPROT_CMD *cmd));
static void kvprot_dt_timeout_wheel_add(KVPROT_CMD *cmd);
static void kvprot_dt_timeout_wheel_remove(KVPROT_CMD *cmd);
static bool kvprot_dt_delete_result_key_failed(const uint8_t *result_buf, uint32_t key_idx);
static uint32_t kvprot_dt_delete_failed_key_count(const uint8_t *result_buf, uint16_t key_num);

class KvprotDtTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        kvprot_dt_reset_all();
    }
};

#endif
