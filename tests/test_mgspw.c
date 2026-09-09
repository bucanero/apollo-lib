/*
 * MGS Peace Walker crypto vectors.
 *
 * mgspw operates on a whole PS3 Peace Walker save at hard-coded offsets, so it
 * can't be exercised with the small buffers the other crypto vectors use. Two
 * bounds bugs lived here precisely because nothing tested it:
 *
 *   - the size guard was 0x35998 while the code reaches 0x44AE8, so a save in
 *     that window overflowed the heap by up to 0xF150 bytes;
 *   - mgspw_SetSalts() derives a read offset from the save's own bytes, so a
 *     tampered header pointed it anywhere — a segfault at a perfectly valid
 *     file size, which no size guard could have caught.
 *
 * The vectors below cover both with synthetic buffers, so they run everywhere
 * and need no game data. Correctness — that the output is actually a valid
 * decryption — needs a real save and is opt-in:
 *
 *     make check-mgspw MGSPW_SAVE=/path/to/00000000.000
 *
 * pointing at an encrypted save with its decrypted twin alongside as
 * <file>.dec. Real save data is deliberately not vendored: it is ~300 KB of
 * binary, six times the whole fixtures tree, and it is somebody's game data.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "test_common.h"

/*
 * Deepest byte either direction touches, per save type; mirrors the min_size
 * values mgspw_GetLayout() derives in decrypt.c. A PS3 save has to reach the
 * end of its second encrypted block; a PSP save only the end of the main one.
 */
#define MGSPW_MIN         0x44AE8    /* APOLLO_MGSPW_PS3    */
#define MGSPW_MIN_PSP     0x359D8    /* APOLLO_MGSPW_PSP    */
#define MGSPW_MIN_PSP_JP  0x359C8    /* APOLLO_MGSPW_PSP_JP */

static uint8_t* zeros(size_t n) { return calloc(1, n); }

static int all_zero(const uint8_t* b, size_t n)
{
    for (size_t i = 0; i < n; i++)
        if (b[i]) return 0;
    return 1;
}

/* One byte under the minimum must be refused outright, not partially processed. */
TEST(mgspw_rejects_undersized_buffer)
{
    size_t n = MGSPW_MIN - 1;
    uint8_t* buf = zeros(n);

    apollo_crypt_mgs_pw(APOLLO_DECRYPT, buf, (uint32_t) n, APOLLO_MGSPW_PS3);
    CHECK_U64("undersized decrypt left the buffer untouched", all_zero(buf, n), 1);

    apollo_crypt_mgs_pw(APOLLO_ENCRYPT, buf, (uint32_t) n, APOLLO_MGSPW_PS3);
    CHECK_U64("undersized encrypt left the buffer untouched", all_zero(buf, n), 1);

    free(buf);
}

/*
 * PSP support is, at bottom, a second and lower size guard. The PS3 minimum
 * (0x44AE8) is the end of the PS3-only second encrypted block, which is past
 * the end of every real PSP save (0x3D9D0 standard, 0x3D9C0 JP) -- sharing it
 * is exactly how PSP saves used to be turned away untouched.
 *
 * A buffer at the PSP minimum is therefore below the PS3 one: refused when
 * typed as PS3, accepted when typed as PSP. The fill is pseudo-random rather
 * than zeros so "accepted" is observable — the leading SwapBlock byte-swaps
 * every word it covers, so the buffer must change. That SwapBlock spans the
 * full swap_words, right up to min_size, so under ASan this also pins that the
 * derived bound is not too LARGE.
 */
TEST(mgspw_psp_guard_is_independent_of_ps3)
{
    size_t n = MGSPW_MIN_PSP;
    uint8_t* buf = malloc(n);
    uint8_t* orig = malloc(n);

    fill_lcg(buf, n, 0x5A5A0001);
    memcpy(orig, buf, n);

    apollo_crypt_mgs_pw(APOLLO_DECRYPT, buf, (uint32_t) n, APOLLO_MGSPW_PS3);
    CHECK_U64("PSP-sized buffer refused when typed as PS3", memcmp(buf, orig, n) == 0, 1);

    apollo_crypt_mgs_pw(APOLLO_DECRYPT, buf, (uint32_t) n, APOLLO_MGSPW_PSP);
    CHECK_U64("PSP-sized buffer accepted when typed as PSP", memcmp(buf, orig, n) != 0, 1);

    free(buf);
    free(orig);
}

/* One byte under the PSP minimum is still refused, in both PSP layouts. */
TEST(mgspw_psp_rejects_undersized_buffer)
{
    size_t n = MGSPW_MIN_PSP_JP - 1;
    uint8_t* buf = malloc(n);
    uint8_t* orig = malloc(n);

    fill_lcg(buf, n, 0x5A5A0002);
    memcpy(orig, buf, n);

    apollo_crypt_mgs_pw(APOLLO_DECRYPT, buf, (uint32_t) n, APOLLO_MGSPW_PSP);
    CHECK_U64("undersized PSP decrypt left the buffer untouched", memcmp(buf, orig, n) == 0, 1);

    apollo_crypt_mgs_pw(APOLLO_ENCRYPT, buf, (uint32_t) n, APOLLO_MGSPW_PSP_JP);
    CHECK_U64("undersized PSP-JP encrypt left the buffer untouched", memcmp(buf, orig, n) == 0, 1);

    free(buf);
    free(orig);
}

/*
 * At exactly the minimum the code runs to its deepest access. Under ASan this
 * is the vector that catches the old 0xF150-byte overflow; without it, it at
 * least proves the boundary size is accepted rather than crashing.
 */
TEST(mgspw_accepts_minimum_size)
{
    size_t n = MGSPW_MIN;
    uint8_t* buf = zeros(n);

    /* An all-zero header makes SetSalts compute offset 0xAD47DE8F, which is out
     * of range, so the salts step refuses and decrypt returns early — but only
     * after the first SwapBlock has run over the buffer. That is the access the
     * old guard let run past the end. */
    apollo_crypt_mgs_pw(APOLLO_DECRYPT, buf, (uint32_t) n, APOLLO_MGSPW_PS3);
    CHECK_U64("minimum-size decrypt completed without a bounds error", 1, 1);

    free(buf);
}

/*
 * A hostile salt offset at a perfectly valid file size. All-zero header words
 * give offset = (0 | 0xAD47DE8F) ^ 0 = 0xAD47DE8F, which would index roughly
 * 2.9 GB past the buffer.
 */
TEST(mgspw_rejects_out_of_range_salt_offset)
{
    size_t n = MGSPW_MIN;
    uint8_t* buf = zeros(n);

    apollo_crypt_mgs_pw(APOLLO_DECRYPT, buf, (uint32_t) n, APOLLO_MGSPW_PS3);
    CHECK_U64("hostile salt offset did not read out of bounds (decrypt)", 1, 1);

    apollo_crypt_mgs_pw(APOLLO_ENCRYPT, buf, (uint32_t) n, APOLLO_MGSPW_PS3);
    CHECK_U64("hostile salt offset did not read out of bounds (encrypt)", 1, 1);

    free(buf);
}

/* ---- opt-in: correctness against a real save ---- */

static uint8_t* slurp(const char* path, size_t* len)
{
    FILE* f = fopen(path, "rb");
    uint8_t* b;

    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *len = (size_t) ftell(f);
    fseek(f, 0, SEEK_SET);

    b = malloc(*len ? *len : 1);
    if (b && fread(b, 1, *len, f) != *len) { free(b); b = NULL; }
    fclose(f);
    return b;
}

/*
 * Decrypting a real save must reproduce its known-good plaintext byte for
 * byte, and re-encrypting that must reproduce the original file. Self-
 * consistency alone would not catch a wrong-but-reversible transform, which is
 * why the reference .dec is required rather than optional.
 */
TEST(mgspw_real_save_round_trip)
{
    const char* path = getenv("MGSPW_SAVE");
    char decpath[1024];
    uint8_t *enc, *dec, *work;
    size_t nenc = 0, ndec = 0;

    if (!path || !*path)
    {
        printf("        (skipped: set MGSPW_SAVE=/path/to/00000000.000)\n");
        return;
    }

    enc = slurp(path, &nenc);
    if (!CHECK_U64("MGSPW_SAVE readable", enc != NULL, 1))
        return;

    snprintf(decpath, sizeof(decpath), "%s.dec", path);
    dec = slurp(decpath, &ndec);
    if (!CHECK_U64("reference <save>.dec readable", dec != NULL, 1))
    {
        free(enc);
        return;
    }

    CHECK_U64("encrypted and reference are the same length", nenc, ndec);
    CHECK_U64("real save is at least the minimum size", nenc >= MGSPW_MIN, 1);

    if (nenc == ndec && nenc >= MGSPW_MIN)
    {
        work = malloc(nenc);
        memcpy(work, enc, nenc);

        apollo_crypt_mgs_pw(APOLLO_DECRYPT, work, (uint32_t) nenc, APOLLO_MGSPW_PS3);
        check_mem(__FILE__, __LINE__, "decrypt(save) == reference .dec", work, dec, nenc);

        apollo_crypt_mgs_pw(APOLLO_ENCRYPT, work, (uint32_t) nenc, APOLLO_MGSPW_PS3);
        check_mem(__FILE__, __LINE__, "encrypt(decrypt(save)) == original", work, enc, nenc);

        free(work);
    }

    free(enc);
    free(dec);
}
