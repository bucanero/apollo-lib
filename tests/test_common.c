#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "test_common.h"

/* ------------------------------------------------------------------ */
/* Library-required stubs                                             */
/* ------------------------------------------------------------------ */

/* patches.c / decrypt.c call dbglogger_log(). Silent unless verbose. */
static int g_log_enabled = -1;

void dbglogger_log(const char* fmt, ...)
{
    if (g_log_enabled < 0)
        g_log_enabled = (getenv("APOLLO_TEST_VERBOSE") != NULL);

    if (!g_log_enabled)
        return;

    char buf[0x800];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fprintf(stderr, "- %s\n", buf);
}

/*
 * Deterministic host callback for BSD codes that request host data
 * (account id, psid, user name, ...). Fixed values keep golden output stable.
 */
void* apollo_test_host_cb(int info, uint32_t* size)
{
    static const uint8_t account_id[8] = { 0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF };
    static const uint8_t psid[16] = {
        0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
        0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF
    };
    static const uint8_t mac[6] = { 0xDE,0xAD,0xBE,0xEF,0x00,0x01 };
    static char sys_name[]  = "APOLLO-TEST";
    static char user_name[] = "tester";
    static char temp_path[] = "/tmp/apollo-test/";
    static char data_path[] = "/tmp/apollo-test/data/";

    switch (info) {
        case APOLLO_HOST_ACCOUNT_ID: if (size) *size = sizeof(account_id); return (void*)account_id;
        case APOLLO_HOST_PSID:       if (size) *size = sizeof(psid);       return (void*)psid;
        case APOLLO_HOST_LAN_ADDR:
        case APOLLO_HOST_WLAN_ADDR:  if (size) *size = sizeof(mac);        return (void*)mac;
        case APOLLO_HOST_SYS_NAME:   if (size) *size = sizeof(sys_name)-1; return (void*)sys_name;
        case APOLLO_HOST_USERNAME:   if (size) *size = sizeof(user_name)-1;return (void*)user_name;
        case APOLLO_HOST_TEMP_PATH:  if (size) *size = sizeof(temp_path)-1;return (void*)temp_path;
        case APOLLO_HOST_DATA_PATH:  if (size) *size = sizeof(data_path)-1;return (void*)data_path;
        default:                     if (size) *size = 0;                  return (void*)sys_name;
    }
}

/* ------------------------------------------------------------------ */
/* Assertions                                                         */
/* ------------------------------------------------------------------ */

int g_checks_run = 0;
int g_checks_failed = 0;
int g_cur_test_failed = 0;

static void hexdump(const char* label, const uint8_t* buf, size_t len)
{
    fprintf(stderr, "      %s:", label);
    for (size_t i = 0; i < len; i++)
        fprintf(stderr, " %02X", buf[i]);
    fprintf(stderr, "\n");
}

int check_mem(const char* file, int line, const char* desc,
              const void* got, const void* exp, size_t len)
{
    g_checks_run++;
    if (memcmp(got, exp, len) == 0)
        return 1;

    g_checks_failed++;
    g_cur_test_failed++;
    fprintf(stderr, "  [FAIL] %s (%s:%d) [%s]\n", desc, file, line, APOLLO_TEST_ENDIAN_NAME);
    hexdump("expected", exp, len);
    hexdump("got     ", got, len);
    return 0;
}

int check_u64(const char* file, int line, const char* desc,
              uint64_t got, uint64_t exp)
{
    g_checks_run++;
    if (got == exp)
        return 1;

    g_checks_failed++;
    g_cur_test_failed++;
    fprintf(stderr, "  [FAIL] %s (%s:%d) [%s] expected=0x%llX got=0x%llX\n",
            desc, file, line, APOLLO_TEST_ENDIAN_NAME,
            (unsigned long long)exp, (unsigned long long)got);
    return 0;
}

int check_str(const char* file, int line, const char* desc,
              const char* got, const char* exp)
{
    g_checks_run++;
    if (got && exp && strcmp(got, exp) == 0)
        return 1;

    g_checks_failed++;
    g_cur_test_failed++;
    fprintf(stderr, "  [FAIL] %s (%s:%d) [%s]\n", desc, file, line, APOLLO_TEST_ENDIAN_NAME);
    fprintf(stderr, "      expected: \"%s\"\n", exp ? exp : "(null)");
    fprintf(stderr, "      got     : \"%s\"\n", got ? got : "(null)");
    return 0;
}

/* ------------------------------------------------------------------ */
/* Code builders                                                      */
/* ------------------------------------------------------------------ */

code_entry_t make_sw_code(const char* codes)
{
    code_entry_t c;
    memset(&c, 0, sizeof(c));
    c.type  = APOLLO_CODE_SAVEWIZARD;
#if APOLLO_TEST_ENDIAN_BE
    c.flags = APOLLO_CODE_FLAG_ORDER_BE;
#endif
    c.name  = (char*)"vector";
    c.file  = (char*)"vector";
    c.codes = (char*)codes;   /* apply_sw_patch_code strdup()s this */
    return c;
}

code_entry_t make_bsd_code(const char* codes)
{
    code_entry_t c = make_sw_code(codes);
    c.type = APOLLO_CODE_BSD;
#if APOLLO_TEST_ENDIAN_BE
    c.flags = APOLLO_CODE_FLAG_ORDER_BE;
#endif
    return c;
}

/* ------------------------------------------------------------------ */
/* Deterministic data helpers                                         */
/* ------------------------------------------------------------------ */

/* xorshift-based fill — no libc rand(), fully reproducible across platforms */
void fill_lcg(uint8_t* buf, size_t len, uint32_t seed)
{
    uint32_t x = seed ? seed : 0xA5A5A5A5u;
    for (size_t i = 0; i < len; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        buf[i] = (uint8_t)(x >> 24);
    }
}

uint64_t fnv1a(const uint8_t* buf, size_t len)
{
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < len; i++) {
        h ^= buf[i];
        h *= 1099511628211ull;
    }
    return h;
}

/* ------------------------------------------------------------------ */
/* Test registry                                                      */
/* ------------------------------------------------------------------ */

#define MAX_TESTS 256
static struct { const char* name; apollo_test_fn fn; } g_tests[MAX_TESTS];
static int g_test_count = 0;

void register_test(const char* name, apollo_test_fn fn)
{
    if (g_test_count < MAX_TESTS) {
        g_tests[g_test_count].name = name;
        g_tests[g_test_count].fn = fn;
        g_test_count++;
    }
}

int run_registered_tests(void)
{
    int failed_tests = 0;

    printf("=== apollo unit vectors [%s build] : %d tests ===\n",
           APOLLO_TEST_ENDIAN_NAME, g_test_count);

    for (int i = 0; i < g_test_count; i++) {
        g_cur_test_failed = 0;
        g_tests[i].fn();
        if (g_cur_test_failed) {
            failed_tests++;
            printf("  FAIL  %s (%d check(s))\n", g_tests[i].name, g_cur_test_failed);
        } else {
            printf("  ok    %s\n", g_tests[i].name);
        }
    }

    printf("--- [%s] %d checks, %d failed, %d/%d tests passed ---\n",
           APOLLO_TEST_ENDIAN_NAME, g_checks_run, g_checks_failed,
           g_test_count - failed_tests, g_test_count);

    return failed_tests ? 1 : 0;
}
