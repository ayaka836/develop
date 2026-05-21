#include "kvprot_dt_context.h"

static int32_t kvprot_dt_memset_s(void *dest, size_t destsz, int ch, size_t count)
{
    if (dest == nullptr || count > destsz) {
        return RETURN_ERROR;
    }
    (void)memset(dest, ch, count);
    return RETURN_OK;
}

static int32_t kvprot_dt_memcpy_s(void *dest, size_t destsz, const void *src, size_t count)
{
    if (dest == nullptr || src == nullptr || count > destsz) {
        return RETURN_ERROR;
    }
    (void)memcpy(dest, src, count);
    return RETURN_OK;
}

static void kvprot_dt_log_info(int log_id, const char *fmt, ...)
{
    (void)log_id;
    (void)fmt;
}

static void kvprot_dt_log_error(int log_id, const char *fmt, ...)
{
    (void)log_id;
    (void)fmt;
}

static void kvprot_dt_spin_lock_init(spinlock_t *lock)
{
    (void)lock;
}

static void kvprot_dt_spin_lock(spinlock_t *lock)
{
    (void)lock;
}

static void kvprot_dt_spin_unlock(spinlock_t *lock)
{
    (void)lock;
}

static int32_t kvprot_dt_scat_frame_register_intf(const scat_drv_interface_s *intf)
{
    g_dt.scat_register_count++;
    if (intf != nullptr) {
        (void)memcpy(&g_dt.scat_intf, intf, sizeof(g_dt.scat_intf));
    }
    return g_dt.scat_register_ret;
}

static void kvprot_dt_scat_frame_unregister_intf(uint32_t protocol_type)
{
    g_dt.scat_unregister_count++;
    g_dt.scat_unregister_protocol = protocol_type;
}

static int32_t kvprot_dt_create_partition(uint32_t max_count, uint32_t slab_count, const char *name,
    uint32_t pid, uint32_t attr, size_t elem_size, uint32_t *part_id, uint32_t context)
{
    (void)max_count;
    (void)slab_count;
    (void)name;
    (void)pid;
    (void)attr;
    (void)elem_size;
    (void)context;

    g_dt.create_partition_count++;
    if (part_id != nullptr) {
        *part_id = g_dt.created_part_id;
    }
    return g_dt.create_partition_ret;
}

static void kvprot_dt_delete_partition(uint32_t part_id)
{
    (void)part_id;
    g_dt.delete_partition_count++;
}

static void kvprot_dt_allocate_structure(uint32_t pid, uint32_t part_id, void **out)
{
    (void)pid;
    (void)part_id;

    g_dt.allocate_structure_count++;
    if (out == nullptr) {
        return;
    }
    if (g_dt.fail_structure_alloc_call != 0 &&
        g_dt.fail_structure_alloc_call == g_dt.allocate_structure_count) {
        *out = nullptr;
        return;
    }
    *out = calloc(1, sizeof(KVPROT_CMD));
}

static void kvprot_dt_free_structure(void *ptr, uint32_t part_id)
{
    (void)part_id;
    g_dt.free_structure_count++;
    free(ptr);
}

static void *kvprot_dt_alloc_page(uint32_t pid, const char *func, uint32_t line)
{
    (void)pid;
    (void)func;
    (void)line;

    g_dt.allocate_page_count++;
    if (g_dt.fail_page_alloc_call != 0 &&
        g_dt.fail_page_alloc_call == g_dt.allocate_page_count) {
        return nullptr;
    }
    return calloc(1, sizeof(KVPROT_DT_PAGE));
}

static void *kvprot_dt_get_page_addr(void *page_ctrl)
{
    return (page_ctrl == nullptr) ? nullptr : ((KVPROT_DT_PAGE *)page_ctrl)->data;
}

static void kvprot_dt_free_page(void *page_ctrl, const char *func, uint32_t line)
{
    (void)func;
    (void)line;
    g_dt.free_page_count++;
    free(page_ctrl);
}

static uint32_t kvprot_dt_vc_get_vcid(void)
{
    return g_dt.vc_id;
}

static void kvprot_dt_dplwt_attr_init(dplwt_attr_s *attr)
{
    (void)attr;
}

static void kvprot_dt_dplwt_dispatch_by_partition_id_set(dplwt_attr_s *attr, uint32_t part_id)
{
    (void)attr;
    g_dt.dplwt_attr_part_id = part_id;
}

static int32_t kvprot_dt_dplwt_get_scpart_from_group(uint32_t vc_id, const char *part_name,
    dplwt_scpart_t **part)
{
    (void)vc_id;
    (void)part_name;

    g_dt.dplwt_get_count++;
    if (part != nullptr) {
        *part = (g_dt.dplwt_get_ret == RETURN_OK) ? &g_dt.normal_part : nullptr;
    }
    return g_dt.dplwt_get_ret;
}

static int32_t kvprot_dt_dplwt_create(dplwt_id_t *id, dplwt_entry_func entry_func, void *arg,
    const dplwt_attr_s *attr)
{
    (void)attr;

    g_dt.dplwt_create_count++;
    if (id != nullptr) {
        *id = (dplwt_id_t)g_dt.dplwt_create_count;
    }
    if (g_dt.dplwt_create_ret != RETURN_OK) {
        return g_dt.dplwt_create_ret;
    }
    if (g_dt.dplwt_run_inline && entry_func != nullptr) {
        entry_func(arg);
    }
    return RETURN_OK;
}

static int kvprot_dt_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start_routine)(void *), void *arg)
{
    (void)attr;
    (void)start_routine;
    (void)arg;

    g_dt.pthread_create_count++;
    if (thread != nullptr) {
        *thread = g_dt_fake_thread;
    }
    return g_dt.pthread_create_ret;
}

static int kvprot_dt_pthread_join(pthread_t thread, void **retval)
{
    (void)thread;
    if (retval != nullptr) {
        *retval = nullptr;
    }
    g_dt.pthread_join_count++;
    return RETURN_OK;
}

static int kvprot_dt_usleep(unsigned int usec)
{
    (void)usec;
    g_dt.usleep_count++;
    return RETURN_OK;
}

#undef memset_s
#undef memcpy_s
#undef DBG_LogInfo
#undef DBG_LogError
#undef spin_lock_init
#undef spin_lock
#undef spin_unlock
#undef scat_frame_register_intf
#undef scat_frame_unregister_intf
#undef CREATE_EXPANDABLE_STRUCTURE_PARTITION
#undef DELETE_MEMORY_PARTITION
#undef ALLOCATE_STRUCTURE
#undef FREE_STRUCTURE
#undef ALLOCATE_ONE_PAGE_SYNC
#undef GET_PAGE_ADDR
#undef FREE_ONE_PAGE
#undef vc_get_vcid
#undef DPLWT_ATTR_INIT
#undef DPLWT_DISPATCH_BY_PARTITION_ID_SET
#undef DPLWT_GET_SCPART_FROM_GROUP
#undef DPLWT_CREATE
#undef pthread_create
#undef pthread_join
#undef usleep

#define memset_s kvprot_dt_memset_s
#define memcpy_s kvprot_dt_memcpy_s
#define DBG_LogInfo kvprot_dt_log_info
#define DBG_LogError kvprot_dt_log_error
#define spin_lock_init kvprot_dt_spin_lock_init
#define spin_lock kvprot_dt_spin_lock
#define spin_unlock kvprot_dt_spin_unlock
#define scat_frame_register_intf kvprot_dt_scat_frame_register_intf
#define scat_frame_unregister_intf kvprot_dt_scat_frame_unregister_intf
#define CREATE_EXPANDABLE_STRUCTURE_PARTITION(max_count, slab_count, name, pid, attr, elem_size, part_id, context) \
    kvprot_dt_create_partition((max_count), (slab_count), (name), (pid), (attr), (elem_size), (part_id), (context))
#define DELETE_MEMORY_PARTITION(part_id) kvprot_dt_delete_partition((part_id))
#define ALLOCATE_STRUCTURE(pid, part_id, out) kvprot_dt_allocate_structure((pid), (part_id), (out))
#define FREE_STRUCTURE(ptr, part_id) kvprot_dt_free_structure((ptr), (part_id))
#define ALLOCATE_ONE_PAGE_SYNC(pid, func, line) kvprot_dt_alloc_page((pid), (func), (line))
#define GET_PAGE_ADDR(page_ctrl) kvprot_dt_get_page_addr((page_ctrl))
#define FREE_ONE_PAGE(page_ctrl, func, line) kvprot_dt_free_page((page_ctrl), (func), (line))
#define vc_get_vcid kvprot_dt_vc_get_vcid
#define DPLWT_ATTR_INIT(attr) kvprot_dt_dplwt_attr_init((attr))
#define DPLWT_DISPATCH_BY_PARTITION_ID_SET(attr, part_id) \
    kvprot_dt_dplwt_dispatch_by_partition_id_set((attr), (part_id))
#define DPLWT_GET_SCPART_FROM_GROUP(vc_id, part_name, out) \
    kvprot_dt_dplwt_get_scpart_from_group((vc_id), (part_name), (out))
#define DPLWT_CREATE(id, entry_func, arg, attr) kvprot_dt_dplwt_create((id), (entry_func), (arg), (attr))
#define pthread_create kvprot_dt_pthread_create
#define pthread_join kvprot_dt_pthread_join
#define usleep kvprot_dt_usleep
