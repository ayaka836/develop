#include "kvprot_cmd.h"
#include "kvprot_common.h"
#include "kvprot_scat.h"
#include "dpumm_cmm.h"
#include "dplwt_api.h"
#include "vcontroller.h"
#include "libos_sched_partition.h"
#include <pthread.h>
#include <unistd.h>

static uint32_t g_kvprot_cmd_part_id;
static KVPROT_CMD_ENTRY *g_kvprot_cmd_entry_list[KVPROT_MAX_CMD_NUM];

typedef struct tagKVPROT_CMD_TIMEOUT_SLOT {
    KVPROT_CMD *head;
} KVPROT_CMD_TIMEOUT_SLOT;

typedef struct tagKVPROT_CMD_TIMEOUT_CONTEXT {
    spinlock_t lock;
    bool stop;
    bool thread_started;
    uint32_t current_slot;
    pthread_t thread_id;
    KVPROT_CMD_TIMEOUT_SLOT slots[KVPROT_CMD_TIMEOUT_WHEEL_SLOT_NUM];
} KVPROT_CMD_TIMEOUT_CONTEXT;

static KVPROT_CMD_TIMEOUT_CONTEXT g_kvprot_cmd_timeout_ctx;

static KVPROT_CMD_ENTRY g_kvprot_cmd_entry_set[] = {
    {
        KVPROT_CMD_OPCODE_DELETE,
        KVPROT_CMD_F_NEED_RX_DATA,
        {
            tgtKvDeleteValidate,
            tgtKvDeleteExecute,
        },
    },
};

static void tgtKvFillResult(KVPROT_CMD *cmd, uint32_t status)
{
    cmd->cqe.status = status;
}

static uint32_t tgtKvCmdTimeoutTicks(uint32_t timeout_ms)
{
    uint32_t interval_ms = KVPROT_CMD_TIMEOUT_SCAN_INTERVAL_MS;
    uint32_t ticks;

    if (interval_ms == 0 || timeout_ms == 0) {
        return 1;
    }

    ticks = (timeout_ms + interval_ms - 1) / interval_ms;
    return (ticks == 0) ? 1 : ticks;
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

static void tgtKvCmdTimeoutWheelLink(KVPROT_CMD *cmd, uint32_t slot)
{
    cmd->timeout_prev = NULL;
    cmd->timeout_next = g_kvprot_cmd_timeout_ctx.slots[slot].head;
    if (g_kvprot_cmd_timeout_ctx.slots[slot].head != NULL) {
        g_kvprot_cmd_timeout_ctx.slots[slot].head->timeout_prev = cmd;
    }
    g_kvprot_cmd_timeout_ctx.slots[slot].head = cmd;
    cmd->timeout_slot = slot;
    cmd->in_timeout_list = true;
}

static void tgtKvCmdTimeoutWheelUnlink(KVPROT_CMD *cmd)
{
    uint32_t slot = cmd->timeout_slot;

    if (cmd->timeout_prev != NULL) {
        cmd->timeout_prev->timeout_next = cmd->timeout_next;
    } else if (g_kvprot_cmd_timeout_ctx.slots[slot].head == cmd) {
        g_kvprot_cmd_timeout_ctx.slots[slot].head = cmd->timeout_next;
    }

    if (cmd->timeout_next != NULL) {
        cmd->timeout_next->timeout_prev = cmd->timeout_prev;
    }
    cmd->timeout_prev = NULL;
    cmd->timeout_next = NULL;
    cmd->in_timeout_list = false;
}

static void tgtKvCmdTimeoutWheelAdd(KVPROT_CMD *cmd)
{
    uint32_t ticks;
    uint32_t slot;

    if (cmd == NULL || cmd->in_timeout_list) {
        return;
    }

    spin_lock(&g_kvprot_cmd_timeout_ctx.lock);
    ticks = tgtKvCmdTimeoutTicks(KVPROT_DELETE_BDM_TIMEOUT_MS);
    slot = (g_kvprot_cmd_timeout_ctx.current_slot + ticks) % KVPROT_CMD_TIMEOUT_WHEEL_SLOT_NUM;
    cmd->timeout_round = (ticks - 1) / KVPROT_CMD_TIMEOUT_WHEEL_SLOT_NUM;
    tgtKvCmdTimeoutWheelLink(cmd, slot);
    spin_unlock(&g_kvprot_cmd_timeout_ctx.lock);
}

static void tgtKvCmdTimeoutWheelRemove(KVPROT_CMD *cmd)
{
    if (cmd == NULL) {
        return;
    }

    spin_lock(&g_kvprot_cmd_timeout_ctx.lock);
    if (cmd->in_timeout_list) {
        tgtKvCmdTimeoutWheelUnlink(cmd);
    }
    spin_unlock(&g_kvprot_cmd_timeout_ctx.lock);
}

static void tgtKvCmdFreeResource(KVPROT_CMD *cmd)
{
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

static void tgtKvCmdTryFree(KVPROT_CMD *cmd)
{
    bool need_free = false;

    if (cmd == NULL) {
        return;
    }

    tgtKvCmdTimeoutWheelRemove(cmd);
    spin_lock(&cmd->lock);
    need_free = cmd->scat_done && cmd->backend_done;
    spin_unlock(&cmd->lock);

    if (!need_free) {
        return;
    }

    tgtKvCmdFreeResource(cmd);
}

void tgtKvCmdMarkBackendPending(KVPROT_CMD *cmd)
{
    if (cmd == NULL) {
        return;
    }

    spin_lock(&cmd->lock);
    cmd->state = KVPROT_CMD_STATE_WAIT_BACKEND;
    cmd->backend_done = false;
    spin_unlock(&cmd->lock);

    tgtKvCmdTimeoutWheelAdd(cmd);
    KVPROT_INFO("KV cmd wait backend, opcode:%u cmd_id:%u nsid:%u.",
        cmd->sqe.opcode, cmd->sqe.cmd_id, cmd->sqe.nsid);
}

void tgtKvCmdComplete(KVPROT_CMD *cmd, uint32_t status)
{
    bool need_resp = false;

    if (cmd == NULL) {
        return;
    }

    spin_lock(&cmd->lock);
    if (!cmd->resp_sent) {
        tgtKvFillResult(cmd, status);
        cmd->resp_sent = true;
        cmd->state = (status == KVPROT_CMD_STATUS_TIMEOUT) ?
            KVPROT_CMD_STATE_TIMEOUT : KVPROT_CMD_STATE_COMPLETE;
        need_resp = true;
    }
    spin_unlock(&cmd->lock);

    if (need_resp) {
        tgtKvSendResponse(cmd);
    }
}

void tgtKvCmdBackendDone(KVPROT_CMD *cmd, uint32_t status)
{
    bool need_resp = false;

    if (cmd == NULL) {
        return;
    }

    tgtKvCmdTimeoutWheelRemove(cmd);
    spin_lock(&cmd->lock);
    if (cmd->backend_done) {
        spin_unlock(&cmd->lock);
        return;
    }

    cmd->backend_done = true;
    if (!cmd->resp_sent) {
        tgtKvFillResult(cmd, status);
        cmd->resp_sent = true;
        cmd->state = (status == KVPROT_CMD_STATUS_TIMEOUT) ?
            KVPROT_CMD_STATE_TIMEOUT : KVPROT_CMD_STATE_COMPLETE;
        need_resp = true;
    }
    spin_unlock(&cmd->lock);

    if (need_resp) {
        tgtKvSendResponse(cmd);
    }
    tgtKvCmdTryFree(cmd);
}

void tgtKvCmdTimeoutScan(void)
{
    uint32_t slot;
    KVPROT_CMD *iter = NULL;
    KVPROT_CMD *next = NULL;
    KVPROT_CMD *timeout_head = NULL;

    spin_lock(&g_kvprot_cmd_timeout_ctx.lock);
    g_kvprot_cmd_timeout_ctx.current_slot =
        (g_kvprot_cmd_timeout_ctx.current_slot + 1) % KVPROT_CMD_TIMEOUT_WHEEL_SLOT_NUM;
    slot = g_kvprot_cmd_timeout_ctx.current_slot;

    iter = g_kvprot_cmd_timeout_ctx.slots[slot].head;
    while (iter != NULL) {
        next = iter->timeout_next;
        spin_lock(&iter->lock);
        if (iter->backend_done || iter->resp_sent || iter->state != KVPROT_CMD_STATE_WAIT_BACKEND) {
            tgtKvCmdTimeoutWheelUnlink(iter);
            spin_unlock(&iter->lock);
            iter = next;
            continue;
        }

        if (iter->timeout_round > 0) {
            iter->timeout_round--;
            spin_unlock(&iter->lock);
            iter = next;
            continue;
        }

        iter->resp_sent = true;
        iter->state = KVPROT_CMD_STATE_TIMEOUT;
        tgtKvFillResult(iter, KVPROT_CMD_STATUS_TIMEOUT);
        tgtKvCmdTimeoutWheelUnlink(iter);
        iter->timeout_next = timeout_head;
        timeout_head = iter;
        spin_unlock(&iter->lock);
        iter = next;
    }
    spin_unlock(&g_kvprot_cmd_timeout_ctx.lock);

    while (timeout_head != NULL) {
        iter = timeout_head;
        timeout_head = timeout_head->timeout_next;
        iter->timeout_next = NULL;
        KVPROT_ERROR("KV cmd backend timeout, opcode:%u cmd_id:%u nsid:%u timeout:%u.",
            iter->sqe.opcode, iter->sqe.cmd_id, iter->sqe.nsid, KVPROT_DELETE_BDM_TIMEOUT_MS);
        tgtKvSendResponse(iter);
    }
}

static void *tgtKvCmdTimeoutThread(void *arg)
{
    (void)arg;

    while (!g_kvprot_cmd_timeout_ctx.stop) {
        tgtKvCmdTimeoutScan();
        (void)usleep(KVPROT_CMD_TIMEOUT_SCAN_INTERVAL_MS * 1000);
    }

    return NULL;
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

static int32_t tgtKvCmdTimeoutInit(void)
{
    (void)memset_s(&g_kvprot_cmd_timeout_ctx, sizeof(g_kvprot_cmd_timeout_ctx), 0,
        sizeof(g_kvprot_cmd_timeout_ctx));
    spin_lock_init(&g_kvprot_cmd_timeout_ctx.lock);

    if (pthread_create(&g_kvprot_cmd_timeout_ctx.thread_id, NULL,
        tgtKvCmdTimeoutThread, NULL) != RETURN_OK) {
        KVPROT_ERROR("Create kv command timeout thread failed.");
        return RETURN_ERROR;
    }
    g_kvprot_cmd_timeout_ctx.thread_started = true;
    return RETURN_OK;
}

static void tgtKvCmdTimeoutExit(void)
{
    g_kvprot_cmd_timeout_ctx.stop = true;
    if (g_kvprot_cmd_timeout_ctx.thread_started) {
        (void)pthread_join(g_kvprot_cmd_timeout_ctx.thread_id, NULL);
        g_kvprot_cmd_timeout_ctx.thread_started = false;
    }
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
    {"kv io cmd",      tgtKvIoCmdInit,      tgtKvIoCmdExit},
    {"kv cmd pool",    tgtKvCmdPoolInit,    tgtKvCmdPoolExit},
    {"kv cmd timeout", tgtKvCmdTimeoutInit, tgtKvCmdTimeoutExit},
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
        KVPROT_ERROR("Unsupported kv opcode:%u cmd_id:%u nsid:%u.",
            opcode, cmd->sqe.cmd_id, cmd->sqe.nsid);
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
        return RETURN_ERROR;
    }

    if (cmd->entry->ops.validate != NULL && cmd->entry->ops.validate(cmd) != RETURN_OK) {
        return RETURN_ERROR;
    }

    return tgtKvPrepareBatchResult(cmd);

}

static void tgtKvExecuteParsedCmd(KVPROT_CMD *cmd)
{
    int32_t ret;
    bool wait_backend = false;

    ret = cmd->entry->ops.execute(cmd);

    spin_lock(&cmd->lock);
    wait_backend = !cmd->backend_done && cmd->state == KVPROT_CMD_STATE_WAIT_BACKEND;
    spin_unlock(&cmd->lock);
    if (wait_backend) {
        return;
    }

    if (ret != RETURN_OK && cmd->cqe.status == KVPROT_CMD_STATUS_OK) {
        tgtKvFillResult(cmd, KVPROT_CMD_STATUS_INTERNAL_ERROR);
    }

    tgtKvCmdComplete(cmd, cmd->cqe.status);
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
    spin_lock_init(&cmd->lock);
    cmd->backend_done = true;
    cmd->drv_cmd.upper = cmd;
    KVPROT_INFO("Get kv target cmd success.");

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
        tgtKvCmdComplete(cmd, cmd->cqe.status);
        return;
    }

    if (tgtKvNeedRxData(cmd)) {
        ret = tgtKvPrepareDataBuffer(cmd);
        if (ret != RETURN_OK) {
            tgtKvCmdComplete(cmd, cmd->cqe.status);
            return;
        }

        ret = tgtKvRequestRxData(cmd);
        if (ret != RETURN_OK) {
            tgtKvCmdComplete(cmd, cmd->cqe.status);
            return;
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
        tgtKvCmdComplete(cmd, cmd->cqe.status);
        return;
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
        tgtKvCmdComplete(cmd, cmd->cqe.status);
        return;
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

    spin_lock(&cmd->lock);
    cmd->scat_done = true;
    spin_unlock(&cmd->lock);
    tgtKvCmdTryFree(cmd);
}

void tgtGetKvSense(scat_error_code_e error_code, scat_cmd_cqe_s *cmd_cqe, void *session)
{
    (void)session;

    if (cmd_cqe == NULL) {
        return;
    }

    cmd_cqe->status = (error_code == DRV_SCAT_NO_ERROR) ? KVPROT_CMD_STATUS_OK : KVPROT_CMD_STATUS_INTERNAL_ERROR;
}
