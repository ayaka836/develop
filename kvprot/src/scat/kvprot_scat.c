#include "kvprot_common.h"
#include "kvprot_cmd.h"
#include "kvprot_scat.h"

static KVPROT_SCAT_CONTEXT g_kvprot_scat_ctx;

static void tgtKvScatContextInit(void)
{
    (void)memset_s(&g_kvprot_scat_ctx, sizeof(g_kvprot_scat_ctx), 0, sizeof(g_kvprot_scat_ctx));
    spin_lock_init(&g_kvprot_scat_ctx.lock);
}

int32_t tgtAddKvTargetDriver(const scat_tgt_driver_s *driver_ops)
{
    if (driver_ops == NULL) {
        KVPROT_ERROR("Add kv target driver failed, driver ops is null.");
        return RETURN_ERROR;
    }

    spin_lock(&g_kvprot_scat_ctx.lock);
    if (g_kvprot_scat_ctx.is_registered) {
        spin_unlock(&g_kvprot_scat_ctx.lock);
        KVPROT_ERROR("Add kv target driver failed, driver already registered.");
        return RETURN_ERROR;
    }

    (void)memcpy_s(&g_kvprot_scat_ctx.drv_ops, sizeof(g_kvprot_scat_ctx.drv_ops),
        driver_ops, sizeof(*driver_ops));
    g_kvprot_scat_ctx.is_registered = true;
    spin_unlock(&g_kvprot_scat_ctx.lock);

    KVPROT_INFO("Add kv target driver success.");
    return RETURN_OK;
}

int32_t tgtRemoveKvTargetDriver(const scat_tgt_driver_s *driver_ops)
{
    (void)driver_ops;

    spin_lock(&g_kvprot_scat_ctx.lock);
    if (!g_kvprot_scat_ctx.is_registered) {
        spin_unlock(&g_kvprot_scat_ctx.lock);
        KVPROT_INFO("Remove kv target driver ignored, driver is not registered.");
        return RETURN_OK;
    }

    (void)memset_s(&g_kvprot_scat_ctx.drv_ops, sizeof(g_kvprot_scat_ctx.drv_ops),
        0, sizeof(g_kvprot_scat_ctx.drv_ops));
    g_kvprot_scat_ctx.is_registered = false;
    spin_unlock(&g_kvprot_scat_ctx.lock);

    KVPROT_INFO("Remove kv target driver success.");
    return RETURN_OK;
}

scat_tgt_driver_s *tgtKvGetScatDrvOps(void)
{
    if (!g_kvprot_scat_ctx.is_registered) {
        return NULL;
    }

    return &g_kvprot_scat_ctx.drv_ops;
}

int32_t tgtKvScatIntfInit(void)
{
    scat_drv_interface_s scat_intf = { 0 };

    tgtKvScatContextInit();

    scat_intf.protocol_type = SCAT_PROTOCOL_TYPE_KV;
    scat_intf.add_scat_tgt_driver = tgtAddKvTargetDriver;
    scat_intf.remove_scat_tgt_driver = tgtRemoveKvTargetDriver;
    scat_intf.get_tgt_cmd = tgtGetKvTargetCmd;
    scat_intf.parse_tgt_cmd = tgtParseKvTargetCmd;
    scat_intf.rx_data = tgtTargetRxKvData;
    scat_intf.tgt_cmd_done = tgtTargetTgtCmdDone;
    scat_intf.get_sense = tgtGetKvSense;

    if (scat_frame_register_intf(&scat_intf) != RETURN_OK) {
        KVPROT_ERROR("Register kv scat interface failed.");
        return RETURN_ERROR;
    }

    return RETURN_OK;
}

void tgtKvScatIntfExit(void)
{
    scat_frame_unregister_intf(SCAT_PROTOCOL_TYPE_KV);
    (void)tgtRemoveKvTargetDriver(NULL);
}
