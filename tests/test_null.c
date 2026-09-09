/*
 * Null-safety vectors.
 *
 * These cover the malformed-input paths hardened in the null-safety pass: a
 * .savepatch that omits a delimiter the parser expects, runs out of lines in
 * the middle of a multi-line code, or names a variable that was never set.
 * Every case must be REJECTED cleanly (return 0, save buffer untouched)
 * instead of dereferencing NULL or overflowing a heap block.
 */
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

static size_t apply_sw(uint8_t* buf, size_t len, const char* codes)
{
    code_entry_t c = make_sw_code(codes);
    return apollo_apply_sw_code(buf, len, &c);
}

static size_t apply_bsd(uint8_t** buf, size_t len, const char* codes)
{
    apollo_free_var_list();
    code_entry_t c = make_bsd_code(codes);
    return apollo_apply_bsd_code(buf, len, &c);
}

/* ---- Save Wizard: truncated multi-line codes ---- */

/* type 4 (multi-write) without its second line -> rejected, buffer untouched */
TEST(null_sw_multiwrite_missing_second_line)
{
    uint8_t buf[16] = {0};
    size_t n = apply_sw(buf, sizeof(buf), "40000000 11223344");

    uint8_t exp[16] = {0};
    CHECK_U64("truncated multi-write rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, exp, sizeof(buf));
}

/* type 5 (copy) without its destination line -> rejected */
TEST(null_sw_copy_missing_second_line)
{
    uint8_t buf[16] = {0};
    buf[0] = 0xDE; buf[1] = 0xAD;
    size_t n = apply_sw(buf, sizeof(buf), "50000000 00000004");

    uint8_t exp[16] = {0};
    exp[0] = 0xDE; exp[1] = 0xAD;
    CHECK_U64("truncated copy rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, exp, sizeof(buf));
}

/* type A (bulk write) with fewer data lines than the declared size -> rejected */
TEST(null_sw_bulk_write_missing_data)
{
    uint8_t buf[32] = {0};
    size_t n = apply_sw(buf, sizeof(buf), "A0000000 00000010\n11223344 55667788");

    uint8_t exp[32] = {0};
    CHECK_U64("truncated bulk write rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, exp, sizeof(buf));
}

/* type 8 search declaring a longer pattern than it provides -> rejected */
TEST(null_sw_search_pattern_truncated)
{
    uint8_t buf[16] = {0};
    size_t n = apply_sw(buf, sizeof(buf), "80000010 11223344");

    CHECK_U64("truncated search pattern rejected", n, 0);
}

/* type 8 search with a zero-length pattern (malloc(0) + 4-byte write) -> rejected */
TEST(null_sw_search_zero_length)
{
    uint8_t buf[16] = {0};
    size_t n = apply_sw(buf, sizeof(buf), "80000000 11223344");

    CHECK_U64("empty search pattern rejected", n, 0);
}

/* a short line can't be parsed at the fixed offsets: skipped, rest still runs */
TEST(null_sw_short_line_skipped)
{
    uint8_t buf[16] = {0};
    size_t n = apply_sw(buf, sizeof(buf), "20\n20000000 000000AA");

    CHECK_U64("code still applied", n, sizeof(buf));
#if APOLLO_TEST_ENDIAN_BE
    CHECK_U64("write performed", buf[3], 0xAA);
#else
    CHECK_U64("write performed", buf[0], 0xAA);
#endif
}

/* ---- BSD: missing delimiters ---- */

/* blowfish_cbc() takes key,iv — a single argument used to NULL-deref */
TEST(null_bsd_blowfish_cbc_missing_iv)
{
    uint8_t init[16] = {0};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "decrypt blowfish_cbc(00112233445566778899AABBCCDDEEFF)");

    CHECK_U64("missing IV rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, init, sizeof(init));
    free(buf);
}

/* aes_cbc(key,iv) whose trailing text hides the ')' from the reverse scan */
TEST(null_bsd_aes_cbc_trailing_comma)
{
    uint8_t init[32] = {0};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init),
                         "decrypt aes_cbc(000102030405060708090A0B0C0D0E0F,000102030405060708090A0B0C0D0E0F) x,y");

    CHECK_U64("malformed aes_cbc rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, init, sizeof(init));
    free(buf);
}

/* reading a variable that was never set must not decode NULL data */
TEST(null_bsd_write_unset_variable)
{
    uint8_t init[16] = {0};
    init[0] = 0x11;
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "write at 0:[never_set]");

    CHECK_U64("unset variable rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, init, sizeof(init));
    free(buf);
}

/* mid() slicing past the end of the decoded value -> rejected */
TEST(null_bsd_mid_slice_out_of_range)
{
    uint8_t init[16] = {0};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "set [v]:mid(AABBCCDD,3,8)\nwrite at 0:[v]");

    CHECK_U64("out-of-range mid() rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, init, sizeof(init));
    free(buf);
}

/* an in-range mid() still works (the guard must not reject valid slices) */
TEST(null_bsd_mid_slice_in_range)
{
    uint8_t init[16] = {0};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "set [v]:mid(AABBCCDD,1,2)\nwrite at 0:[v]");

    uint8_t exp[16] = {0};
    exp[0] = 0xBB; exp[1] = 0xCC;
    CHECK_U64("valid mid() applied", n, sizeof(init));
    CHECK_MEM("slice written", buf, exp, sizeof(exp));
    free(buf);
}

/* repeat() with a negative count used to reach malloc() with a huge size */
TEST(null_bsd_repeat_negative_count)
{
    uint8_t init[16] = {0};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "write at 0:repeat(-4,AA)");

    CHECK_U64("negative repeat rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, init, sizeof(init));
    free(buf);
}

/* xor-writing more bytes than the buffer holds must not read past it */
TEST(null_bsd_xor_write_out_of_bounds)
{
    uint8_t init[4] = {1, 2, 3, 4};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "write at 2:xor:AABBCCDD");

    CHECK_U64("OOB xor write rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, init, sizeof(init));
    free(buf);
}

/* a bitwise op against a variable of a different length is rejected, not applied */
TEST(null_bsd_bitwise_length_mismatch)
{
    uint8_t init[16] = {0};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "set [v]:AABB\nset [v]:xor:11223344\nwrite at 0:[v]");

    CHECK_U64("length mismatch rejected", n, 0);
    CHECK_MEM("buffer untouched", buf, init, sizeof(init));
    free(buf);
}

/* re-setting an existing variable with a hash keeps the engine consistent:
 * the old value must not survive as a pointer into the caller's stack */
TEST(null_bsd_var_overwrite_after_use)
{
    uint8_t init[16] = {0};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init),
                         "set [v]:AABBCCDD\nset range:0,3\nset [v]:crc32\nwrite at 0:[v]");

    CHECK_U64("code applied", n, sizeof(init));
    apollo_free_var_list();   /* must not free a stack pointer */
    free(buf);
}
