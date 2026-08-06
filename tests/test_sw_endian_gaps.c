/*
 * Save Wizard endian-critical opcode coverage — the gaps that matter most for
 * the compile-time-flag removal, because they route multi-byte values through
 * the endian-sensitive MEM16/MEM32/MEM64 macros:
 *
 *   type 3  — 8-byte add (MEM64) and the pointer-relative address form
 *   type 4  — 32-bit multi-write (MEM32)
 *   type 6  — pointer "mega code": endian-sensitive READ (MEM16) and WRITE (MEM32)
 *   type 7  — conditional "no less / no more than" write (MEM16/MEM32)
 *   type 9  — pointer add/sub (ops 2/3) and end-pointer set (ops D/E)
 *   type D  — conditional skip with explicit 16-bit BE (Z=0) and LE (Z=2) reads
 *
 * Endian-sensitive results carry #if APOLLO_TEST_ENDIAN_BE expectations; the
 * pointer-arithmetic / explicit-endian opcodes are build-invariant (one array).
 * Expected bytes are computed by hand from docs/savewizard.rst.
 */
#include <string.h>
#include "test_common.h"

static void apply_sw(uint8_t* buf, size_t len, const char* codes)
{
    code_entry_t c = make_sw_code(codes);
    apply_sw_patch_code(buf, len, &c);
}

/* ---- type 3: 8-byte increment (MEM64) ---------------------------------- */
TEST(sw3_add64)
{
    uint8_t buf[16] = {0};
    buf[0] = 0xFF;   /* LE value 0x00..FF, BE value 0xFF00..00 */
    apply_sw(buf, sizeof(buf), "33000000 00000001");

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    /* 0xFF00000000000000 + 1 -> big-endian */
    exp[0]=0xFF; exp[7]=0x01;
#else
    /* 0x00000000000000FF + 1 = 0x100 -> little-endian */
    exp[1]=0x01;
#endif
    CHECK_MEM("add 8 bytes +1 (MEM64)", buf, exp, sizeof(buf));
}

/* ---- type 3: pointer-relative 4-byte add (op A) ------------------------ */
TEST(sw3_pointer_add32)
{
    uint8_t buf[16] = {0};
    buf[8] = 0xFF;
    /* pointer=4, then op A adds at (offset 4 + pointer 4)=8 */
    apply_sw(buf, sizeof(buf), "95000000 00000004\n3A000004 00000001");

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    exp[8]=0xFF; exp[11]=0x01;
#else
    exp[9]=0x01;
#endif
    CHECK_MEM("pointer-relative add32 @ptr+off", buf, exp, sizeof(buf));
}

/* ---- type 4: 32-bit incremental multi-write (MEM32) -------------------- */
TEST(sw4_multiwrite32)
{
    uint8_t buf[16] = {0};
    /* write 0x12345678 at 0x0, +1 value / +8 addr, 2 times */
    apply_sw(buf, sizeof(buf), "42000000 12345678\n40020008 00000001");

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    exp[0]=0x12; exp[1]=0x34; exp[2]=0x56; exp[3]=0x78;   /* 0x12345678 */
    exp[8]=0x12; exp[9]=0x34; exp[10]=0x56; exp[11]=0x79; /* 0x12345679 */
#else
    exp[0]=0x78; exp[1]=0x56; exp[2]=0x34; exp[3]=0x12;
    exp[8]=0x79; exp[9]=0x56; exp[10]=0x34; exp[11]=0x12;
#endif
    CHECK_MEM("multi-write 32-bit x2", buf, exp, sizeof(buf));
}

/* ---- type 6: move pointer, then 32-bit write (MEM32) ------------------- */
TEST(sw6_move_write32)
{
    uint8_t buf[16] = {0};
    /* w=2 move pointer +8; w=4 write 32-bit value at data+pointer */
    apply_sw(buf, sizeof(buf), "62200000 00000008\n62400000 12345678");

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    exp[8]=0x12; exp[9]=0x34; exp[10]=0x56; exp[11]=0x78;
#else
    exp[8]=0x78; exp[9]=0x56; exp[10]=0x34; exp[11]=0x12;
#endif
    CHECK_MEM("type6 move + write32", buf, exp, sizeof(buf));
}

/* ---- type 6: endian-sensitive 16-bit READ (MEM16) --------------------- */
TEST(sw6_read16)
{
    uint8_t buf[512] = {0};
    buf[4] = 0x01; buf[5] = 0x00;   /* LE reads 0x0001=1, BE reads 0x0100=256 */
    /* READ 16-bit @4 -> ptr_value; MOVE pointer=ptr_value; write 0xAB @ptr
     * (6TWX: t=0 8-bit, w=4 write -> "604.....") */
    apply_sw(buf, sizeof(buf), "61000000 00000004\n61100000 00000000\n60400000 000000AB");

    uint8_t exp[512] = {0};
    exp[4] = 0x01;
#if APOLLO_TEST_ENDIAN_BE
    exp[256] = 0xAB;   /* pointer = 0x0100 */
#else
    exp[1] = 0xAB;     /* pointer = 0x0001 */
#endif
    CHECK_MEM("type6 read16 (MEM16) drives pointer", buf, exp, sizeof(buf));
}

/* ---- type 7: conditional "no less than" 16-bit (MEM16) ---------------- */
TEST(sw7_no_less_than16)
{
    uint8_t buf[16] = {0};
    buf[4] = 0x00; buf[5] = 0x50;   /* LE=0x5000, BE=0x0050 */
    apply_sw(buf, sizeof(buf), "71000004 00001234");   /* ensure >= 0x1234 */

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    /* current 0x0050 < 0x1234 -> written 0x1234 big-endian */
    exp[4]=0x12; exp[5]=0x34;
#else
    /* current 0x5000 >= 0x1234 -> unchanged */
    exp[4]=0x00; exp[5]=0x50;
#endif
    CHECK_MEM("no-less-than 16-bit (MEM16)", buf, exp, sizeof(buf));
}

/* ---- type 7: conditional "no more than" 32-bit (MEM32) ---------------- */
TEST(sw7_no_more_than32)
{
    uint8_t buf[16] = {0};
    buf[4]=0x00; buf[5]=0x00; buf[6]=0x00; buf[7]=0x50;  /* LE=0x50000000, BE=0x50 */
    apply_sw(buf, sizeof(buf), "76000004 00001000");     /* cap at 0x1000 */

    uint8_t exp[16] = {0};
#if APOLLO_TEST_ENDIAN_BE
    /* current 0x50 <= 0x1000 -> unchanged */
    exp[7]=0x50;
#else
    /* current 0x50000000 > 0x1000 -> capped to 0x1000 little-endian */
    exp[5]=0x10;
#endif
    CHECK_MEM("no-more-than 32-bit (MEM32)", buf, exp, sizeof(buf));
}

/* ---- type 9: add / subtract to pointer (ops 2/3, invariant) ----------- */
TEST(sw9_pointer_add_sub)
{
    uint8_t buf[16] = {0};
    /* pointer=8, +4 -> 12, write @12 */
    apply_sw(buf, sizeof(buf), "95000000 00000008\n92000000 00000004\n08000000 000000AB");
    uint8_t exp[16] = {0};
    exp[12] = 0xAB;
    CHECK_MEM("pointer add (op2)", buf, exp, sizeof(buf));

    uint8_t buf2[16] = {0};
    /* pointer=8, -4 -> 4, write @4 */
    apply_sw(buf2, sizeof(buf2), "95000000 00000008\n93000000 00000004\n08000000 000000CD");
    uint8_t exp2[16] = {0};
    exp2[4] = 0xCD;
    CHECK_MEM("pointer sub (op3)", buf2, exp2, sizeof(buf2));
}

/* ---- type 9: set end pointer (ops D/E) bound a backward search --------- */
TEST(sw9_end_pointer_D)
{
    uint8_t buf[16] = {0};
    buf[4]=0x41; buf[5]=0x42;      /* "AB" within range   */
    buf[12]=0x41; buf[13]=0x42;    /* "AB" beyond end ptr */
    /* end pointer = 8 (op D), backward search finds the in-range one @4 */
    apply_sw(buf, sizeof(buf), "9D000000 00000008\nB0010002 41420000\n08000000 000000CD");

    uint8_t exp[16] = {0};
    exp[4]=0xCD; exp[5]=0x42; exp[12]=0x41; exp[13]=0x42;
    CHECK_MEM("end-pointer D bounds backward search", buf, exp, sizeof(buf));
}

TEST(sw9_end_pointer_E)
{
    uint8_t buf[16] = {0};
    buf[4]=0x41; buf[5]=0x42;
    buf[12]=0x41; buf[13]=0x42;
    /* pointer=6, end pointer = pointer+2 = 8 (op E); search finds @4 */
    apply_sw(buf, sizeof(buf), "95000000 00000006\n9E000000 00000002\nB0010002 41420000\n08000000 000000CD");

    uint8_t exp[16] = {0};
    exp[4]=0xCD; exp[5]=0x42; exp[12]=0x41; exp[13]=0x42;
    CHECK_MEM("end-pointer E bounds backward search", buf, exp, sizeof(buf));
}

/* ---- type D: explicit 16-bit BE (Z=0) vs LE (Z=2) value reads ---------- */
/* Same bytes {12 34} and threshold 0x2000, opposite skip decisions:
 * BE reads 0x1234 (< 0x2000 -> skip), LE reads 0x3412 (> 0x2000 -> execute). */
TEST(swD_test_16bit_be)
{
    uint8_t buf[16] = {0};
    buf[4]=0x12; buf[5]=0x34;
    apply_sw(buf, sizeof(buf), "D0000004 01022000\n00000008 000000AB");

    uint8_t exp[16] = {0};
    exp[4]=0x12; exp[5]=0x34;      /* write skipped: 0x1234 not > 0x2000 */
    CHECK_MEM("type D 16-bit BE read", buf, exp, sizeof(buf));
}

TEST(swD_test_16bit_le)
{
    uint8_t buf[16] = {0};
    buf[4]=0x12; buf[5]=0x34;
    apply_sw(buf, sizeof(buf), "D0000004 01222000\n00000008 000000AB");

    uint8_t exp[16] = {0};
    exp[4]=0x12; exp[5]=0x34;
    exp[8]=0xAB;                   /* executed: 0x3412 > 0x2000 */
    CHECK_MEM("type D 16-bit LE read", buf, exp, sizeof(buf));
}
