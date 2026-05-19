#ifndef KVPROT_CMD_H
#define KVPROT_CMD_H

#include "lvos.h"
#include "drv_scat_target.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KVPROT_MAX_CMD_NUM 256
#define KVPROT_CMD_MAX_CONCUR_COUNT (8192 * 2)
#define KVPROT_CMD_SLAB_CNT_MAX (KVPROT_CMD_MAX_CONCUR_COUNT * 2)

#define KVPROT_DELETE_KEY_SECTION_LEN 16
#define KVPROT_RESULT_BUF_KEY_SECTION_LEN_BIT 1
#define KVPROT_RESULT_BUF_KEY_NUM_PER_BYTE (8 / KVPROT_RESULT_BUF_KEY_SECTION_LEN_BIT)

#define KVPROT_CMD_OPCODE_DELETE 0x08

#define KVPROT_CMD_STATUS_OK 0x00
#define KVPROT_CMD_STATUS_INTERNAL_ERROR 0x01
#define KVPROT_CMD_STATUS_BUSY 0x02
#define KVPROT_CMD_STATUS_ALLOC_FAIL 0x07
#define KVPROT_CMD_STATUS_TIMEOUT 0x09

#define KVPROT_DELETE_BDM_TIMEOUT_MS 1000

#define KVPROT_CMD_F_NEED_SCHEDULE 0x1
#define KVPROT_CMD_F_NEED_RX_DATA 0x2

#define KVPROT_CMD_HANDLE_INVALID 0ULL
#define KVPROT_CMD_CPU_INVALID 0xFFFFFFFFU

typedef enum tagKVPROT_CMD_STATE {
    KVPROT_CMD_STATE_ALLOC = 0,
    KVPROT_CMD_STATE_PARSE,
    KVPROT_CMD_STATE_WAIT_RX_DATA,
    KVPROT_CMD_STATE_RX_DATA_DONE,
    KVPROT_CMD_STATE_SCHEDULED,
    KVPROT_CMD_STATE_EXECUTE,
    KVPROT_CMD_STATE_WAIT_BACKEND,
    KVPROT_CMD_STATE_COMPLETE,
    KVPROT_CMD_STATE_DONE,
    KVPROT_CMD_STATE_BUTT
} KVPROT_CMD_STATE;

typedef struct tagKVPROT_CMD_SQE {
    uint16_t opcode : 8;
    uint16_t rsvd1 : 5;
    uint16_t rflag : 1;
    uint16_t rsvd2 : 2;
    uint16_t cmd_id;
    uint32_t nsid;
    uint32_t rsvd3;
    uint64_t rsp_addr;
    uint32_t rsp_mr_key;
    uint64_t data_pointer;
    uint32_t data_len;
    uint32_t rsvd4 : 24;
    uint32_t data_type : 8;
    uint16_t batch_num;
    uint16_t rsvd5;
    uint32_t rsvd6;
    uint32_t rsvd7;
    uint32_t rsvd8;
    uint32_t rsvd9;
    uint32_t rsvd10;
} KVPROT_CMD_SQE;

typedef struct tagKVPROT_CMD_CQE {
    uint16_t result;
    uint16_t rsvd0;
    uint32_t rsvd1;
    uint32_t rsvd2;
    uint16_t cmd_id;
    uint16_t rsvd3 : 1;
    uint16_t status : 15;
} KVPROT_CMD_CQE;

struct tagKVPROT_CMD;

typedef struct tagKVPROT_CMD_OPS {
    int32_t (*execute)(struct tagKVPROT_CMD *cmd);
} KVPROT_CMD_OPS;

typedef struct tagKVPROT_CMD_ENTRY {
    uint16_t opcode;
    uint32_t flags;
    KVPROT_CMD_OPS ops;
} KVPROT_CMD_ENTRY;

typedef struct tagKVPROT_CMD_TRACE {
    uint32_t alloc_cpu;
    uint32_t execute_cpu;
    uint64_t alloc_time_ms;
    uint64_t parse_time_ms;
    uint64_t rx_data_time_ms;
    uint64_t execute_time_ms;
    uint64_t complete_time_ms;
    KVPROT_CMD_STATE state;
    uint16_t opcode;
    uint16_t cmd_id;
    uint32_t nsid;
} KVPROT_CMD_TRACE;

typedef struct tagKVPROT_CMD {
    scat_tgt_cmd_s drv_cmd;
    uint64_t handle;
    KVPROT_CMD_TRACE trace;
    KVPROT_CMD_SQE sqe;
    KVPROT_CMD_CQE cqe;
    KVPROT_CMD_ENTRY *entry;
    void *result_page_ctrl;
    void *data_page_ctrl;
    void *data_buf;
    uint32_t data_len;
} KVPROT_CMD;

int32_t tgtKvCmdInit(void);
void tgtKvCmdExit(void);

scat_tgt_cmd_s *tgtGetKvTargetCmd(OSP_VOID *session, scat_cmd_cqe_s *drv_cmd_cqe);
void tgtParseKvTargetCmd(scat_tgt_cmd_s *drv_cmd);
void tgtTargetRxKvData(scat_tgt_cmd_s *drv_cmd);
void tgtTargetTgtCmdDone(scat_tgt_cmd_s *drv_cmd);
void tgtGetKvSense(scat_error_code_e error_code, scat_cmd_cqe_s *cmd_cqe, void *session);

uint64_t tgtKvCmdHandleAlloc(KVPROT_CMD *cmd);
void tgtKvCmdHandleFree(KVPROT_CMD *cmd);
KVPROT_CMD *tgtKvCmdGetByHandle(uint64_t handle);
KVPROT_CMD *tgtKvCmdClaimByHandle(uint64_t handle);
void tgtKvCmdMarkBackendPending(KVPROT_CMD *cmd);
void tgtKvCmdComplete(KVPROT_CMD *cmd, uint32_t status);
int32_t tgtKvCmdCompleteByHandle(uint64_t handle, uint32_t status);
void tgtKvCmdDumpTrace(const KVPROT_CMD *cmd, const char *reason);

int32_t tgtKvDeleteExecute(KVPROT_CMD *cmd);

#ifdef __cplusplus
}
#endif

#endif
