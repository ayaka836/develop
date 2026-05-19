#include "kvprot_scat.h"
#include "kvprot_cmd.h"
#include "kvprot_common.h"

static const KVPROT_INIT_OPS g_kvprot_init_ops[] = {
    {"kv scat intf", tgtKvScatIntfInit, tgtKvScatIntfExit},
    {"kv cmd",       tgtKvCmdInit,      tgtKvCmdExit},
};

int32_t KvProtocolInit(void)
{
    return KvprotInitOps(g_kvprot_init_ops, ARRAY_LEN(g_kvprot_init_ops), "kv module");
}

void KvProtocolExit(void)
{
    KvprotExitOps(g_kvprot_init_ops, ARRAY_LEN(g_kvprot_init_ops), "kv module");
}
