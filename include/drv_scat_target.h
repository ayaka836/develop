
#ifndef DRV_SCAT_TARGET_H
#define DRV_SCAT_TARGET_H

#include "drv_common.h"
#include "drv_sgl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#define DRV_SCAT_RETURN_OK 0

#define DRV_SCAT_RETURN_ERROR 1

/* Target驱动错误码 */
typedef enum {
    DRV_SCAT_NO_ERROR           = 0x0000,   /* <没有错误 */
    DRV_SCAT_INTERNAL_ERROR     = 0x1000,   /* <阵列内部处理错误 */
    DRV_SCAT_TGTERR_NO_RESOURCE = 0x2000,   /* <驱动IO资源耗尽 */
} scat_error_code_e;

typedef enum {
    SCAT_PROTOCOL_TYPE_KV,
    SCAT_PROTOCOL_TYPE_BUTT
} scat_protocol_type_e;

/* 注册Tgt Host时，驱动的本地端口信息 */
typedef struct {
    uint64_t port_id;             /* 引擎内的物理端口 */
    uint64_t virtual_port_id;     /* 集群内的LIF ID */
    uint32_t hardware_ability;    /* 端口硬件能力预留，按bit使用 */
    uint32_t protocol_type;       /* 驱动协议类型 scat_protocol_type_e */
} scat_target_port_info_s;

/* session信息数据结构 */
typedef struct {
    uint16_t protocol_type;               /* 语义类型，当前只支持KV */
    uint8_t status;                       /* session状态: 1: UP, 0: DOWN */
    uint8_t session_type;                 /* session类型 DRV_SESSION_TYPE_E */
    uint64_t vport_id;                    /* 虚拟端口portid */
    uint8_t ini_ip[DRV_IPV6_MAX + 1];
    uint8_t tgt_ip[DRV_IPV6_MAX + 1];
} scat_session_info_s;

typedef struct {
    uint64_t addr;
    uint8_t length[3];  /* Length 3 个字节 */
    uint8_t key[4];     /* Key 4 个字节 */
    uint8_t type;
} scat_datat_ptr_s;

/* scat命令 */
typedef struct {
    uint16_t opcode : 8;
    uint16_t rsvd1 : 6;
    uint16_t psdt : 2;
    uint16_t cid;              /* 命令标识符 */
    uint32_t nsid;
    uint32_t rsvd2;
    uint32_t rsvd3;
    uint32_t rsvd4;
    uint32_t rsvd5;
    scat_datat_ptr_s dptr;     /* 4个dword */
    uint32_t dword10;
    uint32_t dword11;
    uint32_t dword12;
    uint32_t dword13;
    uint32_t dword14;
    uint32_t dword15;
} scat_cmd_s;

/* scat cmd的CQE结构 */
typedef struct {
    uint16_t result;           /* exist命令使用 */
    uint16_t rsvd0;
    uint32_t rsvd1;
    uint32_t rsvd2;
    uint16_t cmd_id;           /* 命令标识符 */
    uint16_t rsvd3 : 1;        /* Phrase bit预留 */
    uint16_t status : 15;      /* IO状态 */
} scat_cmd_cqe_s;

/* scat cmd的创分结果 */
typedef struct {
    uint32_t disk_id;
    uint32_t data_pos;
    uint32_t rsvd[2];
} scat_cmd_create_index_s;

/* scat cmd的结果 */
typedef union {
    scat_cmd_cqe_s cqe;                    /* SCAT读写命令处理结果CQE，由产品应用填充 */
    scat_cmd_create_index_s create_index;  /* 创分命令处理结果，由产品应用填充 */
} scat_cmd_result_u;

typedef struct {
    void *icd_page;                       /* KV delete/exist命令中的立即数页面指针，由KV驱动填充 */
    void *result_page;                    /* KV命令用于delete/exist处理结果，由产品应用填充，使用umm page */
    uint16_t icd_len;                     /* 立即数有效长度，由KV驱动填充 */
    uint16_t result_len;                  /* KV result有效长度，由产品应用填充 */
    uint16_t qid;                         /* queue id，0为admin queue，由KV驱动填充 */
    uint8_t rsvd[2];                      /* 预留字段，2byte */
    DRV_SGL *sgl;                         /* 指定读/写时使用的数据空间，由产品应用填充 */
    uint32_t data_len;                    /* 指定读/写时使用的数据长度，由产品应用填充 */
} scat_tgt_kv_cmd_s;

typedef union {
    scat_tgt_kv_cmd_s kv_cmd;
    uint8_t comm[144];                    /* 私有cmd的大小不能超过144byte */
} scat_private_cmd_u;

/* SCAT驱动与产品交互的CMD结构体 */
typedef struct {
    uint64_t cmd_sn;                     /* 命令sn，由产品应用填充 */
    scat_cmd_s scat_cmd;                 /* SCAT命令SQE，在接收到initiator命令时由KV驱动填充，64byte */
    void *lower;                         /* SCAT驱动IO资源指针，指向与此命令对应的SCAT驱动IO资源，由SCAT驱动填充 */
    void *upper;                         /* 需要时指向产品应用数据结构(产品私有)，由产品应用填充使用 */
    scat_private_cmd_u private_cmd;      /* 不同类型私有语义定制化CMD部分 */
    scat_error_code_e cmd_status;        /* SCAT驱动处理命令的状态，由SCAT驱动填充 */
    scat_cmd_result_u result;            /* SCAT命令处理结果，由产品应用填充 */
} scat_tgt_cmd_s;

typedef enum {
    DRV_SCAT_TABLE_OPERATE_SUCCESS = 0,  /* 表项操作执行成功 */
    DRV_SCAT_TABLE_OPERATE_FAILED,       /* 表项操作为FAIL */
    DRV_SCAT_TABLE_OPERATE_BUTT
} scat_table_operate_result_e;
 
typedef enum {
    DRV_SCAT_TABLE_TYPE_KVNS_MAP = 0,
    DRV_SCAT_TABLE_TYPE_KV_DISK_MAP,
    DRV_SCAT_TABLE_TYPE_ROOT_INFO,
    DRV_SCAT_TABLE_TYPE_BUTT
} scat_table_type_e;
 
typedef enum {
    DRV_SCAT_TABLE_OPCODE_INS = 0,      /* 插入 */
    DRV_SCAT_TABLE_OPCODE_DEL,          /* 指定删除 */
    DRV_SCAT_TABLE_OPCODE_DELALL,       /* 批量删除 */
    DRV_SCAT_TABLE_OPCODE_QUERY,        /* 查询，当前不支持该操作 */
    DRV_SCAT_TABLE_OPCODE_BUTT
} scat_table_opcode_e;
 
typedef struct {
    scat_table_type_e type;
    scat_table_opcode_e opcode;
    void *in_buf;                       /* 保存产品下发的表项数据 */
    uint32_t in_buf_len;                /* 下发的数据总长度 */
    void *out_buf;                      /* 当需要查询数据时，保存网卡返回的数据，需要提前申请 */
    uint32_t out_buf_len;
} scat_table_param_s;

/* SCAT Target驱动注册给SCAT协议模块的接口模板 */
typedef struct {
    /* 协议类型 */
    uint32_t protocol_type;

    /*
     *  SCAT协议向前端传送响应或数据和响应的接口
     *  产品在解析命令后向SCAT驱动发送响应，读命令通过该接口返回数据和命令处理结果，写命令通过该接口返回命令处理结果
     *
     *  @param[in]  tgt_cmd 命令数据结构
     *  @param[out] 无
     *
     *  @retval 无
     *
     *  @note
     *  1.产品更新命令结构中的数据传输长度和命令处理结果，产品需要将数据页面散列表的地址挂到命令结构中
     *  2.接口由SCAT驱动实现，运行在产品调用的上下文环境中
     *
     *  @see scat_tgt_driver_s, scat_tgt_cmd_s
     */
    void (*send_resp)(scat_tgt_cmd_s *tgt_cmd);

    /*
     *  产品协议准备好数据空间后向前端请求数据
     *  产品在解析到指定写命令后，申请数据页面并挂接到命令结构中发送给SCAT驱动
     *
     *  @param[in]  tgt_cmd 命令数据结构
     *  @param[out] 无
     *
     *  @retval 无
     *
     *  @note
     *  1.产品更新命令结构中的数据传输长度
     *  2.接口由SCAT驱动实现，运行在产品调用的上下文环境中
     *
     *  @see scat_tgt_driver_s, scat_tgt_cmd_s
     */
    void (*xfer_ready)(scat_tgt_cmd_s *tgt_cmd);

    /*
     * 获取端口对应的numa分区
     * TGT调用驱动接口查询当前端口绑定的numa分区
     *
     *  @param[in]  uint32_t 物理端口号
     *  @param[out] DRV_PORT_CPUGROUP_INFO
     *
     *  @retval 0: success
     *  @retval 非0: failed
     *
     *  @note
     *
     *  @see DRV_PORT_CPUGROUP_INFO
     */
    int32_t (*get_port_bind_cpu_group)(uint32_t port_id, DRV_PORT_CPUGROUP_INFO *cpu_info);

    /*
     * 下发表项数据
     * TGT调用驱动接口配置kvns_map、disk_map等表项数据
     *
     *  @param[in]  uint32_t 物理端口号
     *  @param[in] scat_table_param_s
     *
     *  @retval 0: success
     *  @retval 非0: failed
     *
     *  @note
     *  1.单线程调用，不支持多线程
     *  2.支持重复调用
     *  3.当前不支持查询表项
     *
     *  @see scat_table_param_s
     */
    uint32_t (*set_table)(uint32_t port_id, scat_table_param_s *param);
} scat_tgt_driver_s;

/* SCAT协议模块注册给SCAT驱动的接口模板 */
typedef struct {
    /* 协议类型 */
    uint32_t protocol_type;

    /*
     *  SCAT驱动向产品注册SCAT设备驱动接口
     *
     *  @param[in]  driver SCAT Target设备驱动接口模板
     *  @param[out] 无
     *
     *  @retval 0    成功
     *  @retval 非0  非成功时的返回码
     *
     *  @note
     *  1.接口由产品实现，1)在scat_register_intf接口上下文环境中运行，2)在SCAT驱动初始化调用上下文中运行
     *  2.接口一次注册一种SCAT设备所有接口，不支持多次调用分别注册同一种SCAT设备的不同接口
     *
     *  @see scat_drv_interface_s, scat_tgt_driver_s
     */
    int32_t (*add_scat_tgt_driver)(const scat_tgt_driver_s *driver);

    /*
     *  SCAT驱动从产品注销SCAT设备驱动接口
     *
     *  @param[in]  driver SCAT设备驱动接口模板
     *  @param[out] 无
     *
     *  @retval 无
     *
     *  @note
     *  1.接口由产品实现，在SCAT驱动移除调用上下文中运行
     *  2.接口一次注销一种SCAT设备所有接口，不支持多次调用分别注销同一种SCAT设备的不同接口
     *
     *  @see scat_drv_interface_s, scat_tgt_driver_s
     */
    int32_t (*remove_scat_tgt_driver)(const scat_tgt_driver_s *driver);

    /*
     *  SCAT驱动向产品注册SCAT Target设备端口
     *
     *  @param[in]  port_info     SCAT Target设备端口标识
     *  @param[in]  driver        SCAT Target设备驱动接口模板
     *  @param[out] 无
     *
     *  @retval 成功返回产品生成的端口信息指针，SCAT驱动不使用，在其他接口需要时SCAT驱动将原值传回产品，失败返回NULL
     *
     *  @note
     *  1.SCAT Target驱动对支持的每一个端口都需要调用一次本接口向产品注册端口
     *  2.接口由产品实现，在SCAT驱动调用上下文中运行，一般为线程环境
     *
     *  @see scat_drv_interface_s, scat_tgt_driver_s
     */
    void *(*add_scat_tgt_port)(const scat_target_port_info_s *port_info, const scat_tgt_driver_s *driver);

    /*
     *  SCAT驱动从产品注销SCAT Target设备端口，与该SCAT_HOST相关的资源由MTGT一起释放
     *
     *  @param[in]  scat_tgt_port 注册端口接口返回的产品生成的端口信息指针，接口返回后SCAT Target驱动不能再使用本指针
     *  @param[out] 无
     *
     *  @retval 0    成功
     *  @retval 非0  非成功时的返回码
     *
     *  @note
     *  1.SCAT Target驱动对支持的每一个端口都需要调用一次本接口从产品注销端口
     *  2.接口由产品实现，在SCAT驱动调用上下文中运行，一般为线程环境
     *
     *  @see scat_drv_interface_s
     */
    int32_t (*remove_scat_tgt_port)(const void *scat_tgt_port);

    /*
     *  SCAT驱动与initiator之间建立连接后，SCAT驱动向产品注册一个initiator连接会话，后续的IO会与此会话关联
     *
     *  @param[in]  scat_tgt_port 注册端口接口返回的产品生成的端口信息指针
     *  @param[in]  session_info SCAT驱动与initiator之间的连接信息，产品不能依赖该指针
     *  @param[out] 无
     *
     *  @retval 0    成功
     *  @retval 非0  非成功时的返回码
     *
     *  @note
     *  1.SCAT Target驱动对支持的每一个端口都需要调用一次本接口从产品注销端口
     *  2.接口由产品实现，在SCAT驱动调用上下文中运行，一般为线程环境
     *
     *  @see scat_tgt_driver_s, scat_session_info_s
     */
    void *(*add_scat_ini_session)(const void *scat_tgt_port, const scat_session_info_s *session_info);

    /*
     *  SCAT驱动与initiator之间的连接断开后，SCAT驱动从产品移除一个initiator连接会话
     *
     *  @param[in]  session 注册端口接口返回的产品生成的端口信息指针
     *  @param[out] 无
     *
     *  @retval 0    成功
     *  @retval 非0  非成功时的返回码
     *
     *  @note
     *  1.本接口调用返回后，所有SCAT驱动已经向产品申请且IO还在产品协议侧的CMD需返回resp给SCAT驱动
     *  2.接口由产品实现，在SCAT驱动调用上下文中运行，一般为线程环境
     *
     *  @see scat_tgt_driver_s
     */
    int32_t (*remove_scat_ini_session)(void *session);

    /*
     *  SCAT驱动通知产品协议删除会话完成驱动资源回收后，通知产品协议释放会话资源
     *
     *  @param[in]  session 注册端口接口返回的产品生成的端口信息指针
     *  @param[out] 无
     *
     *  @retval 0    成功
     *  @retval 非0  非成功时的返回码
     *
     *  @note
     *  1.本接口调用返回后，产品协议释放session相关资源
     *  2.接口由产品实现，在SCAT驱动调用上下文中运行，一般为线程环境
     *
     *  @see scat_tgt_driver_s
     */
    int32_t (*release_scat_ini_session)(void *session);

    /*
     *  SCAT驱动收到一个SCAT命令后，向产品申请一个交互命令数据结构
     *
     *  @param[in]  session   注册session接口返回的产品生成的连接信息指针
     *  @param[out] scat_cmd_cqe  申请命令失败时需要返回给Initiator的命令处理结果，包括返回码和sense信息
     *
     *  @retval 成功返回产品生成的命令数据结构指针，失败返回NULL
     *
     *  @note
     *  1.接口由产品实现，在SCAT驱动调用上下文中运行
     *
     *  @see scat_drv_interface_s, scat_cmd_cqe_s
     */
    scat_tgt_cmd_s *(*get_tgt_cmd)(void *session, scat_cmd_cqe_s *scat_cmd_cqe);

    /*
     *  SCAT驱动将SCAT命令发给产品解析处理
     *
     *  @param[in]  tgt_cmd  get_tgt_cmd接口返回的命令数据结构指针
     *  @param[out] 无
     *
     *  @retval 无
     *
     *  @note
     *  1.接口由产品实现，在SCAT驱动调用上下文中运行
     *
     *  @see scat_drv_interface_s, scat_tgt_cmd_s
     */
    void (*parse_tgt_cmd)(scat_tgt_cmd_s *tgt_cmd);

    /*
     *  SCAT驱动在数据接收完成后，通知产品处理数据
     *
     *  @param[in]  tgtCmd  产品申请数据空间后的命令数据结构指针
     *  @param[out] 无
     *
     *  @retval 无
     *
     *  @note
     *  1.接口由产品实现，在SCAT驱动调用上下文中运行
     *
     *  @see scat_drv_interface_s, scat_tgt_cmd_s
     */
    void (*rx_data)(scat_tgt_cmd_s *tgt_cmd);

    /*
     *  SCAT驱动在正常完成命令处理或出错命令处理后通知产品释放资源
     *
     *  @param[in]  tgtCmd  命令数据结构指针
     *  @param[out] 无
     *
     *  @retval 无
     *
     *  @note
     *  1.接口调用后，SCAT驱动和产品各自释放资源
     *  2.接口由产品实现，在SCAT驱动调用上下文中运行
     *
     *  @see scat_drv_interface_s, scat_tgt_cmd_s
     */
    void (*tgt_cmd_done)(scat_tgt_cmd_s *tgt_cmd);

    /*
     *  SCAT驱动在命令处理期间发生错误时从产品应用查询需要向Initiator返回的命令处理结果和sense信息
     *
     *  @param[in]  error_code  SCAT驱动出错信息
     *  @param[in]  cmd_cqe     SCAT驱动出错时输入对应的scat_cmd_cqe_s结构
     *  @param[in]  session     SCAT对应的session信息
     *  @param[out] cmd_cqe     产品填写CQE，包含需要向Initiator返回的命令处理结果和sense信息。
     *
     *  @retval 无
     *
     *  @note
     *  1.接口由产品实现，同步接口。
     *
     *  @see scat_drv_interface_s, scat_error_code_e, scat_cmd_cqe_s
     */
    void (*get_sense)(scat_error_code_e error_code, scat_cmd_cqe_s *cmd_cqe, void *session);
} scat_drv_interface_s;

/*
 *  产品协议向驱动注册接口的接口
 *
 *  @param[in]  scat_intf 产品接口模板指针
 *  @param[out] 无
 *
 *  @retval DRV_SCAT_RETURN_OK    成功
 *  @retval DRV_SCAT_RETURN_ERROR 失败
 *
 *  @note
 *
 *  @see scat_drv_interface_s
 */
int32_t scat_frame_register_intf(const scat_drv_interface_s *scat_intf);

/*
 *  产品注销提供给驱动的接口
 *
 *  @param[in]  scat_protocol_type_e 协议类型
 *  @param[out] 无
 *
 *  @retval DRV_SCAT_RETURN_OK    成功
 *  @retval DRV_SCAT_RETURN_ERROR 失败
 *
 *  @note
 *
 *  @see scat_drv_interface_s
 */
int32_t scat_frame_unregister_intf(const scat_protocol_type_e type);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif /* DRV_SCAT_TARGET_H */