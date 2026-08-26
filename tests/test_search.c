/*
 * Search / conditional-skip vectors.
 *
 * Covers the pointer-setting search opcodes and the skip logic they drive:
 *   Save Wizard  type 8 (forward), type B (backward), type C (address-byte),
 *                type D (byte-test conditional skip)
 *   BSD          search command
 *
 * Search patterns use the unconditional BE* macros and the results are byte
 * moves / 8-bit writes, so every case here is endian-flag INVARIANT — one
 * shared expected array, which also asserts LE and BE agree.
 *
 * Two different "not found" behaviors are pinned deliberately:
 *   - Save Wizard: a failed ABSOLUTE search skips all following lines until the
 *     next absolute search (8/B/C with T!=8) or EOF, then resumes.
 *   - BSD: a failed search aborts the whole code (returns 0, file untouched).
 */
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

static void apply_sw(uint8_t* buf, size_t len, const char* codes)
{
    code_entry_t c = make_sw_code(codes);
    apollo_apply_sw_code(buf, len, &c);
}

static size_t apply_bsd(uint8_t** buf, size_t len, const char* codes)
{
    apollo_free_var_list();
    code_entry_t c = make_bsd_code(codes);
    return apollo_apply_bsd_code(buf, len, &c);
}

/* ======================= Save Wizard type 8 (forward) ==================== */

/* found -> pointer at first occurrence, pointer-relative write lands there */
TEST(sw8_forward_found)
{
    uint8_t buf[16] = {0};
    memcpy(buf + 6, "TEST", 4);
    apply_sw(buf, sizeof(buf), "80010004 54455354\n08000000 000000AB");

    uint8_t exp[16] = {0};
    memcpy(exp + 6, "TEST", 4);
    exp[6] = 0xAB;
    CHECK_MEM("t8 found -> write @ptr", buf, exp, sizeof(buf));
}

/* not found -> the following write is skipped, buffer untouched */
TEST(sw8_forward_not_found_skips)
{
    uint8_t buf[16] = {0};   /* no "TEST" present */
    apply_sw(buf, sizeof(buf), "80010004 54455354\n00000005 000000AB");

    uint8_t exp[16] = {0};
    CHECK_MEM("t8 not-found -> write skipped", buf, exp, sizeof(buf));
}

/* not found -> skip lines until the NEXT absolute search, then resume */
TEST(sw8_forward_skip_resumes_at_next_search)
{
    uint8_t buf[16] = {0};
    memcpy(buf + 10, "DATA", 4);
    apply_sw(buf, sizeof(buf),
             "80010004 54455354\n"   /* search TEST (absent) -> enter skip   */
             "00000005 000000AB\n"   /* skipped                              */
             "80010004 44415441\n"   /* search DATA (present @10) -> resume  */
             "08000000 000000CD");   /* write @ptr                           */

    uint8_t exp[16] = {0};
    memcpy(exp + 10, "DATA", 4);
    exp[10] = 0xCD;
    CHECK_MEM("t8 skip then resume at next search", buf, exp, sizeof(buf));
}

/* occurrence count -> pointer at the Nth match */
TEST(sw8_forward_count_second)
{
    uint8_t buf[16] = {0};
    buf[2] = 0x41; buf[3] = 0x42;    /* "AB" #1 */
    buf[8] = 0x41; buf[9] = 0x42;    /* "AB" #2 */
    apply_sw(buf, sizeof(buf), "80020002 41420000\n08000000 000000CD");

    uint8_t exp[16] = {0};
    exp[2] = 0x41; exp[3] = 0x42;
    exp[8] = 0xCD; exp[9] = 0x42;    /* 2nd occurrence hit */
    CHECK_MEM("t8 count=2 -> 2nd occurrence", buf, exp, sizeof(buf));
}

/* ======================= Save Wizard type B (backward) =================== */

/* backward search from EOF finds the LAST occurrence */
TEST(swB_backward_found_last)
{
    uint8_t buf[16] = {0};
    memcpy(buf + 4,  "END!", 4);
    memcpy(buf + 10, "END!", 4);
    apply_sw(buf, sizeof(buf), "B0010004 454E4421\n08000000 000000AB");

    uint8_t exp[16] = {0};
    memcpy(exp + 4,  "END!", 4);
    memcpy(exp + 10, "END!", 4);
    exp[10] = 0xAB;                  /* last occurrence */
    CHECK_MEM("tB backward -> last occurrence", buf, exp, sizeof(buf));
}

/* backward count=2 -> the second-from-end occurrence */
TEST(swB_backward_count_second)
{
    uint8_t buf[16] = {0};
    memcpy(buf + 4,  "END!", 4);
    memcpy(buf + 10, "END!", 4);
    apply_sw(buf, sizeof(buf), "B0020004 454E4421\n08000000 000000CD");

    uint8_t exp[16] = {0};
    memcpy(exp + 4,  "END!", 4);
    memcpy(exp + 10, "END!", 4);
    exp[4] = 0xCD;                   /* 2nd from end */
    CHECK_MEM("tB backward count=2", buf, exp, sizeof(buf));
}

/* backward not found -> following write skipped */
TEST(swB_backward_not_found_skips)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "B0010004 454E4421\n00000005 000000AB");

    uint8_t exp[16] = {0};
    CHECK_MEM("tB not-found -> write skipped", buf, exp, sizeof(buf));
}

/* ==================== Save Wizard type C (address byte) ================== */

/* use the bytes at an address as the pattern; search forward from addr+len */
TEST(swC_addr_search_found)
{
    uint8_t buf[16] = {0};
    buf[2] = 0x58; buf[3] = 0x59;    /* pattern source "XY" */
    buf[9] = 0x58; buf[10] = 0x59;   /* later occurrence    */
    apply_sw(buf, sizeof(buf), "C0010002 00000002\n08000000 000000AB");

    uint8_t exp[16] = {0};
    exp[2] = 0x58; exp[3] = 0x59;
    exp[9] = 0xAB; exp[10] = 0x59;   /* found forward @9 */
    CHECK_MEM("tC addr-search found", buf, exp, sizeof(buf));
}

/* pattern occurs only at the source address -> nothing found forward -> skip */
TEST(swC_addr_search_not_found_skips)
{
    uint8_t buf[16] = {0};
    buf[2] = 0x58; buf[3] = 0x59;
    apply_sw(buf, sizeof(buf), "C0010002 00000002\n00000000 000000AB");

    uint8_t exp[16] = {0};
    exp[2] = 0x58; exp[3] = 0x59;
    CHECK_MEM("tC addr-search not found -> skip", buf, exp, sizeof(buf));
}

/* ==================== Save Wizard type D (byte-test skip) ================ */

/* test passes -> following line executes */
TEST(swD_test_pass_no_skip)
{
    uint8_t buf[16] = {0};
    buf[4] = 0x05;
    apply_sw(buf, sizeof(buf), "D0000004 01100005\n00000008 000000AB");

    uint8_t exp[16] = {0};
    exp[4] = 0x05;
    exp[8] = 0xAB;                   /* 8-bit @4 == 5 -> no skip */
    CHECK_MEM("tD test true -> execute", buf, exp, sizeof(buf));
}

/* test fails -> skip the following line */
TEST(swD_test_fail_skips)
{
    uint8_t buf[16] = {0};           /* data[4] = 0 != 5 */
    apply_sw(buf, sizeof(buf), "D0000004 01100005\n00000008 000000AB");

    uint8_t exp[16] = {0};
    CHECK_MEM("tD test false -> skip", buf, exp, sizeof(buf));
}

/* ============================== BSD search =============================== */

/* found -> pointer set, write relative to it */
TEST(bsd_search_found)
{
    uint8_t init[16] = {0};
    memcpy(init + 6, "TEST", 4);
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "search \"TEST\"\nwrite next 0:AB");

    uint8_t exp[16] = {0};
    memcpy(exp + 6, "TEST", 4);
    exp[6] = 0xAB;
    CHECK_U64("bsd search found: size kept", n, sizeof(init));
    CHECK_MEM("bsd search found -> write @ptr", buf, exp, sizeof(exp));
    free(buf);
}

/* not found -> whole code aborts (returns 0), buffer untouched */
TEST(bsd_search_not_found_aborts)
{
    uint8_t init[16] = {0};          /* no "TEST" */
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "search \"TEST\"\nwrite next 0:AB");

    uint8_t exp[16] = {0};
    CHECK_U64("bsd search not-found: returns 0", n, 0);
    CHECK_MEM("bsd search not-found -> untouched", buf, exp, sizeof(exp));
    free(buf);
}

/* occurrence count -> Nth match */
TEST(bsd_search_count_second)
{
    uint8_t init[16] = {0};
    init[2] = 0x41; init[3] = 0x42;  /* "AB" #1 */
    init[8] = 0x41; init[9] = 0x42;  /* "AB" #2 */
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "search 4142:2\nwrite next 0:CD");

    uint8_t exp[16] = {0};
    exp[2] = 0x41; exp[3] = 0x42;
    exp[8] = 0xCD; exp[9] = 0x42;
    CHECK_U64("bsd search count: size kept", n, sizeof(init));
    CHECK_MEM("bsd search count=2 -> 2nd occurrence", buf, exp, sizeof(exp));
    free(buf);
}
