/*
 * BSD script vectors.
 *
 * BSD is fully endian-mode-invariant: reads and writes go through the
 * unconditional BE* macros, and integer truncation goes through
 * _set_var_slice(), which lays the value out big-endian and hands it to
 * _swap_var_endianness(). So every BSD vector uses a single shared expected
 * array, and the two passes must agree.
 *
 * Two rounds of fixes are guarded here. First, carry()-based truncation
 * (wadd/dwadd/add/sub) originally used the target-endian PADDING macro and
 * produced WRONG, divergent output for big-endian save data -- it sliced the
 * high half of the accumulator on a little-endian host. That was fixed by
 * keying the slice on the real HOST byte order instead; the vectors below with
 * hand-computed accumulators guard the corrected result.
 *
 * Keying on the host was necessary but not sufficient. It picks the right
 * BYTES on either host and leaves them in host ORDER, which only the widths
 * the variable reader converts (2, 4, 8) ever recover from -- so a 3-byte
 * slice still came out reversed between a PS3 and a wasm build, and was
 * unreachable to test. _set_var_slice() closed that, and
 * bsd_carry_truncation_is_big_endian() and
 * bsd_left_right_slices_are_big_endian() pin every width.
 * See tests/README.md.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

/* returns new size; *buf may be realloc'd by insert/delete */
static size_t apply_bsd(uint8_t** buf, size_t len, const char* codes)
{
    apollo_free_var_list();               /* isolate variable state per test */
    code_entry_t c = make_bsd_code(codes);
    return apollo_apply_bsd_code(buf, len, &c);
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
 * identical in both builds (regression guard for the PADDING fix).
 *
 * wadd(0x0,0x3) over {12 34 56 78} = be16(0x1234)+be16(0x5678) = 0x000068AC.
 * carry(2) keeps the LOW 16 bits, 0x68AC, which reaches the file big-endian as
 * [68 AC]. That is what a real PS3 and a real PS4/PC produce; before the fix
 * the __PS3_PC__ build wrongly kept the high half and wrote [00 00].
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
    CHECK_MEM("wadd carry(2) truncation", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * add() with carry — the second truncation site (patches.c add handler).
 * add(0x0,0x3) = 0xFF+0xFF+0xFF+0x04 = 0x00000301. carry(2) keeps the low 16
 * bits 0x0301, emitted big-endian as [03 01] in both modes. Before the fix the
 * big-endian build kept the high half -> [00 00].
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
    CHECK_MEM("add carry(2) truncation", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * right(value,len) — the third truncation site. Keeps the `len` rightmost
 * (least-significant) bytes of the value. right(0x12345678,2) -> 0x5678,
 * emitted big-endian as [56 78] in both modes. Before the fix the big-endian
 * build kept the LEFT bytes -> [12 34]. Width coverage beyond 2 bytes lives in
 * bsd_left_right_slices_are_big_endian().
 */
TEST(bsd_right_truncation)
{
    uint8_t init[16] = {0};
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [r]:right(0x12345678,2)\nwrite at 0:[r]");

    uint8_t exp[16] = {0};
    exp[0]=0x56; exp[1]=0x78;
    CHECK_MEM("right(v,2) keeps low bytes", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * left(value,len) — keeps the leftmost / MOST-significant `len` bytes of the
 * value, emitted big-endian by the write path. left(0x00012345,2) -> the top 2
 * bytes 0x0001 -> [00 01] in every build. Before the fix the little-endian
 * builds wrongly kept the low bytes (0x2345 -> [23 45], identical to right()).
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
 * Update of an already-existing variable — the fourth truncation site, where a
 * var's value is re-fetched into old_val and re-stored. The var is created
 * with read(), which is not a truncation site, so only the re-fetch is under
 * test here.
 *
 * read(0,2) of file bytes {AA BB} stores the value 0xAABB. The second set
 * re-fetches it (now via _get_var_value(), re-stored via _set_var_slice()),
 * endian_swap reverses it to 0xBBAA, and write emits it big-endian as [BB AA].
 * Same in both modes; before the fix the big-endian build re-pointed at the
 * high (zero) bytes -> [00 00]. The 3-byte width this pair has to survive is
 * covered by the two-step accumulate in bsd_carry_truncation_is_big_endian().
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
    CHECK_MEM("existing-var re-fetch truncation", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * A "[name]" reference must match the whole variable name, not a prefix of it.
 *
 * Both lookups resolve the name straight out of the script line, from the span
 * between the brackets, so the comparison is length-delimited (strncmp) rather
 * than a strcmp over a NUL-terminated copy. strncmp alone only answers "does
 * the stored name START with this span", so the lookup also has to check that
 * the stored name ends there.
 *
 * Without that check, declaring [ab] and then [a] does not produce two
 * variables at all: the declaration of [a] goes through the same lookup, finds
 * [ab], and overwrites it. So the tell is a later reference to the LONGER name
 * reading back the shorter one's value. Both vectors below are built that way,
 * and both cover one of the two call sites: value decoding
 * (_decode_variable_data) and integer parsing (_parse_int_value).
 *
 * The values are 1 and 3 bytes wide on purpose. A 2/4/8-byte variable is
 * converted to big-endian on its way out, which would fold an endianness
 * question into a name-resolution test.
 */
TEST(bsd_var_name_prefix_not_matched_as_value)
{
    uint8_t init[16] = {0};
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init),
              "set [ab]:AABBCC\nset [a]:112233\nwrite at 0:[a]\nwrite at 8:[ab]");

    uint8_t exp[16] = {0};
    exp[0]=0x11; exp[1]=0x22; exp[2]=0x33;    /* [a]                          */
    exp[8]=0xAA; exp[9]=0xBB; exp[10]=0xCC;   /* [ab], not clobbered by [a]   */
    CHECK_MEM("[a] and [ab] stay distinct variables", buf, exp, sizeof(exp));
    free(buf);
}

TEST(bsd_var_name_prefix_not_matched_as_offset)
{
    uint8_t init[16];
    uint8_t* buf;

    for (int i = 0; i < 16; i++) init[i] = (uint8_t) i;   /* 00..0F */
    buf = dup_bytes(init, sizeof(init));

    /* read([p],1) reads offset 8 and read([pp],1) offset 4, so the two bytes
     * written back name which variable each reference resolved to. */
    apply_bsd(&buf, sizeof(init),
              "set [pp]:0x00000004\nset [p]:0x00000008\n"
              "set [v]:read([p],1)\nset [w]:read([pp],1)\n"
              "write at 0:[v]\nwrite at 1:[w]");

    uint8_t exp[16];
    memcpy(exp, init, sizeof(exp));
    exp[0]=0x08;                 /* [p]  -> offset 8 */
    exp[1]=0x04;                 /* [pp] -> offset 4 */
    CHECK_MEM("[p] and [pp] resolve to their own offsets", buf, exp, sizeof(exp));
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

/* ======================================================================== */
/* read() of int16/int32/int64, and a few hash smoke tests                  */
/* ======================================================================== */

/*
 * read(offset,len) loads `len` bytes and, for 2/4/8-byte sizes, normalises them
 * from the file's big-endian order to a host-native value (via BE16/32/64). The
 * write path converts back, so a read->write round-trip reproduces the source
 * bytes verbatim on every build. These pin the int16/int32/int64 read widths.
 */
TEST(bsd_read_int16)
{
    uint8_t init[32] = {0};
    init[0]=0xAA; init[1]=0xBB;
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [v]:read(0,2)\nwrite at 8:[v]");

    uint8_t exp[32] = {0};
    exp[0]=0xAA; exp[1]=0xBB;
    exp[8]=0xAA; exp[9]=0xBB;
    CHECK_MEM("read(0,2) int16 round-trip", buf, exp, sizeof(exp));
    free(buf);
}

TEST(bsd_read_int32)
{
    uint8_t init[32] = {0};
    init[0]=0x11; init[1]=0x22; init[2]=0x33; init[3]=0x44;
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [v]:read(0,4)\nwrite at 8:[v]");

    uint8_t exp[32] = {0};
    memcpy(exp, init, 4);
    memcpy(exp + 8, init, 4);
    CHECK_MEM("read(0,4) int32 round-trip", buf, exp, sizeof(exp));
    free(buf);
}

TEST(bsd_read_int64)
{
    uint8_t init[32] = {0};
    for (int i = 0; i < 8; i++) init[i] = (uint8_t)(0x01 + i);   /* 01..08 */
    uint8_t* buf = dup_bytes(init, sizeof(init));

    apply_bsd(&buf, sizeof(init), "set [v]:read(0,8)\nwrite at 8:[v]");

    uint8_t exp[32] = {0};
    memcpy(exp, init, 8);
    memcpy(exp + 8, init, 8);
    CHECK_MEM("read(0,8) int64 round-trip", buf, exp, sizeof(exp));
    free(buf);
}

/*
 * Hash smoke tests over the ASCII input "123456789" (the classic CRC check
 * string). The 32-bit results are stored host-native and emitted big-endian, so
 * the written bytes are the big-endian form of the hash; all are build-invariant.
 *
 *  - crc32big  -> CRC-32/BZIP2, independently known check value 0xFC891918
 *  - sha1      -> independently known SHA-1("123456789") digest (20 bytes)
 *  - jhash     -> Jenkins hash; value characterised from the library (regression)
 */
static uint8_t* hash_buf(void)
{
    uint8_t* b = calloc(1, 64);
    memcpy(b, "123456789", 9);
    return b;
}

TEST(bsd_hash_crc32big)
{
    uint8_t* buf = hash_buf();
    apply_bsd(&buf, 64, "set range:0x0,0x8\nset [h]:crc32big\nwrite at 0x10:[h]");

    uint8_t exp[4] = { 0xFC, 0x89, 0x19, 0x18 };   /* CRC-32/BZIP2 check value */
    CHECK_MEM("crc32big(\"123456789\") = 0xFC891918", buf + 0x10, exp, sizeof(exp));
    free(buf);
}

TEST(bsd_hash_sha1)
{
    uint8_t* buf = hash_buf();
    apply_bsd(&buf, 64, "set range:0x0,0x8\nset [h]:sha1\nwrite at 0x20:[h]");

    /* SHA-1("123456789") */
    uint8_t exp[20] = {
        0xF7,0xC3,0xBC,0x1D,0x80,0x8E,0x04,0x73,0x2A,0xDF,
        0x67,0x99,0x65,0xCC,0xC3,0x4C,0xA7,0xAE,0x34,0x41
    };
    CHECK_MEM("sha1(\"123456789\")", buf + 0x20, exp, sizeof(exp));
    free(buf);
}

/* ======================================================================== */
/* Fletcher-16 / Fletcher-32                                                */
/* ======================================================================== */

/*
 * Published Fletcher check values.
 *
 * Fletcher-32 sums 16-bit LITTLE-endian words by definition, and that is fixed
 * in the implementation rather than taken from the host, so these vectors hold
 * in both the LE and the BE mode -- which is the property that lets a
 * savepatch using it produce the same hash on a PS3 as on a PS4.
 */
static const struct {
    const char* s;
    uint16_t    f16;
    uint32_t    f32;
} fletcher_vectors[] = {
    { "abcde",    0xC8F0, 0xF04FC729 },
    { "abcdef",   0x2057, 0x56502D2A },
    { "abcdefgh", 0x0627, 0xEBE19591 },
};

TEST(hash_fletcher_known_vectors)
{
    for (size_t i = 0; i < sizeof(fletcher_vectors) / sizeof(*fletcher_vectors); i++)
    {
        const uint8_t* d = (const uint8_t*) fletcher_vectors[i].s;
        size_t n = strlen(fletcher_vectors[i].s);

        CHECK_U64("fletcher16 published vector",
                  apollo_hash_fletcher16(d, n), fletcher_vectors[i].f16);
        CHECK_U64("fletcher32 published vector",
                  apollo_hash_fletcher32(d, n), fletcher_vectors[i].f32);
    }
}

/*
 * Both functions defer the modulo to the end of a block, sized so c1 cannot
 * overflow first (5802 bytes / 360 words). This buffer is 6000 bytes, so it
 * crosses that boundary in both -- a wrong block size or a dropped reduction
 * diverges here and nowhere in the short vectors above.
 *
 * The expected values were computed with exact arbitrary-precision arithmetic
 * and a single modulo at the very end, i.e. from Fletcher's definition rather
 * than from a second copy of the blocked algorithm.
 */
TEST(hash_fletcher_block_boundary)
{
    uint8_t* buf = malloc(6000);
    size_t i;

    for (i = 0; i < 6000; i++)
        buf[i] = (uint8_t)(i * 7 + 3);

    CHECK_U64("fletcher16 across the 5802-byte block boundary",
              apollo_hash_fletcher16(buf, 6000), 0x777F);
    CHECK_U64("fletcher32 across the 360-word block boundary",
              apollo_hash_fletcher32(buf, 6000), 0x7921C9B5);
    free(buf);
}

/*
 * An odd length contributes its last byte as the low half of a zero-padded
 * word. The textbook version rounds the length up and then reads that byte from
 * the buffer, one past the end of a caller-sized range; ASan catches that here.
 */
TEST(hash_fletcher32_odd_length_zero_padded)
{
    uint8_t three[3] = { 0x11, 0x22, 0x33 };

    /* words 2211, 0033 */
    CHECK_U64("fletcher32 odd length", apollo_hash_fletcher32(three, 3), 0x44552244);
}

/* An empty range must be well-defined, not a wrapped block length. */
TEST(hash_fletcher_empty_range)
{
    CHECK_U64("fletcher16 of nothing", apollo_hash_fletcher16((const uint8_t*)"", 0), 0);
    CHECK_U64("fletcher32 of nothing", apollo_hash_fletcher32((const uint8_t*)"", 0), 0);
}

/* The BSD commands, over "12345678" (8 bytes, so no padding is involved). */
TEST(bsd_hash_fletcher16)
{
    uint8_t* buf = hash_buf();
    apply_bsd(&buf, 64, "set range:0x0,0x7\nset [h]:fletcher16\nwrite at 0x10:[h]");

    uint8_t exp[2] = { 0x3F, 0xA5 };
    CHECK_MEM("fletcher16(\"12345678\") = 0x3FA5", buf + 0x10, exp, sizeof(exp));
    free(buf);
}

TEST(bsd_hash_fletcher32)
{
    uint8_t* buf = hash_buf();
    apply_bsd(&buf, 64, "set range:0x0,0x7\nset [h]:fletcher32\nwrite at 0x10:[h]");

    uint8_t exp[4] = { 0x0A, 0x00, 0xD4, 0xD0 };
    CHECK_MEM("fletcher32(\"12345678\") = 0x0A00D4D0", buf + 0x10, exp, sizeof(exp));
    free(buf);
}

TEST(bsd_hash_jhash)
{
    uint8_t* buf = hash_buf();
    apply_bsd(&buf, 64, "set range:0x0,0x8\nset [h]:jhash\nwrite at 0x10:[h]");

    uint8_t exp[4] = { 0x4B, 0xF8, 0x35, 0x26 };   /* characterised from the library */
    CHECK_MEM("jhash(\"123456789\") regression", buf + 0x10, exp, sizeof(exp));
    free(buf);
}

/*
 * md5_xor / sha1_xor64 — apollo-specific folded hashes (32- and 64-bit). Stored
 * host-native and emitted big-endian, so build-invariant. Values characterised
 * from the library over "123456789" (regression guards).
 */
TEST(bsd_hash_md5_xor)
{
    uint8_t* buf = hash_buf();
    apply_bsd(&buf, 64, "set range:0x0,0x8\nset [h]:md5_xor\nwrite at 0x10:[h]");

    uint8_t exp[4] = { 0xB8, 0xF7, 0x55, 0x89 };
    CHECK_MEM("md5_xor(\"123456789\") regression", buf + 0x10, exp, sizeof(exp));
    free(buf);
}

TEST(bsd_hash_sha1_xor64)
{
    uint8_t* buf = hash_buf();
    apply_bsd(&buf, 64, "set range:0x0,0x8\nset [h]:sha1_xor64\nwrite at 0x10:[h]");

    uint8_t exp[8] = { 0x7A, 0xB2, 0xEF, 0xC5, 0xE5, 0x42, 0xC7, 0x3F };
    CHECK_MEM("sha1_xor64(\"123456789\") regression", buf + 0x10, exp, sizeof(exp));
    free(buf);
}

/*
 * ---- host byte order must not reach the file ---------------------------
 *
 * Two vectors, and what makes them vectors is that neither branches on
 * apollo_test_be(): the expected bytes below are the answer on a PS3 and on a
 * wasm build alike. That is the whole claim. Before these fixes each case gave
 * one answer on a big-endian host and the reverse on a little-endian one, and
 * nothing in the suite said so.
 *
 * The thing that decides it is _decode_variable_data(), which converts a
 * variable on the way out BY LENGTH: 2, 4 and 8 bytes are treated as a
 * host-native integer and re-emitted big-endian, every other length is copied
 * through as a raw byte string. A producer has to know which side of that line
 * it is on.
 */

/* carry(1) truncates the accumulator to THREE bytes, the one width with no
 * case in that switch. _set_var_slice() is what makes it work anyway: the
 * value is laid out big-endian and then handed to _swap_var_endianness(),
 * which converts the widths the reader converts and leaves the rest alone. So
 * every width below has ONE expected answer rather than a host-dependent
 * pair. 16 bytes of 0xFF sum to 0xFF0. */
TEST(bsd_carry_truncation_is_big_endian)
{
    static const uint8_t init[16] = {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
    };
    static const uint8_t exp0[4] = { 0x00, 0x00, 0x0F, 0xF0 };
    static const uint8_t exp1[3] = { 0x00, 0x0F, 0xF0 };
    static const uint8_t exp2[2] = { 0x0F, 0xF0 };
    static const uint8_t exp3[1] = { 0xF0 };
    uint8_t* buf;

    /* A fresh copy before every case: the write lands at offset 0, inside the
     * range the next sum would cover, so reusing the buffer would silently
     * change the input rather than the width under test. */
#define CARRY_CASE(label, script, exp)                                        \
    do {                                                                      \
        buf = dup_bytes(init, sizeof(init));                                  \
        CHECK_U64(label ": applied", apply_bsd(&buf, sizeof(init), script),   \
                  sizeof(init));                                              \
        CHECK_MEM(label, buf, exp, sizeof(exp));                              \
        free(buf);                                                            \
    } while (0)

    CARRY_CASE("carry(0): four bytes, big-endian",
               "set [v]:0\nset [v]:add(0x0,0xF)\nwrite at 0:[v]", exp0);
    CARRY_CASE("carry(1): three bytes, big-endian",
               "carry(1)\nset [v]:0\nset [v]:add(0x0,0xF)\nwrite at 0:[v]", exp1);
    CARRY_CASE("carry(2): two bytes, big-endian",
               "carry(2)\nset [v]:0\nset [v]:add(0x0,0xF)\nwrite at 0:[v]", exp2);
    CARRY_CASE("carry(3): low byte",
               "carry(3)\nset [v]:0\nset [v]:add(0x0,0xF)\nwrite at 0:[v]", exp3);

    /* Accumulating in two steps must reach the same place as one. This is the
     * half that needs _get_var_value(): the second add() seeds itself from the
     * 3-byte variable the first one left, and reading that back as zero would
     * silently drop the first 2040. */
    CARRY_CASE("carry(1): 3-byte accumulator survives being re-set",
               "carry(1)\nset [v]:0\nset [v]:add(0x0,0x7)\nset [v]:add(0x8,0xF)\nwrite at 0:[v]",
               exp1);
#undef CARRY_CASE
}

/*
 * host_account_id is a byte string, handed over the same way as the PSID and
 * the MAC addresses beside it -- but it is EIGHT bytes, which is a length the
 * reader converts. So an id already in the order the save wants came back
 * reversed on a little-endian build. apollo_test_host_cb serves
 * 01 23 45 67 89 AB CD EF and that is what has to land in the file, here and
 * on a PS3.
 *
 * Driven through apollo_apply_code() rather than apollo_apply_bsd_code(),
 * because the host callback is installed by the former and there is no other
 * way to reach it.
 */
extern void* apollo_test_host_cb(int info, uint32_t* size);

TEST(bsd_host_account_id_is_big_endian)
{
    static const uint8_t want[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
    static const uint8_t zero[16] = {0};
    const char* tmpdir = getenv("TMPDIR");
    char tmp[4096];
    code_entry_t c;
    uint8_t* out = NULL;
    size_t len = 0;

    if (!tmpdir || !*tmpdir) tmpdir = "/tmp";
    snprintf(tmp, sizeof(tmp), "%s/apollo_acct_%d.bin", tmpdir, (int) getpid());
    if (write_buffer(tmp, zero, sizeof(zero)) != 0)
        return;

    memset(&c, 0, sizeof(c));
    c.type  = APOLLO_CODE_BSD;
    c.name  = (char*) "vector";
    c.file  = tmp;
    c.codes = (char*) "set [id]:host_account_id\nwrite at 0:[id]";

    apollo_free_var_list();
    CHECK_U64("host_account_id: code applied",
              apollo_apply_code(tmp, &c, apollo_test_host_cb) != 0, 1);

    if (read_buffer(tmp, &out, &len) == 0)
    {
        CHECK_MEM("host_account_id: written big-endian on every host",
                  out, want, sizeof(want));
        free(out);
    }
    unlink(tmp);
}

/*
 * right() and left() reach the same three-byte width from the other side:
 * there the 3 IS the requested width, not a count of bytes dropped. Every
 * width is pinned below with no apollo_test_be() branch, which is the claim
 * that _set_var_slice() removed the host from the answer -- a 3-byte slice
 * used to come out 22 33 44 on a little-endian build and 44 33 22 on a PS3.
 */
TEST(bsd_left_right_slices_are_big_endian)
{
    static const uint8_t init[16] = {0};
    static const uint8_t right1[1] = { 0x44 };
    static const uint8_t right2[2] = { 0x33, 0x44 };
    static const uint8_t right3[3] = { 0x22, 0x33, 0x44 };
    static const uint8_t right4[4] = { 0x11, 0x22, 0x33, 0x44 };
    static const uint8_t left1[1]  = { 0x11 };
    static const uint8_t left2[2]  = { 0x11, 0x22 };
    static const uint8_t left3[3]  = { 0x11, 0x22, 0x33 };
    uint8_t* buf = dup_bytes(init, sizeof(init));

    CHECK_U64("right(,1) applied", apply_bsd(&buf, sizeof(init),
              "set [v]:right(0x11223344,1)\nwrite at 0:[v]"), sizeof(init));
    CHECK_MEM("right(,1): lowest byte", buf, right1, sizeof(right1));

    CHECK_U64("right(,2) applied", apply_bsd(&buf, sizeof(init),
              "set [v]:right(0x11223344,2)\nwrite at 0:[v]"), sizeof(init));
    CHECK_MEM("right(,2): low half, big-endian", buf, right2, sizeof(right2));

    CHECK_U64("right(,3) applied", apply_bsd(&buf, sizeof(init),
              "set [v]:right(0x11223344,3)\nwrite at 0:[v]"), sizeof(init));
    CHECK_MEM("right(,3): low three bytes, big-endian", buf, right3, sizeof(right3));

    CHECK_U64("right(,4) applied", apply_bsd(&buf, sizeof(init),
              "set [v]:right(0x11223344,4)\nwrite at 0:[v]"), sizeof(init));
    CHECK_MEM("right(,4): whole value, big-endian", buf, right4, sizeof(right4));

    CHECK_U64("left(,1) applied", apply_bsd(&buf, sizeof(init),
              "set [v]:left(0x11223344,1)\nwrite at 0:[v]"), sizeof(init));
    CHECK_MEM("left(,1): highest byte", buf, left1, sizeof(left1));

    CHECK_U64("left(,2) applied", apply_bsd(&buf, sizeof(init),
              "set [v]:left(0x11223344,2)\nwrite at 0:[v]"), sizeof(init));
    CHECK_MEM("left(,2): high half, big-endian", buf, left2, sizeof(left2));

    CHECK_U64("left(,3) applied", apply_bsd(&buf, sizeof(init),
              "set [v]:left(0x11223344,3)\nwrite at 0:[v]"), sizeof(init));
    CHECK_MEM("left(,3): high three bytes, big-endian", buf, left3, sizeof(left3));
    free(buf);
}

/*
 * The shape apollo-patches actually ships, and the one every carry vector
 * above misses: PSP/ULUS10579 (BlazBlue: Continuum Shift II) sums a whole
 * SYSTEM.DAT with wadd() under carry(2).
 *
 *   set [csum]:0
 *   carry(2)
 *   set pointer:eof+1
 *   set [csum]:wadd(0x000004,pointer)
 *   set [csum]:xor:FFFF
 *   write at 0x000000:[csum]
 *
 * The other vectors sum four or sixteen bytes, so their accumulator never
 * passes 0xFFFF and the fold loop
 *
 *   while (carry > 0 && add > 0xFFFF)
 *       add = (add & 0xFFFF) + ((add & 0xFFFF0000) >> 8*carry);
 *
 * never executes once. A real save runs it, and these two sizes run it once
 * and twice respectively. They discriminate: skip the fold and 0x2000 would
 * write the raw low half 0x0BDE ^ FFFF = F421 instead of E3E5 ^ FFFF = 1C1A.
 *
 * Expected values computed from the algorithm by hand, not captured from this
 * engine. Both sums stay under 2^32, so the uint32_t accumulator does not wrap
 * and the arithmetic is unambiguous.
 *
 * xor:FFFF also pins the storage contract from the other side: _bitwise_var_value
 * refuses a length mismatch, so the variable has to be exactly the two bytes
 * carry(2) leaves, held host-native the way that helper expects to find it.
 */
static void carry_fold_case(size_t n, uint8_t hi, uint8_t lo, const char* label)
{
    uint8_t* init = malloc(n);
    uint8_t* buf;
    uint8_t exp[2];

    for (size_t i = 0; i < n; i++)
        init[i] = (uint8_t) (i * 7 + 3);

    buf = dup_bytes(init, n);
    exp[0] = hi; exp[1] = lo;

    check_u64(__FILE__, __LINE__, label,
              apply_bsd(&buf, n,
                        "set [csum]:0\n"
                        "carry(2)\n"
                        "set pointer:eof+1\n"
                        "set [csum]:wadd(0x000004,pointer)\n"
                        "set [csum]:xor:FFFF\n"
                        "write at 0x000000:[csum]"), n);
    check_mem(__FILE__, __LINE__, label, buf, exp, sizeof(exp));
    /* everything past the checksum is untouched */
    check_mem(__FILE__, __LINE__, label, buf + 2, init + 2, n - 2);

    free(buf);
    free(init);
}

TEST(bsd_carry_fold_over_long_buffer)
{
    carry_fold_case(0x2000,  0x1C, 0x1A, "ULUS10579 shape, 8KB: wadd carry(2), fold runs once");
    carry_fold_case(0x20000, 0x93, 0xA2, "ULUS10579 shape, 128KB: wadd carry(2), fold runs twice");
}

/*
 * Seed-then-accumulate, the other real shape: PS3/BLUS30863 (Champion Jockey)
 * primes the variable with a constant and then adds a range into it.
 *
 *   set [csum]:0x190518
 *   set [csum]:add(0x000010,0x190527)
 *   write at 0xC:[csum]
 *
 * The second `set` overwrites a variable that already exists, which is the
 * path _get_var_value() reads and _set_var_slice() writes back -- the one
 * place the slice rewrite could have dropped a value silently rather than
 * reversing it. add() seeds its accumulator from what comes out, so a
 * mis-read seed is invisible except as a wrong checksum.
 *
 * Both of the patch's branches, with its own constants: the 42KB SYSTEM range
 * and the 1.6MB STORY one. No carry() here, so the variable stays four bytes
 * wide and the write emits it big-endian. Expected values computed from the
 * algorithm; neither sum wraps the uint32_t accumulator.
 */
static void seed_then_add_case(uint32_t seed, uint32_t start, uint32_t end,
                               const uint8_t exp[4], const char* label)
{
    size_t n = (size_t) end + 1;
    uint8_t* init = malloc(n);
    uint8_t* buf;
    char script[160];

    for (size_t i = 0; i < n; i++)
        init[i] = (uint8_t) (i * 7 + 3);

    buf = dup_bytes(init, n);
    snprintf(script, sizeof(script),
             "set [csum]:0x%06X\nset [csum]:add(0x%06X,0x%06X)\nwrite at 0xC:[csum]",
             seed, start, end);

    check_u64(__FILE__, __LINE__, label, apply_bsd(&buf, n, script), n);
    check_mem(__FILE__, __LINE__, label, buf + 0xC, exp, 4);
    /* the write lands before the summed range, so the input is untouched */
    check_mem(__FILE__, __LINE__, label, buf + 0x10, init + 0x10, n - 0x10);

    free(buf);
    free(init);
}

TEST(bsd_seed_then_add_existing_variable)
{
    static const uint8_t sys_exp[4]   = { 0x00, 0x53, 0x67, 0x74 };
    static const uint8_t story_exp[4] = { 0x0C, 0x8F, 0x11, 0xEC };

    seed_then_add_case(0xA628, 0x10, 0xA637, sys_exp,
                       "BLUS30863 SYSTEM: seed 0xA628 + add(0x10,0xA637)");
    seed_then_add_case(0x190518, 0x10, 0x190527, story_exp,
                       "BLUS30863 STORY: seed 0x190518 + add(0x10,0x190527)");
}
