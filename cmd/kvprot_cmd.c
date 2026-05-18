#include "kvprot_cmd.h"
#include "kvprot_common.h"
#include "kvprot_scat.h"
#include "dpumm_cmm.h"
#include "dplwt_api.h"
#include "vcontroller.h"
#include "libos_sched_partition.h"

static uint32_t g_kvprot_cmd_part_id;
static KVPROT_CMD_ENTRY *g_kvprot_cmd_entry_list[KVPROT_MAX_CMD_NUM];

static KVPROT_CMD_ENTRY g_kvprot_cmd_entry_set[] = {
    {
        KVPROT_CMD_OPCODE_DELETE,
        KVPROT_CMD_F_NEED_RX_DATA,
        {
            tgtKvDeleteExecute,
        },
    },
};

static void tgtKvFillResult(KVPROT_CMD *cmd, uint32_t status)
{
    cmd->cqe.status = status;
}

static KVPROT_CMD_ENTRY *tgtKvGetCmdEntry(uint16_t opcode)
{
    if (opcode >= KVPROT_MAX_CMD_NUM) {
        return NULL;
    }

    return g_kvprot_cmd_entry_list[opcode];
}

static void tgtKvSendResponse(KVPROT_CMD *cmd)
{
    scat_tgt_driver_s *drv_ops = tgtKvGetScatDrvOps();

    (void)memcpy_s(&cmd->drv_cmd.result.cqe, sizeof(cmd->drv_cmd.result.cqe), &cmd->cqe, sizeof(cmd->cqe));
    if (drv_ops == NULL || drv_ops->send_resp == NULL) {
        KVPROT_ERROR("Send kv response failed, scat driver ops is invalid.");
        return;
    }

    drv_ops->send_resp(&cmd->drv_cmd);
}

static int32_t tgtKvScheduleToNormalPart(dplwt_entry_func entry_func, void *arg)
{
    uint32_t vc_id = vc_get_vcid();
    dplwt_scpart_t *sched_part_normal = NULL;
    dplwt_id_t lwt_id = 0;
    dplwt_attr_s lwt_attr = { 0 };
    int32_t ret = DPLWT_GET_SCPART_FROM_GROUP(vc_id, CPU_PART_NAME_NORMAL, &sched_part_normal);

    if (ret != RETURN_OK || sched_part_normal == NULL) {
        KVPROT_ERROR("Get normal schedule partition failed, vc:%u ret:%d.", vc_id, ret);
        return RETURN_ERROR;
    }

    (void)memset_s(&lwt_attr, sizeof(lwt_attr), 0, sizeof(lwt_attr));
    DPLWT_ATTR_INIT(&lwt_attr);
    DPLWT_DISPATCH_BY_PARTITION_ID_SET(&lwt_attr, sched_part_normal->part_id);

    ret = DPLWT_CREATE(&lwt_id, entry_func, arg, &lwt_attr);
    if (ret != RETURN_OK) {
        KVPROT_ERROR("Create kv command lwt failed:%d.", ret);
    }

    return ret;
}

static int32_t tgtKvCmdRuntimeInit(void)
{
    return RETURN_OK;
}

static void tgtKvCmdRuntimeExit(void)
{
}

static int32_t tgtKvIoCmdInit(void)
{
    uint32_t i;

    (void)memset_s(g_kvprot_cmd_entry_list, sizeof(g_kvprot_cmd_entry_list), 0,
        sizeof(g_kvprot_cmd_entry_list));

    for (i = 0; i < ARRAY_LEN(g_kvprot_cmd_entry_set); i++) {
        g_kvprot_cmd_entry_list[g_kvprot_cmd_entry_set[i].opcode] = &g_kvprot_cmd_entry_set[i];
    }

    return RETURN_OK;
}

static void tgtKvIoCmdExit(void)
{
}

static int32_t tgtKvCmdPoolInit(void)
{
    int32_t ret = CREATE_EXPANDABLE_STRUCTURE_PARTITION(KVPROT_CMD_MAX_CONCUR_COUNT,
        KVPROT_CMD_SLAB_CNT_MAX,
        "KVPROT_CMD",
        PID_TGT_MIDDLE,
        PART_ATTR_NONE | PART_ATTR_MAX_EFFECTIVE,
        sizeof(KVPROT_CMD),
        &g_kvprot_cmd_part_id,
        ALL_CONTEXT);
    if (ret != RETURN_OK) {
        KVPROT_ERROR("Create kv protocol cmd partition failed:%d.", ret);
    }

    return ret;
}

static void tgtKvCmdPoolExit(void)
{
    DELETE_MEMORY_PARTITION(g_kvprot_cmd_part_id);
}

static const KVPROT_INIT_OPS g_kvprot_cmd_init_ops[] = {
    {"kv cmd runtime", tgtKvCmdRuntimeInit, tgtKvCmdRuntimeExit},
    {"kv io cmd",      tgtKvIoCmdInit,      tgtKvIoCmdExit},
    {"kv cmd pool",    tgtKvCmdPoolInit,    tgtKvCmdPoolExit},
};

int32_t tgtKvCmdInit(void)
{
    return KvprotInitOps(g_kvprot_cmd_init_ops, ARRAY_LEN(g_kvprot_cmd_init_ops), "kv cmd");
}

void tgtKvCmdExit(void)
{
    KvprotExitOps(g_kvprot_cmd_init_ops, ARRAY_LEN(g_kvprot_cmd_init_ops), "kv cmd");
}

static int32_t tgtKvPrepareBatchResult(KVPROT_CMD *cmd)
{
    uint16_t batch_num = cmd->sqe.batch_num;
    void *page_ctrl = ALLOCATE_ONE_PAGE_SYNC(PID_TGT_MIDDLE, __FUNCTION__, __LINE__);

    if (page_ctrl == NULL) {
        KVPROT_ERROR("Allocate batch result page failed.");
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_ALLOC_FAIL);
        return RETURN_ERROR;
    }

    cmd->result_page_ctrl = page_ctrl;
    cmd->drv_cmd.private_cmd.kv_cmd.result_page = GET_PAGE_ADDR(page_ctrl);
    cmd->drv_cmd.private_cmd.kv_cmd.result_len = batch_num;
    (void)memset_s(cmd->drv_cmd.private_cmd.kv_cmd.result_page,
        cmd->drv_cmd.private_cmd.kv_cmd.result_len, 0, cmd->drv_cmd.private_cmd.kv_cmd.result_len);

    return RETURN_OK;
}

static int32_t tgtKvPrepareDataBuffer(KVPROT_CMD *cmd)
{
    void *page_ctrl = NULL;
    uint32_t expect_len = (uint32_t)cmd->sqe.batch_num * KVPROT_DELETE_KEY_SECTION_LEN;

    if (cmd->sqe.batch_num == 0 || cmd->sqe.data_len == 0) {
        KVPROT_ERROR("Prepare data buffer failed, batch num:%u data len:%u.",
            cmd->sqe.batch_num, cmd->sqe.data_len);
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
        return RETURN_ERROR;
    }

    if (cmd->sqe.data_len != expect_len) {
        KVPROT_ERROR("Prepare data buffer failed, data len:%u expect:%u.",
            cmd->sqe.data_len, expect_len);
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
        return RETURN_ERROR;
    }

    page_ctrl = ALLOCATE_ONE_PAGE_SYNC(PID_TGT_MIDDLE, __FUNCTION__, __LINE__);
    if (page_ctrl == NULL) {
        KVPROT_ERROR("Allocate data page failed.");
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_ALLOC_FAIL);
        return RETURN_ERROR;
    }

    cmd->data_page_ctrl = page_ctrl;
    cmd->data_buf = GET_PAGE_ADDR(page_ctrl);
    cmd->data_len = cmd->sqe.data_len;
    (void)memset_s(cmd->data_buf, cmd->data_len, 0, cmd->data_len);

    cmd->drv_cmd.private_cmd.kv_cmd.icd_page = cmd->data_buf;
    cmd->drv_cmd.private_cmd.kv_cmd.icd_len = cmd->data_len;
    cmd->drv_cmd.private_cmd.kv_cmd.data_len = cmd->data_len;

    return RETURN_OK;
}

static int32_t tgtKvRequestRxData(KVPROT_CMD *cmd)
{
    scat_tgt_driver_s *drv_ops = tgtKvGetScatDrvOps();

    if (drv_ops == NULL || drv_ops->xfer_ready == NULL) {
        KVPROT_ERROR("Request rx data failed, scat driver ops is invalid.");
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
        return RETURN_ERROR;
    }

    drv_ops->xfer_ready(&cmd->drv_cmd);
    return RETURN_OK;
}

static int32_t tgtKvParseCmd(KVPROT_CMD *cmd)
{
    uint16_t opcode;

    (void)memcpy_s(&cmd->sqe, sizeof(cmd->sqe), &cmd->drv_cmd.scat_cmd, sizeof(cmd->sqe));
    cmd->cqe.cmd_id = cmd->sqe.cmd_id;
    cmd->drv_cmd.result.cqe.cmd_id = cmd->cqe.cmd_id;

    opcode = cmd->sqe.opcode;
    cmd->entry = tgtKvGetCmdEntry(opcode);
    if (cmd->entry == NULL || cmd->entry->ops.execute == NULL) {
        KVPROT_ERROR("Unsupported kv opcode:%u.", opcode);
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
        return RETURN_ERROR;
    }

    if (cmd->sqe.batch_num == 0) {
        KVPROT_ERROR("Parse kv command failed, batch num is 0.");
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
        return RETURN_ERROR;
    }
    return tgtKvPrepareBatchResult(cmd);

}

static void tgtKvExecuteParsedCmd(KVPROT_CMD *cmd)
{
    int32_t ret = cmd->entry->ops.execute(cmd);

    if (ret != RETURN_OK && cmd->cqe.status == KVPROT_CMD_STATUS_OK) {
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
    }

    tgtKvSendResponse(cmd);
}

static void tgtKvExecuteParsedCmdWrapper(void *arg)
{
    tgtKvExecuteParsedCmd((KVPROT_CMD *)arg);
}

static bool tgtKvNeedSchedule(const KVPROT_CMD *cmd)
{
    return (cmd->entry->flags & KVPROT_CMD_F_NEED_SCHEDULE) != 0;
}

static bool tgtKvNeedRxData(const KVPROT_CMD *cmd)
{
    return (cmd->entry->flags & KVPROT_CMD_F_NEED_RX_DATA) != 0;
}

scat_tgt_cmd_s *tgtGetKvTargetCmd(OSP_VOID *session, scat_cmd_cqe_s *drv_cmd_cqe)
{
    KVPROT_CMD *cmd = NULL;
    (void)session;

    ALLOCATE_STRUCTURE(PID_TGT_MIDDLE, g_kvprot_cmd_part_id, (void **)&cmd);
    if (cmd == NULL) {
        if (drv_cmd_cqe != NULL) {
            drv_cmd_cqe->status = KVPROT_CMD_STATUS_BUSY;
        }

        KVPROT_ERROR("Allocate kv target cmd failed.");
        return NULL;
    }

    (void)memset_s(cmd, sizeof(KVPROT_CMD), 0, sizeof(KVPROT_CMD));
    cmd->drv_cmd.upper = cmd;

    if (drv_cmd_cqe != NULL) {
        drv_cmd_cqe->status = KVPROT_CMD_STATUS_OK;
    }

    return &cmd->drv_cmd;
}

void tgtParseKvTargetCmd(scat_tgt_cmd_s *drv_cmd)
{
    KVPROT_CMD *cmd = NULL;
    int32_t ret;

    if (drv_cmd == NULL) {
        KVPROT_ERROR("Parse kv target cmd failed, drv cmd is null.");
        return;
    }

    cmd = (KVPROT_CMD *)drv_cmd->upper;
    if (cmd == NULL) {
        KVPROT_ERROR("Parse kv target cmd failed, private cmd is null.");
        return;
    }

    ret = tgtKvParseCmd(cmd);
    if (ret != RETURN_OK) {
        tgtKvSendResponse(cmd);
        return;
    }

    if (tgtKvNeedRxData(cmd)) {
        ret = tgtKvPrepareDataBuffer(cmd);
        if (ret != RETURN_OK) {
            tgtKvSendResponse(cmd);
            return;
        }

        ret = tgtKvRequestRxData(cmd);
        if (ret != RETURN_OK) {
            tgtKvSendResponse(cmd);
        }
        return;
    }

    if (!tgtKvNeedSchedule(cmd)) {
        tgtKvExecuteParsedCmd(cmd);
        return;
    }

    ret = tgtKvScheduleToNormalPart(tgtKvExecuteParsedCmdWrapper, (void *)cmd);
    if (ret != RETURN_OK) {
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
        tgtKvSendResponse(cmd);
    }
}

void tgtTargetRxKvData(scat_tgt_cmd_s *drv_cmd)
{
    KVPROT_CMD *cmd = NULL;
    int32_t ret;

    if (drv_cmd == NULL) {
        KVPROT_ERROR("Receive kv data failed, drv cmd is null.");
        return;
    }

    cmd = (KVPROT_CMD *)drv_cmd->upper;
    if (cmd == NULL) {
        KVPROT_ERROR("Receive kv data failed, private cmd is null.");
        return;
    }

    if (!tgtKvNeedSchedule(cmd)) {
        tgtKvExecuteParsedCmd(cmd);
        return;
    }

    ret = tgtKvScheduleToNormalPart(tgtKvExecuteParsedCmdWrapper, (void *)cmd);
    if (ret != RETURN_OK) {
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
        tgtKvSendResponse(cmd);
    }
}

void tgtTargetTgtCmdDone(scat_tgt_cmd_s *drv_cmd)
{
    KVPROT_CMD *cmd = NULL;

    if (drv_cmd == NULL) {
        return;
    }

    cmd = (KVPROT_CMD *)drv_cmd->upper;
    if (cmd == NULL) {
        return;
    }

    if (cmd->result_page_ctrl != NULL) {
        FREE_ONE_PAGE(cmd->result_page_ctrl, __FUNCTION__, __LINE__);
        cmd->result_page_ctrl = NULL;
    }

    if (cmd->data_page_ctrl != NULL) {
        FREE_ONE_PAGE(cmd->data_page_ctrl, __FUNCTION__, __LINE__);
        cmd->data_page_ctrl = NULL;
    }

    FREE_STRUCTURE(cmd, g_kvprot_cmd_part_id);
}

void tgtGetKvSense(scat_error_code_e error_code, scat_cmd_cqe_s *cmd_cqe, void *session)
{
    (void)session;

    if (cmd_cqe == NULL) {
        return;
    }

    cmd_cqe->status = (error_code == DRV_SCAT_NO_ERROR) ? KVPROT_CMD_STATUS_OK : KVPROT_CMD_STATUS_INTERNAL_ERROR;
}
