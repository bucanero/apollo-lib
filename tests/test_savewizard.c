/*
 * Save Wizard opcode vectors.
 *
 * Save-wizard multi-byte reads/writes go through the MEM* macros, which are
 * the ONLY endian-sensitive path in the library:
 *   - LE build (default) : MEM* is a no-op  -> values stored little-endian
 *   - BE build (__PS3_PC__): MEM* byte-swaps -> values stored big-endian
 *
 * Vectors whose result depends on that flag carry #if APOLLO_TEST_ENDIAN_BE
 * expectations. Opcodes that use BE* (search/bulk, always big-endian) or that
 * read pointers with an explicit endianness are INVARIANT: their expected
 * bytes are identical in both builds, which is asserted by using one array.
 *
 * Expected bytes are computed by hand from docs/savewizard.rst, independent of
 * the implementation, so a behavior change is caught rather than blessed.
 */
#include <string.h>
#include "test_common.h"

static void apply_sw(uint8_t* buf, size_t len, const char* codes)
{
    code_entry_t c = make_sw_code(codes);
    apollo_apply_sw_code(buf, len, &c);
}

/* Type 0 — 8-bit direct write (endian-invariant). */
TEST(sw_write8)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "00000004 000000AB");

    uint8_t exp[16] = {0};
    exp[4] = 0xAB;
    CHECK_MEM("8-bit write @0x4", buf, exp, sizeof(buf));
}

/* Type 1 — 16-bit direct write (endian-sensitive). */
TEST(sw_write16)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "10000002 00001234");

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    exp[2] = 0x12; exp[3] = 0x34;
#else
    exp[2] = 0x34; exp[3] = 0x12;
#endif
    CHECK_MEM("16-bit write 0x1234 @0x2", buf, exp, sizeof(buf));
}

/* Type 2 — 32-bit direct write (endian-sensitive). Canonical case. */
TEST(sw_write32)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "20000004 12345678");

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    exp[4] = 0x12; exp[5] = 0x34; exp[6] = 0x56; exp[7] = 0x78;
#else
    exp[4] = 0x78; exp[5] = 0x56; exp[6] = 0x34; exp[7] = 0x12;
#endif
    CHECK_MEM("32-bit write 0x12345678 @0x4", buf, exp, sizeof(buf));
}

/* Type 3 (op 2) — add to 32-bit value (read-modify-write, endian-sensitive). */
TEST(sw_add32)
{
    uint8_t buf[16] = {0};
    buf[4] = 0xFF;  /* stored value: LE reads 0x000000FF, BE reads 0xFF000000 */
    apply_sw(buf, sizeof(buf), "32000004 00000001");

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    /* 0xFF000000 + 1 = 0xFF000001 -> big-endian */
    exp[4] = 0xFF; exp[5] = 0x00; exp[6] = 0x00; exp[7] = 0x01;
#else
    /* 0x000000FF + 1 = 0x00000100 -> little-endian */
    exp[4] = 0x00; exp[5] = 0x01; exp[6] = 0x00; exp[7] = 0x00;
#endif
    CHECK_MEM("add32 +1", buf, exp, sizeof(buf));
}

/* Type 3 (op 5) — subtract from 16-bit value (endian-sensitive). */
TEST(sw_sub16)
{
    uint8_t buf[16] = {0};
    buf[0] = 0x00; buf[1] = 0x01;  /* LE reads 0x0100, BE reads 0x0001 */
    apply_sw(buf, sizeof(buf), "35000000 00000001");

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    /* 0x0001 - 1 = 0x0000 -> big-endian */
    exp[0] = 0x00; exp[1] = 0x00;
#else
    /* 0x0100 - 1 = 0x00FF -> little-endian */
    exp[0] = 0xFF; exp[1] = 0x00;
#endif
    CHECK_MEM("sub16 -1", buf, exp, sizeof(buf));
}

/* Type 4 — incremental multi-write of 16-bit values (endian-sensitive). */
TEST(sw_multiwrite16)
{
    uint8_t buf[16] = {0};
    /* write 0x1234 at 0x0, then +1 value / +4 addr, 3 times */
    apply_sw(buf, sizeof(buf), "41000000 00001234\n40030004 00000001");

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    exp[0] = 0x12; exp[1] = 0x34;   /* 0x1234 */
    exp[4] = 0x12; exp[5] = 0x35;   /* 0x1235 */
    exp[8] = 0x12; exp[9] = 0x36;   /* 0x1236 */
#else
    exp[0] = 0x34; exp[1] = 0x12;
    exp[4] = 0x35; exp[5] = 0x12;
    exp[8] = 0x36; exp[9] = 0x12;
#endif
    CHECK_MEM("multi-write 16-bit x3", buf, exp, sizeof(buf));
}

/* Type 5 — copy bytes (verbatim memcpy, endian-invariant). */
TEST(sw_copy)
{
    uint8_t buf[16] = {0};
    buf[0] = 0xDE; buf[1] = 0xAD; buf[2] = 0xBE; buf[3] = 0xEF;
    apply_sw(buf, sizeof(buf), "50000000 00000004\n50000008 00000000");

    uint8_t exp[16] = {0};
    exp[0] = 0xDE; exp[1] = 0xAD; exp[2] = 0xBE; exp[3] = 0xEF;
    exp[8] = 0xDE; exp[9] = 0xAD; exp[10] = 0xBE; exp[11] = 0xEF;
    CHECK_MEM("copy 4 bytes 0x0->0x8", buf, exp, sizeof(buf));
}

/* Type A — bulk write. Data uses BE* (always big-endian) -> invariant. */
TEST(sw_bulk_write)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "A0000004 00000008\n11223344 55667788");

    uint8_t exp[16] = {0};
    uint8_t payload[8] = { 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88 };
    memcpy(exp + 4, payload, 8);
    CHECK_MEM("bulk write 8 bytes @0x4", buf, exp, sizeof(buf));
}

/* Type 8 — forward search (pattern is big-endian via BE*, invariant),
 * then an 8-bit pointer-relative write to prove the pointer landed. */
TEST(sw_search_then_write)
{
    uint8_t buf[16] = {0};
    memcpy(buf + 6, "TEST", 4);   /* 54 45 53 54 */
    apply_sw(buf, sizeof(buf), "80010004 54455354\n08000000 000000AB");

    uint8_t exp[16] = {0};
    memcpy(exp + 6, "TEST", 4);
    exp[6] = 0xAB;                /* pointer=6, write overwrites first byte */
    CHECK_MEM("search 'TEST' then write @ptr", buf, exp, sizeof(buf));
}

/* Type 9 (op 0) — set pointer to BIG-endian value at address (explicit,
 * endian-flag-invariant), then write at that pointer. */
TEST(sw_ptr_from_be_value)
{
    uint8_t buf[32] = {0};
    buf[8] = 0x00; buf[9] = 0x00; buf[10] = 0x00; buf[11] = 0x10;  /* BE 0x10 = 16 */
    apply_sw(buf, sizeof(buf), "90000000 00000008\n08000000 000000AB");

    uint8_t exp[32] = {0};
    exp[11] = 0x10;
    exp[16] = 0xAB;
    CHECK_MEM("ptr=BE(read@8)=16, write @ptr", buf, exp, sizeof(buf));
}

/* Type 9 (op 1) — set pointer to LITTLE-endian value at address (explicit,
 * endian-flag-invariant). */
TEST(sw_ptr_from_le_value)
{
    uint8_t buf[32] = {0};
    buf[8] = 0x10; buf[9] = 0x00; buf[10] = 0x00; buf[11] = 0x00;  /* LE 0x10 = 16 */
    apply_sw(buf, sizeof(buf), "91000000 00000008\n08000000 000000CD");

    uint8_t exp[32] = {0};
    exp[8] = 0x10;
    exp[16] = 0xCD;
    CHECK_MEM("ptr=LE(read@8)=16, write @ptr", buf, exp, sizeof(buf));
}

/* Type 9 (op 5) — set pointer directly to X, then pointer-relative write. */
TEST(sw_ptr_set_direct)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "95000000 00000008\n08000000 000000AB");

    uint8_t exp[16] = {0};
    exp[8] = 0xAB;
    CHECK_MEM("ptr=8, write @ptr", buf, exp, sizeof(buf));
}

/* Type 9 (op 4) — set pointer to EOF minus X, then pointer-relative write. */
TEST(sw_ptr_from_eof)
{
    uint8_t buf[16] = {0};
    apply_sw(buf, sizeof(buf), "94000000 00000004\n08000000 000000AB");

    uint8_t exp[16] = {0};
    exp[12] = 0xAB;   /* 16 - 4 = 12 */
    CHECK_MEM("ptr=eof-4=12, write @ptr", buf, exp, sizeof(buf));
}
