/*
 * Bounds-safety vectors.
 *
 * These guard the offset/length validation added to apply_sw_patch_code and
 * apply_bsd_patch_code: an out-of-range access driven by a .savepatch must be
 * SKIPPED (buffer left untouched, no crash), while an in-bounds access — right
 * up to the exact end of the buffer — must still be performed. Before the
 * bounds checks these OOB cases corrupted the heap or crashed.
 */
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

static void apply_sw(uint8_t* buf, size_t len, const char* codes)
{
    code_entry_t c = make_sw_code(codes);
    apply_sw_patch_code(buf, len, &c);
}

static size_t apply_bsd(uint8_t** buf, size_t len, const char* codes)
{
    free_patch_var_list();
    code_entry_t c = make_bsd_code(codes);
    return apply_bsd_patch_code(buf, len, &c);
}

/* SW 32-bit write far past the end -> skipped, buffer untouched */
TEST(bounds_sw_write_oob_skipped)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "20FFFFFF 12345678");

    uint8_t exp[16] = {0};
    CHECK_MEM("OOB 32-bit write skipped", buf, exp, sizeof(buf));
}

/* SW 32-bit write straddling the end (off+4 > dsize) -> skipped */
TEST(bounds_sw_write_straddle_end_skipped)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "2000000E 12345678");   /* off 14 + 4 = 18 > 16 */

    uint8_t exp[16] = {0};
    CHECK_MEM("straddling write skipped", buf, exp, sizeof(buf));
}

/* SW 32-bit write ending exactly at the buffer end (off+4 == dsize) -> allowed */
TEST(bounds_sw_write_at_edge_ok)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "2000000C 12345678");   /* off 12 + 4 = 16 == 16 */

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    exp[12]=0x12; exp[13]=0x34; exp[14]=0x56; exp[15]=0x78;
#else
    exp[12]=0x78; exp[13]=0x56; exp[14]=0x34; exp[15]=0x12;
#endif
    CHECK_MEM("edge write performed", buf, exp, sizeof(buf));
}

/* SW copy (type 5) with OOB source/dest -> skipped */
TEST(bounds_sw_copy_oob_skipped)
{
    uint8_t buf[16] = {0};
    buf[0]=0xDE; buf[1]=0xAD;
    apply_sw(buf, sizeof(buf), "50000000 00000004\n50FFFFF0 00000000");  /* dst OOB */

    uint8_t exp[16] = {0};
    exp[0]=0xDE; exp[1]=0xAD;
    CHECK_MEM("OOB copy skipped", buf, exp, sizeof(buf));
}

/* BSD write past the end -> skipped, size unchanged */
TEST(bounds_bsd_write_oob_skipped)
{
    uint8_t init[16] = {0};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "write at 0xFFFFFF:AABBCCDD");

    uint8_t exp[16] = {0};
    CHECK_U64("bsd OOB write: size kept", n, sizeof(init));
    CHECK_MEM("bsd OOB write skipped", buf, exp, sizeof(exp));
    free(buf);
}

/* BSD read() past the end -> code aborts cleanly (returns 0), buffer untouched */
TEST(bounds_bsd_read_oob_aborts)
{
    uint8_t init[16] = {0};
    init[0]=0xAA; init[1]=0xBB;
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "set [v]:read(0xFFFFFF,4)\nwrite at 0:[v]");

    uint8_t exp[16] = {0};
    exp[0]=0xAA; exp[1]=0xBB;
    CHECK_U64("bsd OOB read: aborts (returns 0)", n, 0);
    CHECK_MEM("bsd OOB read: buffer untouched", buf, exp, sizeof(exp));
    free(buf);
}
