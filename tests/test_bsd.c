/*
 * BSD script vectors.
 *
 * BSD is fully build-flag-invariant: reads/writes go through the unconditional
 * BE* macros, and host-native integer truncation goes through HOST_LSB (which
 * depends only on the real host byte order, the same in both LE and __PS3_PC__
 * builds). So every BSD vector uses a single shared expected array, and the
 * two builds must agree.
 *
 * Note: carry()-based checksum truncation (wadd/dwadd/add/sub) originally used
 * the target-endian PADDING macro and produced WRONG, divergent output in the
 * __PS3_PC__ build (it sliced the high half of the accumulator on a
 * little-endian host). That was fixed by switching those sites to HOST_LSB;
 * bsd_carry_padding_truncation() below guards the corrected, invariant result.
 * See tests/README.md.
 */
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

/* returns new size; *buf may be realloc'd by insert/delete */
static size_t apply_bsd(uint8_t** buf, size_t len, const char* codes)
{
    free_patch_var_list();               /* isolate variable state per test */
    code_entry_t c = make_bsd_code(codes);
    return apply_bsd_patch_code(buf, len, &c);
}

static uint8_t* dup_bytes(const uint8_t* src, size_t len)
{
    uint8_t* p = malloc(len);
    memcpy(p, src, len);
    return p;
}

/* write at OFFSET: verbatim hex bytes */
TEST(bsd_write_hex)
{
    uint8_t init[16] = {0};
    uint8_t* buf = dup_bytes(init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "write at 4:AABBCCDD");

    uint8_t exp[16] = {0};
    exp[4]=0xAA; exp[5]=0xBB; exp[6]=0xCC; exp[7]=0xDD;
    CHECK_U64("write hex: size unchanged", n, sizeof(init));
    CHECK_MEM("write at 4:AABBCCDD", buf, exp, sizeof(exp));
    free(buf);
}

/* set pointer then write relative to it */
TEST(bsd_write_next_pointer)
{
    uint8_t init[16] = {0};
    uint8_t* buf = dup_bytes(init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "set pointer:8\nwrite next 0:EEFF");

    uint8_t exp[16] = {0};
    exp[8]=0xEE; exp[9]=0xFF;
    CHECK_U64("write next: size unchanged", n, sizeof(init));
    CHECK_MEM("pointer=8, write next 0:EEFF", buf, exp, sizeof(exp));
    free(buf);
}

/* repeat(count,value) */
TEST(bsd_write_repeat)
{
    uint8_t init[16] = {0};
    uint8_t* buf = dup_bytes(init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "write at 0:repeat(4,AB)");

    uint8_t exp[16] = {0};
    exp[0]=exp[1]=exp[2]=exp[3]=0xAB;
    CHECK_U64("repeat: size unchanged", n, sizeof(init));
    CHECK_MEM("write at 0:repeat(4,AB)", buf, exp, sizeof(exp));
    free(buf);
}

/* insert grows the buffer, shifting the tail right */
TEST(bsd_insert)
{
    uint8_t init[16];
    for (int i = 0; i < 16; i++) init[i] = (uint8_t)i;   /* 00..0F */
    uint8_t* buf = dup_bytes(init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "insert at 4:AABB");

    uint8_t exp[18] = { 0,1,2,3, 0xAA,0xBB, 4,5,6,7,8,9,10,11,12,13,14,15 };
    CHECK_U64("insert: size +2", n, 18);
    CHECK_MEM("insert at 4:AABB", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * carry()-based checksum truncation must be host-consistent and therefore
 * identical in both builds (regression guard for the PADDING -> HOST_LSB fix).
 *
 * wadd(0x0,0x3) over {12 34 56 78} = be16(0x1234)+be16(0x5678) = 0x000068AC.
 * In little-endian host memory that u32 is [AC 68 00 00]. carry(2) keeps the
 * low 16 bits (HOST_LSB=0 on a little-endian host, in BOTH builds), which the
 * write path then emits big-endian as [68 AC]. This matches what a real PS3
 * (__PPU__) and a real PS4/PC produce; before the fix the __PS3_PC__ build
 * wrongly kept the high half and wrote [00 00].
 */
TEST(bsd_carry_padding_truncation)
{
    uint8_t init[16] = {0};
    init[0]=0x12; init[1]=0x34; init[2]=0x56; init[3]=0x78;
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [c]:0\ncarry(2)\nset [c]:wadd(0x0,0x3)\nwrite at 0x8:[c]");

    uint8_t exp[16] = {0};
    memcpy(exp, init, 4);
    exp[8]=0x68; exp[9]=0xAC;   /* low half 0x68AC, emitted big-endian; same in LE and BE */
    CHECK_MEM("wadd carry(2) truncation (HOST_LSB)", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * add() with carry — the second HOST_LSB site (patches.c add handler).
 * add(0x0,0x3) = 0xFF+0xFF+0xFF+0x04 = 0x00000301. carry(2) keeps the low 16
 * bits 0x0301 (HOST_LSB=0 on the host), emitted big-endian as [03 01] in both
 * builds. Before the fix the __PS3_PC__ build kept the high half -> [00 00].
 */
TEST(bsd_add_carry_truncation)
{
    uint8_t init[16] = {0};
    init[0]=0xFF; init[1]=0xFF; init[2]=0xFF; init[3]=0x04;
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [c]:0\ncarry(2)\nset [c]:add(0x0,0x3)\nwrite at 0x8:[c]");

    uint8_t exp[16] = {0};
    memcpy(exp, init, 4);
    exp[8]=0x03; exp[9]=0x01;
    CHECK_MEM("add carry(2) truncation (HOST_LSB)", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * right(value,len) — the third HOST_LSB site. Keeps the `len` rightmost
 * (least-significant) bytes of the value. right(0x12345678,2) -> 0x5678,
 * emitted big-endian as [56 78] in both builds. Before the fix the __PS3_PC__
 * build kept the LEFT bytes -> [12 34].
 */
TEST(bsd_right_truncation)
{
    uint8_t init[16] = {0};
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [r]:right(0x12345678,2)\nwrite at 0:[r]");

    uint8_t exp[16] = {0};
    exp[0]=0x56; exp[1]=0x78;
    CHECK_MEM("right(v,2) keeps low bytes (HOST_LSB)", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * left(value,len) — keeps the leftmost / MOST-significant `len` bytes of the
 * value, host-consistently (via HOST_MSB), emitted big-endian by the write
 * path. left(0x00012345,2) -> the top 2 bytes 0x0001 -> [00 01] in every build.
 * Before the HOST_MSB fix the little-endian builds wrongly kept the low bytes
 * (0x2345 -> [23 45], identical to right()).
 */
TEST(bsd_left)
{
    uint8_t init[16] = {0};
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [l]:left(0x00012345,2)\nwrite at 0:[l]");

    uint8_t exp[16] = {0};
    exp[0]=0x00; exp[1]=0x01;
    CHECK_MEM("left(v,2) keeps leftmost bytes", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * mid(value,start,len) — extracts `len` bytes starting at `start` of the value's
 * big-endian byte view, and (via the read()-style normalisation) emits them
 * verbatim on every host. mid(0x00012345,0,2) -> [00 01], mid(...,2,2) -> [23 45].
 * Before the fix the little-endian builds byte-swapped the 2-byte result
 * ([00 01] -> [01 00]).
 */
TEST(bsd_mid)
{
    uint8_t init[16] = {0};
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [m]:mid(0x00012345,0,2)\nwrite at 0:[m]");

    uint8_t exp[16] = {0};
    exp[0]=0x00; exp[1]=0x01;
    CHECK_MEM("mid(v,0,2) verbatim big-endian slice", buf, exp, sizeof(exp));
    free(buf);
}

/* mid at a non-zero offset, and a length (3) the write path never swaps —
 * confirms the substring is verbatim for both swapped and non-swapped sizes. */
TEST(bsd_mid_offset)
{
    uint8_t init[16] = {0};
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [m]:mid(0x00012345,1,3)\nwrite at 0:[m]");

    uint8_t exp[16] = {0};
    exp[0]=0x01; exp[1]=0x23; exp[2]=0x45;
    CHECK_MEM("mid(v,1,3) verbatim substring", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * Update of an already-existing variable — the fourth HOST_LSB site
 * (patches.c:796, where an existing var's value is re-fetched into old_val and
 * var->data is re-pointed at its low bytes). The var is created with read()
 * (not a HOST_LSB site) so ONLY line 796 is under test here.
 *
 * read(0,2) of file bytes {AA BB} stores the big-endian value 0xAABB. The
 * second set re-fetches it (line 796 -> low bytes), endian_swap reverses to
 * 0xBBAA, and write emits it big-endian as [BB AA]. Same in both builds; before
 * the fix the __PS3_PC__ build re-pointed at the high (zero) bytes -> [00 00].
 */
TEST(bsd_update_existing_variable)
{
    uint8_t init[16] = {0};
    init[0]=0xAA; init[1]=0xBB;
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init),
              "set [v]:read(0,2)\nset [v]:endian_swap\nwrite at 4:[v]");

    uint8_t exp[16] = {0};
    exp[0]=0xAA; exp[1]=0xBB;    /* untouched source */
    exp[4]=0xBB; exp[5]=0xAA;    /* re-fetched, swapped, written big-endian */
    CHECK_MEM("existing-var re-fetch truncation (HOST_LSB)", buf, exp, sizeof(exp));
    free(buf);
}

/* delete shrinks the buffer, shifting the tail left */
TEST(bsd_delete)
{
    uint8_t init[16];
    for (int i = 0; i < 16; i++) init[i] = (uint8_t)i;   /* 00..0F */
    uint8_t* buf = dup_bytes(init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "delete at 4:2");

    uint8_t exp[14] = { 0,1,2,3, 6,7,8,9,10,11,12,13,14,15 };
    CHECK_U64("delete: size -2", n, 14);
    CHECK_MEM("delete at 4:2", buf, exp, sizeof(exp));
    free(buf);
}
