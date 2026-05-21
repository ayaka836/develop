#ifndef KVPROT_DT_CONTEXT_H
#define KVPROT_DT_CONTEXT_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kvprot_common.h"
#include "kvprot_cmd.h"
#include "kvprot_init.h"
#include "kvprot_scat.h"
#include "dpumm_cmm.h"
#include "dplwt_api.h"
#include "vcontroller.h"
#include "libos_sched_partition.h"
#include "drv_scat_target.h"
#include <pthread.h>
#include <unistd.h>

#ifndef CPU_PART_NAME_NORMAL
#define CPU_PART_NAME_NORMAL "IOD_NORMAL"
#endif

typedef struct tagKVPROT_DT_PAGE {
    uint8_t data[4096];
} KVPROT_DT_PAGE;

typedef struct tagKVPROT_DT_STATE {
    int32_t scat_register_ret;
    uint32_t scat_register_count;
    uint32_t scat_unregister_count;
    uint32_t scat_unregister_protocol;
    scat_drv_interface_s scat_intf;

    int32_t create_partition_ret;
    uint32_t created_part_id;
    uint32_t create_partition_count;
    uint32_t delete_partition_count;
    uint32_t allocate_structure_count;
    uint32_t free_structure_count;
    uint32_t allocate_page_count;
    uint32_t free_page_count;
    uint32_t fail_structure_alloc_call;
    uint32_t fail_page_alloc_call;

    uint32_t send_resp_count;
    uint32_t xfer_ready_count;
    uint32_t last_resp_status;
    scat_tgt_cmd_s *last_resp_cmd;
    scat_tgt_cmd_s *last_xfer_cmd;

    uint32_t vc_id;
    dplwt_scpart_t normal_part;
    int32_t dplwt_get_ret;
    int32_t dplwt_create_ret;
    bool dplwt_run_inline;
    uint32_t dplwt_get_count;
    uint32_t dplwt_create_count;
    uint32_t dplwt_attr_part_id;

    int32_t pthread_create_ret;
    uint32_t pthread_create_count;
    uint32_t pthread_join_count;
    uint32_t usleep_count;
} KVPROT_DT_STATE;

static KVPROT_DT_STATE g_dt;
static pthread_t g_dt_fake_thread;

#endif
