/*
 * BSD encrypt/decrypt command vectors.
 *
 * The BSD script exposes ~18 crypto commands through two sibling blocks in
 * apollo_apply_bsd_code() — "decrypt <alg>" and "encrypt <alg>" — that are
 * identical apart from the direction. Only `aes_ecb` had any coverage (one
 * corpus fixture), so merging those blocks would have been unguarded.
 *
 * These vectors round-trip every command that has an inverse: encrypt then
 * decrypt over the same range must reproduce the input byte for byte, and the
 * encrypted form must differ from it (so a silently-skipped command can't pass).
 * The self-inverse streams are applied twice instead.
 *
 * Build-flag invariant: every algorithm here works on bytes, so the LE and BE
 * builds must agree.
 *
 * Not covered:
 *   `mgs_base64` — its encode step expands the data past the range it was
 *      handed, so it is not a safe in-place round-trip on a fixed buffer.
 *   `mgs_pw` — it operates on a whole MGS Peace Walker save at fixed offsets
 *      and refuses anything under 0x35998 bytes, so it cannot round-trip a
 *      small buffer. (Its size guard is also too low for what it then
 *      touches; that is a separate bug, not something these vectors cover.)
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "test_common.h"

#define CRYPT_LEN   64          /* multiple of 16 and 8: fits every block size */

/* 24-byte key (3-DES requires exactly this), also valid for AES-192 */
#define KEY24       "\"0123456789abcdef01234567\""
#define KEY32       "\"0123456789abcdef0123456789abcdef\""
#define IV16        "\"abcdefghijklmnop\""
#define IV8         "\"abcdefgh\""

static void seed(uint8_t* buf)
{
    for (int i = 0; i < CRYPT_LEN; i++)
        buf[i] = (uint8_t)(i * 7 + 3);
}

/* Runs one BSD script over `buf`; returns the size the engine reports. */
static size_t run_bsd(uint8_t** buf, const char* script)
{
    code_entry_t c = make_bsd_code(script);
    return apollo_apply_bsd_code(buf, CRYPT_LEN, &c);
}

/*
 * encrypt <cmd> then decrypt <cmd> over the whole buffer must restore it,
 * and the intermediate must not equal the input.
 */
static void round_trip(const char* label, const char* cmd)
{
    char script[512];
    uint8_t orig[CRYPT_LEN], mid[CRYPT_LEN];
    uint8_t* buf = malloc(CRYPT_LEN);
    size_t out;

    seed(orig);
    memcpy(buf, orig, CRYPT_LEN);

    apollo_free_var_list();     /* isolate variable + endianness state */

    snprintf(script, sizeof(script), "set range:0x0,0x%X\nencrypt %s", CRYPT_LEN, cmd);
    out = run_bsd(&buf, script);
    if (!check_u64(__FILE__, __LINE__, label, out != 0, 1))
    {
        free(buf);
        return;
    }
    memcpy(mid, buf, CRYPT_LEN);

    if (memcmp(mid, orig, CRYPT_LEN) == 0)
        check_u64(__FILE__, __LINE__, label, 0, 1);   /* encrypt was a no-op */

    apollo_free_var_list();

    snprintf(script, sizeof(script), "set range:0x0,0x%X\ndecrypt %s", CRYPT_LEN, cmd);
    out = run_bsd(&buf, script);
    check_u64(__FILE__, __LINE__, label, out != 0, 1);
    check_mem(__FILE__, __LINE__, label, buf, orig, CRYPT_LEN);

    free(buf);
}

/* Self-inverse streams: the same command applied twice restores the input. */
static void applied_twice(const char* label, const char* cmd)
{
    char script[512];
    uint8_t orig[CRYPT_LEN], mid[CRYPT_LEN];
    uint8_t* buf = malloc(CRYPT_LEN);

    seed(orig);
    memcpy(buf, orig, CRYPT_LEN);

    apollo_free_var_list();
    snprintf(script, sizeof(script), "set range:0x0,0x%X\nencrypt %s", CRYPT_LEN, cmd);
    check_u64(__FILE__, __LINE__, label, run_bsd(&buf, script) != 0, 1);
    memcpy(mid, buf, CRYPT_LEN);

    if (memcmp(mid, orig, CRYPT_LEN) == 0)
        check_u64(__FILE__, __LINE__, label, 0, 1);

    apollo_free_var_list();
    snprintf(script, sizeof(script), "set range:0x0,0x%X\ndecrypt %s", CRYPT_LEN, cmd);
    check_u64(__FILE__, __LINE__, label, run_bsd(&buf, script) != 0, 1);
    check_mem(__FILE__, __LINE__, label, buf, orig, CRYPT_LEN);

    free(buf);
}

/* ---- standard ciphers ---- */

TEST(bsd_crypt_aes_ecb)      { round_trip("aes_ecb",      "aes_ecb(" KEY32 ")"); }
TEST(bsd_crypt_aes_cbc)      { round_trip("aes_cbc",      "aes_cbc(" KEY32 "," IV16 ")"); }
TEST(bsd_crypt_aes_ctr)      { applied_twice("aes_ctr",   "aes_ctr(" KEY32 "," IV16 ")"); }
TEST(bsd_crypt_camellia_ecb) { round_trip("camellia_ecb", "camellia_ecb(" KEY32 ")"); }
TEST(bsd_crypt_des3_ecb)     { round_trip("des3_ecb",     "des3_ecb(" KEY24 ")"); }
TEST(bsd_crypt_des3_cbc)     { round_trip("des3_cbc",     "des3_cbc(" KEY24 "," IV8 ")"); }
TEST(bsd_crypt_blowfish_ecb) { round_trip("blowfish_ecb", "blowfish_ecb(" KEY32 ")"); }
TEST(bsd_crypt_blowfish_cbc) { round_trip("blowfish_cbc", "blowfish_cbc(" KEY32 "," IV8 ")"); }

/* ---- game-specific, no arguments ---- */

TEST(bsd_crypt_diablo3)        { round_trip("diablo3",        "diablo3"); }
TEST(bsd_crypt_silent_hill3)   { round_trip("silent_hill3",   "silent_hill3"); }
TEST(bsd_crypt_nfs_undercover) { round_trip("nfs_undercover", "nfs_undercover"); }

/* ---- game-specific, with arguments ---- */

TEST(bsd_crypt_mgs)          { round_trip("mgs",          "mgs(" KEY24 ")"); }
TEST(bsd_crypt_ffxiii)       { round_trip("ffxiii",       "ffxiii(1," KEY24 ")"); }
TEST(bsd_crypt_borderlands3) { round_trip("borderlands3", "borderlands3(1)"); }
TEST(bsd_crypt_monster_hunter) { round_trip("monster_hunter", "monster_hunter(2)"); }

/* ---- self-inverse XOR streams ---- */

TEST(bsd_crypt_rgg_studio) { applied_twice("rgg_studio", "rgg_studio(" KEY24 ")"); }
TEST(bsd_crypt_dw8xl)      { applied_twice("dw8xl",      "dw8xl"); }
TEST(bsd_crypt_mgs5_tpp)   { applied_twice("mgs5_tpp",   "mgs5_tpp(0x1234)"); }

/*
 * The command keyword is matched case-insensitively, and the corpus fixture
 * that exercises this path spells it in caps. Guard both spellings.
 */
TEST(bsd_crypt_keyword_is_case_insensitive)
{
    uint8_t orig[CRYPT_LEN];
    uint8_t* buf = malloc(CRYPT_LEN);
    char script[512];

    seed(orig);
    memcpy(buf, orig, CRYPT_LEN);

    apollo_free_var_list();
    snprintf(script, sizeof(script), "set range:0x0,0x%X\nENCRYPT aes_ecb(" KEY32 ")", CRYPT_LEN);
    CHECK_U64("uppercase ENCRYPT applied", run_bsd(&buf, script) != 0, 1);
    CHECK_U64("uppercase ENCRYPT changed data", memcmp(buf, orig, CRYPT_LEN) != 0, 1);

    apollo_free_var_list();
    snprintf(script, sizeof(script), "set range:0x0,0x%X\nDECRYPT aes_ecb(" KEY32 ")", CRYPT_LEN);
    CHECK_U64("uppercase DECRYPT applied", run_bsd(&buf, script) != 0, 1);
    CHECK_MEM("uppercase round-trip restored", buf, orig, CRYPT_LEN);

    free(buf);
}

/* An unknown algorithm must fall through both blocks without touching data. */
TEST(bsd_crypt_unknown_algorithm_is_inert)
{
    uint8_t orig[CRYPT_LEN];
    uint8_t* buf = malloc(CRYPT_LEN);
    char script[512];

    seed(orig);
    memcpy(buf, orig, CRYPT_LEN);

    apollo_free_var_list();
    snprintf(script, sizeof(script), "set range:0x0,0x%X\nencrypt not_a_cipher(\"x\")", CRYPT_LEN);
    run_bsd(&buf, script);
    CHECK_MEM("unknown encrypt left data alone", buf, orig, CRYPT_LEN);

    apollo_free_var_list();
    snprintf(script, sizeof(script), "set range:0x0,0x%X\ndecrypt not_a_cipher(\"x\")", CRYPT_LEN);
    run_bsd(&buf, script);
    CHECK_MEM("unknown decrypt left data alone", buf, orig, CRYPT_LEN);

    free(buf);
}

/*
 * Direction pinning.
 *
 * The round-trips above are symmetric: they would pass just as happily if the
 * block mapped "encrypt" to APOLLO_DECRYPT and vice versa, because
 * encrypt(decrypt(x)) == x as well. These two vectors tie each BSD keyword to
 * a specific library call, so an inverted mode fails.
 */
TEST(bsd_crypt_direction_is_not_inverted)
{
    static const char key[] = "0123456789abcdef0123456789abcdef";
    uint8_t orig[CRYPT_LEN], direct[CRYPT_LEN];
    uint8_t* buf = malloc(CRYPT_LEN);
    char script[512];

    /* what the library does for ENCRYPT, called straight */
    seed(orig);
    memcpy(direct, orig, CRYPT_LEN);
    apollo_crypt_aes_ecb(APOLLO_ENCRYPT, direct, CRYPT_LEN, (const uint8_t*)key, sizeof(key) - 1);

    /* what the BSD "encrypt" keyword does */
    memcpy(buf, orig, CRYPT_LEN);
    apollo_free_var_list();
    snprintf(script, sizeof(script), "set range:0x0,0x%X\nencrypt aes_ecb(" KEY32 ")", CRYPT_LEN);
    run_bsd(&buf, script);
    CHECK_MEM("BSD encrypt == apollo_crypt_aes_ecb(APOLLO_ENCRYPT)", buf, direct, CRYPT_LEN);

    /* and the same for DECRYPT */
    memcpy(direct, orig, CRYPT_LEN);
    apollo_crypt_aes_ecb(APOLLO_DECRYPT, direct, CRYPT_LEN, (const uint8_t*)key, sizeof(key) - 1);

    memcpy(buf, orig, CRYPT_LEN);
    apollo_free_var_list();
    snprintf(script, sizeof(script), "set range:0x0,0x%X\ndecrypt aes_ecb(" KEY32 ")", CRYPT_LEN);
    run_bsd(&buf, script);
    CHECK_MEM("BSD decrypt == apollo_crypt_aes_ecb(APOLLO_DECRYPT)", buf, direct, CRYPT_LEN);

    free(buf);
}
