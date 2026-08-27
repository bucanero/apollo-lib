/*
 * Known-answer vectors against real game saves.
 *
 * The synthetic vectors in test_crypt_bsd.c prove each cipher is reversible,
 * but a wrong-yet-reversible transform passes them just as well. These check
 * libapollo's output against reference plaintext produced by the community
 * decrypters, which is the only thing that proves it speaks the real format.
 *
 * Where the algorithm needs a non-trivial range, the vector drives the ACTUAL
 * script from the shipped .savepatch rather than a hand-computed offset. That
 * matters more than it sounds: the NFS Undercover engine fix (63f334a) and its
 * savepatch range are coupled, and only a script-driven vector sees both ends
 * of that. A hand-written offset would have hidden the coupling completely.
 *
 * Neither the saves nor the patches are vendored. Point the check at local
 * clones of https://github.com/bucanero/save-decrypters and, optionally,
 * https://github.com/bucanero/apollo-patches :
 *
 *     make check-samples SAMPLES=/path/to/save-decrypters
 *
 * Without SAMPLES set, every vector here skips.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "test_common.h"

static const char* samples_root(void)
{
    const char* r = getenv("SAMPLES");
    return (r && *r) ? r : NULL;
}

static uint8_t* slurp_at(const char* root, const char* rel, size_t* len)
{
    char path[2048];
    FILE* f;
    uint8_t* b;

    snprintf(path, sizeof(path), "%s/%s", root, rel);
    f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    *len = (size_t) ftell(f);
    fseek(f, 0, SEEK_SET);

    b = malloc(*len ? *len : 1);
    if (b && fread(b, 1, *len, f) != *len) { free(b); b = NULL; }
    fclose(f);
    return b;
}

/* Loads a sample pair, or reports why it is skipping. */
static int load_pair(const char* label, const char* rel_enc, const char* rel_dec,
                     uint8_t** enc, uint8_t** dec, size_t* n)
{
    const char* root = samples_root();
    size_t nenc = 0, ndec = 0;

    if (!root)
    {
        printf("        (skipped %s: set SAMPLES=/path/to/save-decrypters)\n", label);
        return 0;
    }

    *enc = slurp_at(root, rel_enc, &nenc);
    *dec = slurp_at(root, rel_dec, &ndec);
    if (!*enc || !*dec)
    {
        printf("        (skipped %s: sample pair not found)\n", label);
        free(*enc); free(*dec);
        return 0;
    }

    if (!check_u64(__FILE__, __LINE__, label, nenc, ndec))
    {
        free(*enc); free(*dec);
        return 0;
    }

    *n = nenc;
    return 1;
}

/* ---- whole-buffer algorithms (their savepatches use `set range:0,eof+1`) ---- */

typedef void (*apply_fn)(uint8_t* buf, size_t len, apollo_crypt_mode_t mode);

static void known_answer(const char* label, const char* rel_enc, const char* rel_dec, apply_fn apply)
{
    uint8_t *enc, *dec, *work;
    size_t n;

    if (!load_pair(label, rel_enc, rel_dec, &enc, &dec, &n))
        return;

    work = malloc(n);

    memcpy(work, enc, n);
    apply(work, n, APOLLO_DECRYPT);
    check_mem(__FILE__, __LINE__, label, work, dec, n);

    memcpy(work, dec, n);
    apply(work, n, APOLLO_ENCRYPT);
    check_mem(__FILE__, __LINE__, label, work, enc, n);

    free(work); free(enc); free(dec);
}

/*
 * Script-driven: runs the real .savepatch code bodies through the BSD engine.
 * `cmp_from` skips a leading region on the decrypt comparison only, for tools
 * that blank a checksum field the engine leaves alone (see silent_hill3).
 */
static void known_answer_script(const char* label, const char* rel_enc, const char* rel_dec,
                                const char* dec_script, const char* enc_script, size_t cmp_from)
{
    uint8_t *enc, *dec, *work;
    size_t n;
    code_entry_t c;

    if (!load_pair(label, rel_enc, rel_dec, &enc, &dec, &n))
        return;

    work = malloc(n);

    memcpy(work, enc, n);
    apollo_free_var_list();
    c = make_bsd_code(dec_script);
    check_u64(__FILE__, __LINE__, label, apollo_apply_bsd_code(&work, n, &c), n);
    check_mem(__FILE__, __LINE__, label, work + cmp_from, dec + cmp_from, n - cmp_from);

    memcpy(work, dec, n);
    apollo_free_var_list();
    c = make_bsd_code(enc_script);
    check_u64(__FILE__, __LINE__, label, apollo_apply_bsd_code(&work, n, &c), n);
    check_mem(__FILE__, __LINE__, label, work, enc, n);

    apollo_free_var_list();
    free(work); free(enc); free(dec);
}

/* ---- Diablo 3 (PS3 BLUS31188: `set range:0x000000,eof+1`) ---- */

static void apply_diablo3(uint8_t* b, size_t n, apollo_crypt_mode_t m)
{
    apollo_crypt_diablo3(m, b, (uint32_t) n);
}

TEST(sample_diablo3)
{
    known_answer("diablo3 ACCOUNT.DAT",
                 "diablo3-decrypter/samples/ACCOUNT.DAT.enc",
                 "diablo3-decrypter/samples/ACCOUNT.DAT.dec",
                 apply_diablo3);
}

/* ---- Monster Hunter PSP (ULJM05500: `set range:0x0000,EOF+1`) ---- */

static void apply_mh2(uint8_t* b, size_t n, apollo_crypt_mode_t m)
{
    apollo_crypt_monster_hunter(m, b, (uint32_t) n, 2);
}

static void apply_mh3(uint8_t* b, size_t n, apollo_crypt_mode_t m)
{
    apollo_crypt_monster_hunter(m, b, (uint32_t) n, 3);
}

TEST(sample_monster_hunter_v2)
{
    known_answer("monster_hunter MHP2NDG (ver 2)",
                 "monsterhunter-psp-decrypter/samples/MHP2NDG.BIN.ENC",
                 "monsterhunter-psp-decrypter/samples/MHP2NDG.BIN.DEC",
                 apply_mh2);
}

TEST(sample_monster_hunter_v3)
{
    known_answer("monster_hunter MHP3RD (ver 3)",
                 "monsterhunter-psp-decrypter/samples/MHP3RD.BIN.ENC",
                 "monsterhunter-psp-decrypter/samples/MHP3RD.BIN.DEC",
                 apply_mh3);
}

/* ---- MGS Peace Walker (NPUB30611: `set range:0x0000,eof+1`) ---- */

static void apply_mgspw(uint8_t* b, size_t n, apollo_crypt_mode_t m)
{
    apollo_crypt_mgs_pw(m, b, (uint32_t) n);
}

TEST(sample_mgs_pw)
{
    known_answer("mgs_pw 00000000.000",
                 "mgs-pw-decrypter/samples/00000000.000",
                 "mgs-pw-decrypter/samples/00000000.000.dec",
                 apply_mgspw);
}

/*
 * NFS Undercover — PS3/BLES00450 and PS3/BLUS30248.
 *
 * The range is read out of the save: a big-endian u32 at 0x64 holds the
 * payload length plus 20, the key seed sits at 0x70, and the payload follows.
 * The range must cover seed + payload, because the seed block is consumed as
 * the first block of the range.
 *
 * This is the vector that pins the engine/savepatch contract. Against the
 * pre-63f334a engine the range was one block short and the engine overran by
 * exactly one block, cancelling out; either half alone produces wrong output.
 */
static const char NFSU_DEC[] =
    "set pointer:read(0x64, 4)\n"
    "set range:0x000070,pointer+0x6B\n"
    "DECRYPT nfs_undercover";
static const char NFSU_ENC[] =
    "set pointer:read(0x64, 4)\n"
    "set range:0x000070,pointer+0x6B\n"
    "ENCRYPT nfs_undercover";

TEST(sample_nfs_undercover)
{
    known_answer_script("nfs_undercover USR-DATA",
                        "nfs-undercover-decrypter/samples/USR-DATA.enc",
                        "nfs-undercover-decrypter/samples/USR-DATA.dec",
                        NFSU_DEC, NFSU_ENC, 0);
}

/*
 * Dynasty Warriors 8 XL — PS3/BLAS50672. The range deliberately stops at
 * `eof-1`: the final byte is a checksum a separate code maintains, and the
 * XOR stream must not cover it.
 */
static const char DW8XL_DEC[] =
    "set [end]:eof-1\n"
    "set range:0x0000,[end]\n"
    "DECRYPT dw8xl";
static const char DW8XL_ENC[] =
    "set [end]:eof-1\n"
    "set range:0x0000,[end]\n"
    "ENCRYPT dw8xl";

TEST(sample_dw8xl)
{
    known_answer_script("dw8xl APP.BIN",
                        "dw8xl-decrypter/samples/APP.BIN",
                        "dw8xl-decrypter/samples/APP.BIN.dec",
                        DW8XL_DEC, DW8XL_ENC, 0);
}

/*
 * Borderlands 3 profile — PS4/CUSA07823. The range is not fixed: it is found
 * by searching for a class name, then sized from a length field relative to
 * the hit. This is the only sample vector exercising `search`.
 */
static const char BL3_DEC[] =
    "search \"BP_DefaultOakProfile_C\"\n"
    "set [size]:read(pointer+0x17,4)\n"
    "set [size]:endian_swap\n"
    "set [size]:[size]+0x1A\n"
    "set range:pointer+0x1B,pointer+[size]\n"
    "DECRYPT borderlands3(0)";
static const char BL3_ENC[] =
    "search \"BP_DefaultOakProfile_C\"\n"
    "set [size]:read(pointer+0x17,4)\n"
    "set [size]:endian_swap\n"
    "set [size]:[size]+0x1A\n"
    "set range:pointer+0x1B,pointer+[size]\n"
    "ENCRYPT borderlands3(0)";

TEST(sample_borderlands3_profile)
{
    known_answer_script("borderlands3 profile",
                        "ps4-borderlands3-decrypter/samples/profile.enc",
                        "ps4-borderlands3-decrypter/samples/profile.dec",
                        BL3_DEC, BL3_ENC, 0);
}

/*
 * Silent Hill 3 — PS3/BLUS30810. Crypto covers 0x40 onwards, and a separate
 * `dwadd` code maintains a checksum at 0x10.
 *
 * The encrypt direction is compared in full: running the checksum code and
 * then ENCRYPT over SAVEDATA.DAT.dec reproduces the original file exactly,
 * including the 0x10 checksum. The decrypt direction is compared from 0x40,
 * because the reference tool blanks 0x10..0x13 to zero in its output while the
 * engine leaves the field alone — that field is outside the crypto range.
 */
static const char SH3_DEC[] =
    "set pointer:read(0xC, 4)\n"
    "set range:0x000040,pointer+0x3F\n"
    "DECRYPT silent_hill3";
static const char SH3_ENC[] =
    "write at 0x000010:00000000\n"
    "set pointer:read(0xC, 4)\n"
    "set [csum]:dwadd(0x000000,pointer+0x3F)\n"
    "set [csum]:[csum]+[csum]+0x1\n"
    "write at 0x000010:[csum]\n"
    "set pointer:read(0xC, 4)\n"
    "set range:0x000040,pointer+0x3F\n"
    "ENCRYPT silent_hill3";

TEST(sample_silent_hill3)
{
    known_answer_script("silent_hill3 SAVEDATA.DAT",
                        "silent-hill3-decrypter/samples/SAVEDATA.DAT.enc",
                        "silent-hill3-decrypter/samples/SAVEDATA.DAT.dec",
                        SH3_DEC, SH3_ENC, 0x40);
}
