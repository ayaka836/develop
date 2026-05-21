#include "kvprot_dt_fixture.h"

static int32_t kvprot_dt_init_ok_a(void);
static int32_t kvprot_dt_init_ok_b(void);
static int32_t kvprot_dt_init_fail(void);
static void kvprot_dt_exit_a(void);
static void kvprot_dt_exit_b(void);
static int g_dt_order[16];
static uint32_t g_dt_order_len;

static void kvprot_dt_order_push(int value)
{
    if (g_dt_order_len < (sizeof(g_dt_order) / sizeof(g_dt_order[0]))) {
        g_dt_order[g_dt_order_len++] = value;
    }
}

static int32_t kvprot_dt_init_ok_a(void)
{
    kvprot_dt_order_push(1);
    return RETURN_OK;
}

static int32_t kvprot_dt_init_ok_b(void)
{
    kvprot_dt_order_push(2);
    return RETURN_OK;
}

static int32_t kvprot_dt_init_fail(void)
{
    kvprot_dt_order_push(3);
    return -77;
}

static void kvprot_dt_exit_a(void)
{
    kvprot_dt_order_push(11);
}

static void kvprot_dt_exit_b(void)
{
    kvprot_dt_order_push(12);
}

TEST_F(KvprotDtTest, CommonInitOpsSuccessFailureAndExitOrder)
{
    KVPROT_INIT_OPS success_ops[] = {
        {"skip", nullptr, kvprot_dt_exit_a},
        {"ok_a", kvprot_dt_init_ok_a, kvprot_dt_exit_a},
        {"ok_b", kvprot_dt_init_ok_b, kvprot_dt_exit_b},
    };
    KVPROT_INIT_OPS fail_ops[] = {
        {"ok_a", kvprot_dt_init_ok_a, kvprot_dt_exit_a},
        {"fail", kvprot_dt_init_fail, kvprot_dt_exit_b},
        {"ok_b", kvprot_dt_init_ok_b, kvprot_dt_exit_b},
    };

    g_dt_order_len = 0;
    EXPECT_EQ(RETURN_OK, KvprotInitOps(success_ops, (uint32_t)ARRAY_LEN(success_ops), "dt"));
    EXPECT_EQ(2U, g_dt_order_len);
    EXPECT_EQ(1, g_dt_order[0]);
    EXPECT_EQ(2, g_dt_order[1]);

    g_dt_order_len = 0;
    EXPECT_EQ(-77, KvprotInitOps(fail_ops, (uint32_t)ARRAY_LEN(fail_ops), "dt"));
    EXPECT_EQ(3U, g_dt_order_len);
    EXPECT_EQ(1, g_dt_order[0]);
    EXPECT_EQ(3, g_dt_order[1]);
    EXPECT_EQ(11, g_dt_order[2]);

    g_dt_order_len = 0;
    KvprotExitOps(success_ops, (uint32_t)ARRAY_LEN(success_ops), "dt");
    EXPECT_EQ(3U, g_dt_order_len);
    EXPECT_EQ(12, g_dt_order[0]);
    EXPECT_EQ(11, g_dt_order[1]);
    EXPECT_EQ(11, g_dt_order[2]);

    EXPECT_EQ(RETURN_ERROR, KvprotInitOps(nullptr, 1, "dt"));
    KvprotExitOps(nullptr, 1, "dt");
}
