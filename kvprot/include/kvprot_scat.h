#ifndef KVPROT_SCAT_H
#define KVPROT_SCAT_H

#include "lvos.h"
#include "drv_scat_target.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagKVPROT_SCAT_CONTEXT {
    bool is_registered;
    spinlock_t lock;
    scat_tgt_driver_s drv_ops;
} KVPROT_SCAT_CONTEXT;

int32_t tgtKvScatIntfInit(void);
void tgtKvScatIntfExit(void);

int32_t tgtAddKvTargetDriver(const scat_tgt_driver_s *driver_ops);
int32_t tgtRemoveKvTargetDriver(const scat_tgt_driver_s *driver_ops);
scat_tgt_driver_s *tgtKvGetScatDrvOps(void);

#ifdef __cplusplus
}
#endif

#endif
