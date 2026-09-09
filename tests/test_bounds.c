/*
 * Bounds-safety vectors.
 *
 * These guard the offset/length validation added to apollo_apply_sw_code and
 * apollo_apply_bsd_code: an out-of-range access driven by a .savepatch must be
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
    apollo_apply_sw_code(buf, len, &c);
}

static size_t apply_bsd(uint8_t** buf, size_t len, const char* codes)
{
    apollo_free_var_list();
    code_entry_t c = make_bsd_code(codes);
    return apollo_apply_bsd_code(buf, len, &c);
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

/* ---------------------------------------------------------------------------
 * Parameter clamping: the checksum commands take an INCLUSIVE end offset and
 * real patches write it as `eof+1` (one past the last byte), so the range must
 * be clamped to the buffer. Before the clamp these summed one byte of
 * unallocated heap and produced a nondeterministic checksum.
 * ------------------------------------------------------------------------- */

/* add(start, eof+1) must sum exactly the buffer, not one byte more */
TEST(clamp_bsd_add_eof_plus_one)
{
    uint8_t init[16];
    for (int i = 0; i < 16; i++) init[i] = (uint8_t)(i + 1);   /* sum 1..16 = 136 */
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    /* range 0x4..end: 5+6+...+16 = 126 = 0x7E */
    size_t n = apply_bsd(&buf, sizeof(init),
                         "set [v]:0\nset pointer:eof+1\nset [v]:add(0x000004,pointer)\nwrite at 0:[v]");

    uint8_t exp[16];
    memcpy(exp, init, sizeof(init));
    exp[0]=0x00; exp[1]=0x00; exp[2]=0x00; exp[3]=0x7E;   /* variables are written big-endian */
    CHECK_U64("add(eof+1): applied", n, sizeof(init));
    CHECK_MEM("add(eof+1): sums the buffer only", buf, exp, sizeof(exp));
    free(buf);
}

/* the same range written as eof (the last byte) must give the same answer */
TEST(clamp_bsd_add_eof_matches_eof_plus_one)
{
    uint8_t init[16];
    for (int i = 0; i < 16; i++) init[i] = (uint8_t)(i + 1);
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init),
                         "set [v]:0\nset pointer:eof\nset [v]:add(0x000004,pointer)\nwrite at 0:[v]");

    uint8_t exp[16];
    memcpy(exp, init, sizeof(init));
    exp[0]=0x00; exp[1]=0x00; exp[2]=0x00; exp[3]=0x7E;
    CHECK_U64("add(eof): applied", n, sizeof(init));
    CHECK_MEM("add(eof) == add(eof+1)", buf, exp, sizeof(exp));
    free(buf);
}

/* a start past the end yields an empty range, not a wild read */
TEST(clamp_bsd_add_start_past_end)
{
    uint8_t init[16];
    for (int i = 0; i < 16; i++) init[i] = (uint8_t)(i + 1);
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init), "set [v]:0\nset [v]:add(0x00FFFF,0x00FFFF)\nwrite at 0:[v]");

    uint8_t exp[16];
    memcpy(exp, init, sizeof(init));
    exp[0]=0x00; exp[1]=0x00; exp[2]=0x00; exp[3]=0x00;
    CHECK_U64("add() past the end: applied", n, sizeof(init));
    CHECK_MEM("add() past the end: sums nothing", buf, exp, sizeof(exp));
    free(buf);
}

/* wadd(start, eof+1) stays inside the buffer too */
TEST(clamp_bsd_wadd_eof_plus_one)
{
    uint8_t init[16];
    for (int i = 0; i < 16; i++) init[i] = (uint8_t)(i + 1);
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    /* BE words from 0x4: 0506+0708+090A+0B0C+0D0E+0F10 = 0x3C42 */
    size_t n = apply_bsd(&buf, sizeof(init),
                         "set [v]:0\nset pointer:eof+1\nset [v]:wadd(0x000004,pointer)\nwrite at 0:[v]");

    uint8_t exp[16];
    memcpy(exp, init, sizeof(init));
    exp[0]=0x00; exp[1]=0x00; exp[2]=0x3C; exp[3]=0x42;
    CHECK_U64("wadd(eof+1): applied", n, sizeof(init));
    CHECK_MEM("wadd(eof+1): sums the buffer only", buf, exp, sizeof(exp));
    free(buf);
}

/* carry > 4 used to underflow var->len (huge malloc) and shift a uint32 by 32 */
TEST(clamp_bsd_carry_out_of_range)
{
    uint8_t init[16];
    for (int i = 0; i < 16; i++) init[i] = 0xFF;
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    size_t n = apply_bsd(&buf, sizeof(init),
                         "carry(9)\nset [v]:0\nset [v]:add(0x000000,0x00000F)\nwrite at 0:[v]");

    CHECK_U64("carry(9): no crash, code still applied", n, sizeof(init));
    free(buf);
}

/* left()/right() slice a 32-bit value: a wider length is rejected, not read OOB */
TEST(clamp_bsd_left_right_width)
{
    uint8_t init[16] = {0};
    uint8_t* buf = malloc(sizeof(init));
    memcpy(buf, init, sizeof(init));

    CHECK_U64("right(,9) rejected", apply_bsd(&buf, sizeof(init), "set [v]:right(0x11223344,9)\nwrite at 0:[v]"), 0);
    CHECK_U64("left(,9) rejected",  apply_bsd(&buf, sizeof(init), "set [v]:left(0x11223344,9)\nwrite at 0:[v]"), 0);

    /* a valid width still works: right(...,2) keeps the low 2 bytes */
    size_t n = apply_bsd(&buf, sizeof(init), "set [v]:right(0x11223344,2)\nwrite at 0:[v]");
    uint8_t exp[16] = {0};
    exp[0]=0x33; exp[1]=0x44;
    CHECK_U64("right(,2) applied", n, sizeof(init));
    CHECK_MEM("right(,2) keeps low bytes", buf, exp, sizeof(exp));
    free(buf);
}

/* ---------------------------------------------------------------------------
 * rockstar_checksum walks CHKS records whose block offset and length come
 * straight out of the save file. A valid record must still be processed; a
 * record pointing outside the buffer must be skipped, not hashed/written.
 * ------------------------------------------------------------------------- */

/* builds a buffer with one CHKS record at 0x80 covering [0x80, 0x80+blk_len) */
static uint8_t* make_chks_buf(size_t size, uint32_t blk_len)
{
    uint8_t* b = calloc(1, size);

    for (size_t i = 0; i < size; i++) b[i] = (uint8_t)(i * 7 + 1);

    memcpy(b + 0x80, "CHKS", 4);
    b[0x84] = 0x00;                                   /* search matches "CHKS\0" */
    b[0x85] = 0x00; b[0x86] = 0x00; b[0x87] = 0x40;   /* chks     = 0x40 (BE) */
    b[0x88] = (uint8_t)(blk_len >> 24); b[0x89] = (uint8_t)(blk_len >> 16);
    b[0x8A] = (uint8_t)(blk_len >> 8);  b[0x8B] = (uint8_t)blk_len;
    return b;
}

TEST(clamp_bsd_rockstar_chks_valid_record)
{
    const uint32_t blk_len = 0x40;               /* block = 0x80 - 0x40 + 0x40 = 0x80 */
    uint8_t* buf = make_chks_buf(0x100, blk_len);
    uint8_t exp_hash[4];

    /* replicate the engine: zero 8 bytes at +8, then hash [0x80, 0x80+len) */
    uint8_t* ref = make_chks_buf(0x100, blk_len);
    memset(ref + 0x88, 0, 8);
    uint32_t h = apollo_hash_jenkins_oaat(ref + 0x80, blk_len, 0x3FAC7125);
    exp_hash[0] = (uint8_t)(h >> 24); exp_hash[1] = (uint8_t)(h >> 16);
    exp_hash[2] = (uint8_t)(h >> 8);  exp_hash[3] = (uint8_t)h;

    size_t n = apply_bsd(&buf, 0x100, "set [v]:rockstar_checksum");

    uint8_t exp_len[4] = { (uint8_t)(blk_len >> 24), (uint8_t)(blk_len >> 16),
                           (uint8_t)(blk_len >> 8),  (uint8_t)blk_len };

    CHECK_U64("rockstar: code applied", n, 0x100);
    CHECK_MEM("rockstar: checksum written at +0xC", buf + 0x8C, exp_hash, 4);
    CHECK_MEM("rockstar: block length restored at +0x8", buf + 0x88, exp_len, 4);
    free(buf);
    free(ref);
}

TEST(clamp_bsd_rockstar_chks_oob_record_skipped)
{
    /* blk_len 0x1000 puts the block start before the buffer and its end past it */
    uint8_t* buf = make_chks_buf(0x100, 0x1000);
    uint8_t* ref = make_chks_buf(0x100, 0x1000);

    size_t n = apply_bsd(&buf, 0x100, "set [v]:rockstar_checksum");

    CHECK_U64("rockstar OOB: code still completes", n, 0x100);
    CHECK_MEM("rockstar OOB: record left untouched", buf, ref, 0x100);
    free(buf);
    free(ref);
}
