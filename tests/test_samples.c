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
#include <unistd.h>
#include <dirent.h>
#include "test_common.h"

extern void* apollo_test_host_cb(int info, uint32_t* size);

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

/* ------------------------------------------------------------------------ *
 *  Python vectors.
 *
 *  Nine of the save-decrypters tools have no BSD equivalent at all: their
 *  savepatch does the work in a [PYTHON:...] code that imports a helper module
 *  from apollo-patches/python. test_corpus.c skips every Python code, so
 *  without these the whole MicroPython path runs unexercised by the suite --
 *  and it is the path that half the PS4 decrypters in the database take.
 *
 *  These have to go through apollo_apply_code() rather than
 *  apollo_apply_py_code(): the import path is built from the HOST callback
 *  (APOLLO_HOST_DATA_PATH + "python"), and apollo_apply_code() is what installs
 *  that callback. So the driver writes the sample to a temp file, applies, and
 *  reads it back, which is exactly what a front-end does.
 *
 *  They need the apollo-patches checkout too, so they carry their own opt-in:
 *
 *      make check-samples SAMPLES=/path/to/save-decrypters \
 *                         PATCHES=/path/to/apollo-patches
 *
 *  With SAMPLES but no PATCHES, only these skip; the rest still run.
 * ------------------------------------------------------------------------ */

static char g_py_data_path[2048];

/* APOLLO_HOST_DATA_PATH has "python" appended to it verbatim, so it has to
 * carry its own trailing slash. Everything else defers to the shared stub. */
static void* py_host_cb(int info, uint32_t* size)
{
    if (info == APOLLO_HOST_DATA_PATH)
    {
        if (size) *size = (uint32_t) strlen(g_py_data_path);
        return g_py_data_path;
    }
    return apollo_test_host_cb(info, size);
}

/*
 * The Python vectors need the helper modules from apollo-patches, not just any
 * directory: tests/Makefile defaults PATCHES to `fixtures`, which is the
 * vendored savepatch set for the corpus target and has no python/ in it.
 * Pointing the engine at that would surface as an ImportError from inside the
 * patch -- a test FAILURE that looks like a broken decrypter -- so check for
 * the modules and skip cleanly when they are not there.
 */
static int py_modules_available(const char* patches)
{
    char path[2048];
    DIR* d;

    if (!patches || !*patches) return 0;

    snprintf(path, sizeof(path), "%s/python", patches);
    d = opendir(path);
    if (!d) return 0;

    closedir(d);
    return 1;
}

static const char* py_tmp_path(void)
{
    static char tmpl[4096];

    if (!tmpl[0])
    {
        const char* tmpdir = getenv("TMPDIR");
        if (!tmpdir || !*tmpdir) tmpdir = "/tmp";
        snprintf(tmpl, sizeof(tmpl), "%s/apollo_sample_py_%d.bin", tmpdir, (int) getpid());
    }
    return tmpl;
}

/*
 * Applies a NULL-terminated CHAIN of Python codes to `in`, leaving the result
 * in `out`/`out_len`.
 *
 * One apply per script, through the file, rather than concatenating the
 * bodies -- because those are not the same thing. The engine hands each code
 * `savedata` as a fresh bytearray and writes back whatever the body leaves
 * there, so a code that REASSIGNS it (`savedata = uzlib.compress(...)`,
 * `savedata = ctx.decrypt_cbc(...)`) leaves an immutable `bytes` behind. That
 * is fine at the end of a code -- the engine just reads the buffer out -- and
 * fatal in the middle of a concatenated one, because the next statement gets
 * bytes where it needs a bytearray:
 *
 *     TypeError: 'bytes' object does not support item assignment
 *
 * FF Pixel Remaster hits exactly that between its Compress and Encrypt codes.
 * Applying them separately is what a front-end does, and it is what works.
 */
static int py_apply(const char* const* scripts, const uint8_t* in, size_t n,
                    uint8_t** out, size_t* out_len)
{
    const char* tmp = py_tmp_path();
    uint8_t* buf = NULL;
    size_t len = n;
    int i;

    if (write_buffer(tmp, in, n) != 0)
        return 0;

    for (i = 0; scripts[i]; i++)
    {
        code_entry_t c;

        memset(&c, 0, sizeof(c));
        c.type  = APOLLO_CODE_PYTHON;
        c.name  = (char*) "vector";
        c.file  = (char*) tmp;
        c.codes = (char*) scripts[i];

        apollo_free_var_list();
        if (!apollo_apply_code(tmp, &c, py_host_cb))
            return 0;
    }

    if (read_buffer(tmp, &buf, &len) != 0)
        return 0;

    *out = buf;
    *out_len = len;
    return 1;
}

/*
 * Python known-answer pair.
 *
 * Two properties, because they are not equally strong:
 *
 *   1. decrypt(.enc) == .dec, byte for byte. This is the known answer, and it
 *      is what proves the engine speaks the format.
 *
 *   2. decrypt(encrypt(.dec)) == .dec -- a round-trip, not a comparison
 *      against .enc. Several of these patches COMPRESS on the way out, and an
 *      LZ77/deflate stream is not canonical: re-compressing Max Payne 3's
 *      plaintext yields 17202 bytes where the sample's own compressor
 *      produced 18108, and both decompress to the same save. Asserting
 *      encrypt(.dec) == .enc there would be asserting that two compressors
 *      agree, which is not a property of this library.
 *
 * `canonical` opts into the stronger encrypt(.dec) == .enc for the transforms
 * that ARE reproducible, so those keep full known-answer coverage in both
 * directions.
 *
 * `rt_cmp_from` skips a leading header on the round-trip only, for the
 * compressing patches whose header carries a checksum or hash OVER the
 * compressed stream: a different-but-valid stream gives a different checksum,
 * so those few bytes cannot come back identical while the save body does.
 *
 * Lengths are deliberately not required to match between .enc and .dec: a
 * compressing patch legitimately decrypts to something larger.
 */
static void known_answer_python(const char* label, const char* rel_enc, const char* rel_dec,
                                const char* const* dec_chain, const char* const* enc_chain,
                                int canonical, size_t rt_cmp_from)
{
    const char* root = samples_root();
    const char* patches = getenv("PATCHES");
    uint8_t *enc = NULL, *dec = NULL, *got = NULL, *back = NULL;
    size_t nenc = 0, ndec = 0, ngot = 0, nback = 0;

    if (!root)
    {
        printf("        (skipped %s: set SAMPLES=/path/to/save-decrypters)\n", label);
        return;
    }
    if (!py_modules_available(patches))
    {
        printf("        (skipped %s: set PATCHES=/path/to/apollo-patches)\n", label);
        return;
    }
    snprintf(g_py_data_path, sizeof(g_py_data_path), "%s/", patches);

    enc = slurp_at(root, rel_enc, &nenc);
    dec = slurp_at(root, rel_dec, &ndec);
    if (!enc || !dec)
    {
        printf("        (skipped %s: sample pair not found)\n", label);
        free(enc); free(dec);
        return;
    }

    /* 1. known answer */
    if (!py_apply(dec_chain, enc, nenc, &got, &ngot))
        check_u64(__FILE__, __LINE__, label, 0, 1);
    else
    {
        if (check_u64(__FILE__, __LINE__, label, ngot, ndec))
            check_mem(__FILE__, __LINE__, label, got, dec, ndec);
        free(got); got = NULL;
    }

    /* 2. re-encrypt, then decrypt what came back */
    if (!py_apply(enc_chain, dec, ndec, &got, &ngot))
        check_u64(__FILE__, __LINE__, label, 0, 1);
    else
    {
        if (canonical)
        {
            if (check_u64(__FILE__, __LINE__, label, ngot, nenc))
                check_mem(__FILE__, __LINE__, label, got, enc, nenc);
        }

        if (!py_apply(dec_chain, got, ngot, &back, &nback))
            check_u64(__FILE__, __LINE__, label, 0, 1);
        else
        {
            if (check_u64(__FILE__, __LINE__, label, nback, ndec))
                check_mem(__FILE__, __LINE__, label, back + rt_cmp_from,
                          dec + rt_cmp_from, ndec - rt_cmp_from);
            free(back);
        }
        free(got);
    }

    apollo_free_var_list();
    unlink(py_tmp_path());
    free(enc); free(dec);
}

/*
 * Max Payne 3 — PS3/BLUS30557. Compresses, so no canonical encrypt; and its
 * header holds a checksum at 0x10 plus an encrypted hash block at 0x18..0x2B,
 * both computed over the compressed buffer, so the round-trip is compared
 * from 0x2C on. Everything from there -- the whole save body -- comes back
 * byte for byte.
 */

static const char MAXPAYNE3_DEC[] =
    "import maxpayne3\n"
    "maxpayne3.maxpayne3_crypt(maxpayne3.MODE_DECRYPT, savedata)";
static const char MAXPAYNE3_ENC[] =
    "import maxpayne3\n"
    "maxpayne3.maxpayne3_crypt(maxpayne3.MODE_ENCRYPT, savedata)";

static const char* const MAXPAYNE3_DEC_C[] = { MAXPAYNE3_DEC, NULL };
static const char* const MAXPAYNE3_ENC_C[] = { MAXPAYNE3_ENC, NULL };

TEST(sample_maxpayne3)
{
    known_answer_python("python maxpayne3 RAGE.SAV",
                        "maxpayne3-decrypter/samples/RAGE.SAV.enc",
                        "maxpayne3-decrypter/samples/RAGE.SAV.dec",
                        MAXPAYNE3_DEC_C, MAXPAYNE3_ENC_C, 0, 0x2C);
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

/*
 * MGS Peace Walker — one vector per save type.
 *
 * The save type is always named: there is no un-parameterised `mgs_pw`, and the
 * shipped PS3 savepatches (NPUB30611/NPEB00686) carry `mgs_pw(0)` to match.
 */
static const char MGSPW_PS3_DEC[] =
    "set range:0x0000,eof+1\n"
    "DECRYPT mgs_pw(0)";
static const char MGSPW_PS3_ENC[] =
    "set range:0x0000,eof+1\n"
    "ENCRYPT mgs_pw(0)";

TEST(sample_mgs_pw_ps3)
{
    known_answer_script("mgs_pw PS3 00000000.000",
                        "mgs-pw-decrypter/samples/00000000.000",
                        "mgs-pw-decrypter/samples/00000000.000.dec",
                        MGSPW_PS3_DEC, MGSPW_PS3_ENC, 0);
}

/*
 * The PSP vectors need their own driver, because libapollo deliberately leaves
 * the decrypted header byte-swapped for PSP saves too, where the reference tool
 * swaps the first 17 words back so its output header reads little-endian. One
 * convention for both platforms means a savepatch reads the header fields the
 * same way on PS3 and PSP, and it leaves PS3 output identical to previous
 * releases — the cost is that libapollo's PSP plaintext differs from the tool's
 * in those 0x44 bytes.
 *
 * So all three properties get asserted:
 *
 *   1. the payload past 0x44 matches the reference plaintext exactly;
 *   2. the header is precisely the word-swapped reference header, which is what
 *      separates "documented convention" from "corrupted";
 *   3. re-encrypting libapollo's OWN plaintext reproduces the original file
 *      byte for byte — the lossless round-trip that actually matters, since
 *      decrypt -> patch -> encrypt is what a client does.
 *
 * Property 3 is why the encrypt half cannot come from the reference .dec the
 * way known_answer_script() does it: apollo's encrypt is the inverse of
 * apollo's decrypt, and feeding it the tool's differently-framed header would
 * fail for that reason alone.
 */
#define MGSPW_HDR_LEN   0x44

static void known_answer_mgspw_psp(const char* label, const char* rel_enc, const char* rel_dec,
                                   const char* dec_script, const char* enc_script)
{
    uint8_t *enc, *dec, *work;
    size_t n;
    int i, j, hdr_ok = 1;
    code_entry_t c;

    if (!load_pair(label, rel_enc, rel_dec, &enc, &dec, &n))
        return;

    work = malloc(n);
    memcpy(work, enc, n);

    apollo_free_var_list();
    c = make_bsd_code(dec_script);
    check_u64(__FILE__, __LINE__, label, apollo_apply_bsd_code(&work, n, &c), n);

    /* 1. payload */
    check_mem(__FILE__, __LINE__, label,
              work + MGSPW_HDR_LEN, dec + MGSPW_HDR_LEN, n - MGSPW_HDR_LEN);

    /* 2. header framing */
    for (i = 0; i < MGSPW_HDR_LEN; i += 4)
        for (j = 0; j < 4; j++)
            if (work[i + j] != dec[i + (3 - j)]) hdr_ok = 0;
    check_u64(__FILE__, __LINE__, "PSP header is the word-swapped reference header", hdr_ok, 1);

    /* 3. round-trip, from apollo's own plaintext */
    apollo_free_var_list();
    c = make_bsd_code(enc_script);
    check_u64(__FILE__, __LINE__, label, apollo_apply_bsd_code(&work, n, &c), n);
    check_mem(__FILE__, __LINE__, label, work, enc, n);

    apollo_free_var_list();
    free(work); free(enc); free(dec);
}

static const char MGSPW_PSP_DEC[] =
    "set range:0x0000,eof+1\n"
    "DECRYPT mgs_pw(1)";
static const char MGSPW_PSP_ENC[] =
    "set range:0x0000,eof+1\n"
    "ENCRYPT mgs_pw(1)";

TEST(sample_mgs_pw_psp)
{
    known_answer_mgspw_psp("mgs_pw PSP US/EU",
                           "mgs-pw-decrypter/samples/00000000.000.PSP.enc",
                           "mgs-pw-decrypter/samples/00000000.000.PSP.dec",
                           MGSPW_PSP_DEC, MGSPW_PSP_ENC);
}

static const char MGSPW_PSP_JP_DEC[] =
    "set range:0x0000,eof+1\n"
    "DECRYPT mgs_pw(2)";
static const char MGSPW_PSP_JP_ENC[] =
    "set range:0x0000,eof+1\n"
    "ENCRYPT mgs_pw(2)";

/*
 * The JP digital build's layout is 0x10 bytes shorter throughout. Decrypting it
 * with the standard offsets gets the shared prefix right and then diverges, so
 * this is the vector that pins the compact layout.
 */
TEST(sample_mgs_pw_psp_jp)
{
    known_answer_mgspw_psp("mgs_pw PSP JP digital",
                           "mgs-pw-decrypter/samples/00000000.000.PSP-JP.enc",
                           "mgs-pw-decrypter/samples/00000000.000.PSP-JP.dec",
                           MGSPW_PSP_JP_DEC, MGSPW_PSP_JP_ENC);
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

/* ------------------------------------------------------------------------ *
 *  Vectors below cover the remaining save-decrypters tools that ship an
 *  .enc/.dec sample pair. Each drives the real scripts from the shipped
 *  .savepatch, and the encrypt direction runs the patch's checksum codes
 *  first where it has them, in file order -- which is the order a front-end
 *  applying "every required code" produces, so these cover that path too.
 * ------------------------------------------------------------------------ */

/* ---- Call of Duty: Black Ops — PS3/BLES01031 ---- */

static const char COD_BO_DEC[] =
    "set range:0x000000,eof+1\n"
    "DECRYPT des3_cbc(\"Md8ea20lPcftYwsl496q63x9\", \"0Peyx825\")";
static const char COD_BO_ENC[] =
    "set range:0x000000,eof+1\n"
    "ENCRYPT des3_cbc(\"Md8ea20lPcftYwsl496q63x9\", \"0Peyx825\")";

TEST(sample_cod_blackops)
{
    known_answer_script("des3_cbc GPAD0_CM.PRF",
                        "cod-blackops-decrypter/samples/GPAD0_CM.PRF.enc",
                        "cod-blackops-decrypter/samples/GPAD0_CM.PRF.dec",
                        COD_BO_DEC, COD_BO_ENC, 0);
}

/* ---- DmC: Devil May Cry — PS3/BLES01698 ---- */

static const char DMC_DEC[] =
    "set range:0x000000,eof+1\n"
    "DECRYPT blowfish_ecb(\"8jdf*jsd@dfd8*P:QcNt\")";
static const char DMC_ENC[] =
    "set pointer:read(0x18, 4)\n"
    "set range:0x00001C,pointer+0x1B\n"
    "set [hash]:SHA1\n"
    "write at 0x4:[hash]\n"
    "set range:0x000000,eof+1\n"
    "ENCRYPT blowfish_ecb(\"8jdf*jsd@dfd8*P:QcNt\")";

TEST(sample_dmc)
{
    known_answer_script("blowfish_ecb PROFILE",
                        "dmc-decrypter/samples/PROFILE.enc",
                        "dmc-decrypter/samples/PROFILE.dec",
                        DMC_DEC, DMC_ENC, 0);
}

/* ---- Resident Evil HD Remaster / RE0 — PS3/NPEB02226 ---- */

static const char RE_REMASTER_DEC[] =
    "set range:0x000000,eof+1\n"
    "DECRYPT blowfish_ecb(\"SBmdYgEamc=#sA0)Mhs9#>/4iiXbMPxW\")";
static const char RE_REMASTER_ENC[] =
    "set pointer:eof+1\n"
    "set range:0x000040,pointer\n"
    "set [hash]:SHA1\n"
    "write at 0x00000C:[hash]\n"
    "set range:0x000000,eof+1\n"
    "ENCRYPT blowfish_ecb(\"SBmdYgEamc=#sA0)Mhs9#>/4iiXbMPxW\")";

TEST(sample_re_remaster)
{
    known_answer_script("blowfish_ecb DATA0.DAT",
                        "re-remaster-decrypter/samples/DATA0.DAT.enc",
                        "re-remaster-decrypter/samples/DATA0.DAT.dec",
                        RE_REMASTER_DEC, RE_REMASTER_ENC, 0);
}

/* ---- Shin Megami Tensei V — PS4/CUSA42502 ---- */

static const char SMT5_DEC[] =
    "set range:0x0000,EOF+1\n"
    "DECRYPT aes_ecb(\"0123456789abcdef0123456789abcdef\")";
static const char SMT5_ENC[] =
    "set pointer:EOF+1\n"
    "set range:0x000040,pointer\n"
    "set [hash]:SHA1\n"
    "write at 0x0000:[hash]\n"
    "set range:0x0000,EOF+1\n"
    "ENCRYPT aes_ecb(\"0123456789abcdef0123456789abcdef\")";

TEST(sample_smt5)
{
    known_answer_script("aes_ecb Megaten5.ps4.sav",
                        "smt5-decrypter/samples/Megaten5.ps4.sav.enc",
                        "smt5-decrypter/samples/Megaten5.ps4.sav.dec",
                        SMT5_DEC, SMT5_ENC, 0);
}

/* ---- Patapon 3 — PSP/UCES01421 ---- */

static const char PATAPON3_DEC[] =
    "set range:0x0000,EOF+1\n"
    "DECRYPT camellia_ecb(\"SVsyE56pniSRS9dIPTiE8ApDaUnN0AEa\")";
static const char PATAPON3_ENC[] =
    "set [len]:read(EOF-0x27, 4)\n"
    "set [len]:endian_swap\n"
    "set range:0x0000,[len]-1\n"
    "set [hash]:hmac_sha1(\"CyZ2o3SPBqMWVVvUVt4WwJOBpq9hCjNq\")\n"
    "set pointer:EOF-0x23\n"
    "write next (0):[hash]\n"
    "set range:0x0000,EOF+1\n"
    "ENCRYPT camellia_ecb(\"SVsyE56pniSRS9dIPTiE8ApDaUnN0AEa\")";

TEST(sample_patapon3)
{
    known_answer_script("camellia_ecb SECURE.BIN",
                        "patapon3-decrypter/samples/SECURE.BIN.ENC",
                        "patapon3-decrypter/samples/SECURE.BIN.DEC",
                        PATAPON3_DEC, PATAPON3_ENC, 0);
}

/*
 * Need For Speed: Rivals — PS3/BLES01894.
 *
 * The crypto range here is pinned to 0x57803 rather than the shipped script's
 * `eof+1`, because the SAMPLE is inconsistent, not the engine.
 *
 * USR-DATA is 716800 bytes, but its ciphertext only covers 0x4..0x57803
 * (358400 bytes); everything past that is zero in BOTH .enc and .dec, so it
 * was never encrypted. The savepatch and the C tool agree with each other and
 * would both cover the whole file — `set range:4,eof+1` block-truncates to
 * [4, 716796), and nfs-rivals-decrypter/main.c passes (data+4, len-8), which
 * is the same [4, 716796) — so either one run over the .dec re-encrypts the
 * padding and cannot reproduce the .enc.
 *
 * Over the range the fixture actually covers, the engine matches it byte for
 * byte in both directions, and the CRC code below runs verbatim from the
 * patch over the whole file and reproduces the stored hash. So this vector
 * still proves blowfish_ecb and the custom-CRC parameters; what it cannot
 * prove is the range, until the pair is regenerated from a real save.
 */

static const char NFS_RIVALS_DEC[] =
    "set range:0x000004,0x057803\n"
    "DECRYPT blowfish_ecb(0x2391F201B36C85E81B1272D690FFA545)";
static const char NFS_RIVALS_ENC[] =
    "set range:0x000004,0x057803\n"
    "ENCRYPT blowfish_ecb(0x2391F201B36C85E81B1272D690FFA545)\n"
    "set crc_bandwidth:32\n"
    "set crc_polynomial:0x4C11DB7\n"
    "set crc_initial_value:0xE195D3B7\n"
    "set crc_output_xor:0xFFFFFFFF\n"
    "set crc_reflection_input:1\n"
    "set crc_reflection_output:1\n"
    "set range:0x00004,eof+1\n"
    "set [hash]:crc\n"
    "write at 0x0:[hash]";

TEST(sample_nfs_rivals)
{
    known_answer_script("blowfish_ecb USR-DATA",
                        "nfs-rivals-decrypter/samples/USR-DATA.enc",
                        "nfs-rivals-decrypter/samples/USR-DATA.dec",
                        NFS_RIVALS_DEC, NFS_RIVALS_ENC, 0);
}

/* ---- Like a Dragon: Ishin! — PS4/CUSA32171 ---- */

static const char RGG_DEC[] =
    "set pointer:EOF-0x0F\n"
    "set range:0x00,pointer\n"
    "DECRYPT rgg_studio(\"fuEw5rWN8MBS\")";
static const char RGG_ENC[] =
    "set pointer:EOF-0x0F\n"
    "set range:0x00,pointer\n"
    "set [crc]:crc32\n"
    "set [crc]:endian_swap\n"
    "write next 0x08:[crc]\n"
    "set pointer:EOF-0x0F\n"
    "set range:0x00,pointer\n"
    "ENCRYPT rgg_studio(\"fuEw5rWN8MBS\")";

TEST(sample_rgg_studio)
{
    known_answer_script("rgg_studio data.ps4.sav",
                        "rgg-decrypter/samples/data.ps4.sav.enc",
                        "rgg-decrypter/samples/data.ps4.sav.dec",
                        RGG_DEC, RGG_ENC, 0);
}

/* ---- Red Dead Redemption 2 — PS4/CUSA08519 ---- */

static const char RDR2_DEC[] =
    "set range:0x000120,EOF+1\n"
    "DECRYPT aes_ecb(0x1685FFA38D010F0DFE661CF9B5572C500D802648DB37B9ED0F48C57342C022F5)";
static const char RDR2_ENC[] =
    "set range:0x000000,eof+1\n"
    "set [chks]:rockstar_checksum\n"
    "set range:0x000120,EOF+1\n"
    "ENCRYPT aes_ecb(0x1685FFA38D010F0DFE661CF9B5572C500D802648DB37B9ED0F48C57342C022F5)";

TEST(sample_rdr2)
{
    known_answer_script("aes_ecb SRDR30000",
                        "rdr2-decrypter/samples/SRDR30000.enc",
                        "rdr2-decrypter/samples/SRDR30000.dec",
                        RDR2_DEC, RDR2_ENC, 0);
}

/* ---- Grand Theft Auto V (PS3) — PS3/BLES01807 ---- */

static const char GTA5_PS3_DEC[] =
    "set range:0x000000,eof+1\n"
    "DECRYPT aes_ecb(0x1685FFA38D010F0DFE661CF9B5572C500D802648DB37B9ED0F48C57342C022F5)";
static const char GTA5_PS3_ENC[] =
    "set range:0x000000,eof+1\n"
    "set [chks]:rockstar_checksum\n"
    "set range:0x000000,eof+1\n"
    "ENCRYPT aes_ecb(0x1685FFA38D010F0DFE661CF9B5572C500D802648DB37B9ED0F48C57342C022F5)";

TEST(sample_gta5_ps3)
{
    known_answer_script("aes_ecb RAGE.SAV",
                        "gta5-decrypter/samples/RAGE.SAV.enc",
                        "gta5-decrypter/samples/RAGE.SAV.dec",
                        GTA5_PS3_DEC, GTA5_PS3_ENC, 0);
}

/*
 * Grand Theft Auto V (PS4) — PS4/CUSA00411. The range is read from the file:
 * a length at 0x108 sizes the region starting at 0x114.
 */
static const char GTA5_PS4_DEC[] =
    "set [end]:read(0x108,4)\n"
    "set range:0x000114,[end]+0x0113\n"
    "DECRYPT aes_ecb(0x1685FFA38D010F0DFE661CF9B5572C500D802648DB37B9ED0F48C57342C022F5)";
static const char GTA5_PS4_ENC[] =
    "set range:0x000000,eof+1\n"
    "set [chks]:rockstar_checksum\n"
    "set [end]:read(0x108,4)\n"
    "set range:0x000114,[end]+0x0113\n"
    "ENCRYPT aes_ecb(0x1685FFA38D010F0DFE661CF9B5572C500D802648DB37B9ED0F48C57342C022F5)";

TEST(sample_gta5_ps4)
{
    known_answer_script("aes_ecb memory.dat",
                        "gta5-decrypter/samples/memory.dat.enc",
                        "gta5-decrypter/samples/memory.dat.dec",
                        GTA5_PS4_DEC, GTA5_PS4_ENC, 0);
}

/*
 * Dark Souls Remastered — PS4/CUSA08495. The CBC IV is read from the head of
 * the file itself, so decrypt and encrypt both start by loading it.
 */
static const char DARKSOULS_DEC[] =
    "set [iv_cbc]:read(0x00, 0x10)\n"
    "set range:0x0010,EOF-0x10\n"
    "decrypt AES_CBC(0x20EC4B7519C2BD15E70C1EE4B204B8CB, [iv_cbc])";
static const char DARKSOULS_ENC[] =
    "set [iv_cbc]:read(0x00, 0x10)\n"
    "set range:0x0010,EOF-0x10\n"
    "encrypt AES_CBC(0x20EC4B7519C2BD15E70C1EE4B204B8CB, [iv_cbc])\n"
    "set pointer:EOF-0x0F\n"
    "set range:0x0000,pointer\n"
    "set [hash]:MD5\n"
    "write next 0x00:[hash]";

TEST(sample_darksouls_remastered)
{
    known_answer_script("aes_cbc userdata0000",
                        "darksouls-remaster-decrypter/samples/userdata0000.enc",
                        "darksouls-remaster-decrypter/samples/userdata0000.dec",
                        DARKSOULS_DEC, DARKSOULS_ENC, 0);
}

TEST(sample_darksouls_remastered_10)
{
    known_answer_script("aes_cbc userdata0010",
                        "darksouls-remaster-decrypter/samples/userdata0010.enc",
                        "darksouls-remaster-decrypter/samples/userdata0010.dec",
                        DARKSOULS_DEC, DARKSOULS_ENC, 0);
}

/* ---- Resident Evil Revelations 2 (PS3) — PS3/BLES02040 ---- */

static const char REREV2_PS3_DEC[] =
    "set range:0x000010,eof+1\n"
    "DECRYPT blowfish_ecb(\"zW$2eWaHNdT~6j86T_&j\")";
static const char REREV2_PS3_ENC[] =
    "set pointer:eof-0x1F\n"
    "set [csum]:0\n"
    "set [csum]:dwadd(0x000010,pointer)\n"
    "write at 0x000008:[csum]\n"
    "set pointer:eof-0x1F\n"
    "set range:0x000010,pointer\n"
    "set [hash]:SHA1\n"
    "write next (0):[hash]\n"
    "set range:0x000010,eof+1\n"
    "ENCRYPT blowfish_ecb(\"zW$2eWaHNdT~6j86T_&j\")";

TEST(sample_re_revelations2_ps3)
{
    known_answer_script("blowfish_ecb DATA0.DAT (PS3)",
                        "re-revelations2-decrypter/samples/DATA0.DAT.enc",
                        "re-revelations2-decrypter/samples/DATA0.DAT.dec",
                        REREV2_PS3_DEC, REREV2_PS3_ENC, 0);
}

/*
 * Resident Evil Revelations 2 (PS4) — PS4/CUSA00924. Same cipher and key as
 * the PS3 save, wrapped in `endian_swap(4)` either side because the PS4 file
 * stores the blocks the other way round.
 */
static const char REREV2_PS4_DEC[] =
    "set range:0x0020,eof+1\n"
    "endian_swap(4)\n"
    "DECRYPT blowfish_ecb(\"zW$2eWaHNdT~6j86T_&j\")\n"
    "endian_swap(4)";
static const char REREV2_PS4_ENC[] =
    "set range:0x0020,eof+1\n"
    "endian_swap(4)\n"
    "ENCRYPT blowfish_ecb(\"zW$2eWaHNdT~6j86T_&j\")\n"
    "endian_swap(4)";

TEST(sample_re_revelations2_ps4)
{
    known_answer_script("blowfish_ecb data1.dat (PS4)",
                        "re-revelations2-decrypter/samples/data1.dat.enc",
                        "re-revelations2-decrypter/samples/data1.dat.dec",
                        REREV2_PS4_DEC, REREV2_PS4_ENC, 0);
}

/*
 * Resident Evil 2 / 4 Remake — PS4/CUSA09193 and PS4/CUSA33388. Same script
 * shape, different key: the body is blowfish_cbc, and the 0x10..0x1F header is
 * then re-framed by undoing the CBC pass over it and applying ECB instead.
 */
#define RE_REMAKE_DEC(key)                                          \
    "set range:0x0010,EOF-0x04\n"                                   \
    "endian_swap(4)\n"                                              \
    "decrypt blowfish_cbc(\"" key "\", 0x0000000000000000)\n"       \
    "endian_swap(4)\n"                                              \
    "set range:0x0010,0x001F\n"                                     \
    "endian_swap(4)\n"                                              \
    "encrypt blowfish_cbc(\"" key "\", 0x0000000000000000)\n"       \
    "decrypt blowfish_ecb(\"" key "\")\n"                           \
    "endian_swap(4)"

#define RE_REMAKE_ENC(key)                                          \
    "set range:0x0010,0x001F\n"                                     \
    "endian_swap(4)\n"                                              \
    "encrypt blowfish_ecb(\"" key "\")\n"                           \
    "decrypt blowfish_cbc(\"" key "\", 0x0000000000000000)\n"       \
    "endian_swap(4)\n"                                              \
    "set range:0x0010,EOF-0x04\n"                                   \
    "endian_swap(4)\n"                                              \
    "encrypt blowfish_cbc(\"" key "\", 0x0000000000000000)\n"       \
    "endian_swap(4)\n"                                              \
    "set pointer:EOF-0x03\n"                                        \
    "set range:0x0000,pointer\n"                                    \
    "set [hash]:murmur3_32:0xFFFFFFFF\n"                            \
    "set [hash]:endian_swap\n"                                      \
    "write next 0x00:[hash]"

#define RE2R_KEY "K<>$cl%isqA|~nV4W5~3z_Q)j]5DHdB9sb{cI9Hn&Gqc-zO8O6zf"
#define RE4R_KEY "wa9Ui_tFKa_6E_D5gVChjM69xMKDX8QxEykYKhzb4cRNLknpCZUra"

static const char RE2R_DEC[] = RE_REMAKE_DEC(RE2R_KEY);
static const char RE2R_ENC[] = RE_REMAKE_ENC(RE2R_KEY);
static const char RE4R_DEC[] = RE_REMAKE_DEC(RE4R_KEY);
static const char RE4R_ENC[] = RE_REMAKE_ENC(RE4R_KEY);

TEST(sample_re2_remake)
{
    known_answer_script("blowfish_cbc RE2R-data000.bin",
                        "ps4-re4r-decrypter/samples/RE2R-data000.bin.enc",
                        "ps4-re4r-decrypter/samples/RE2R-data000.bin.dec",
                        RE2R_DEC, RE2R_ENC, 0);
}

TEST(sample_re4_remake)
{
    known_answer_script("blowfish_cbc RE4R a",
                        "ps4-re4r-decrypter/samples/a.enc",
                        "ps4-re4r-decrypter/samples/a.dec",
                        RE4R_DEC, RE4R_ENC, 0);
}

/*
 * Same as known_answer_script(), with the engine's global byte order pinned
 * for the duration.
 *
 * BSD codes take their byte order from apollo_set_endianness() alone -- the
 * per-code APOLLO_CODE_FLAG_ORDER_* flags are read by apollo_apply_sw_code()
 * and by nothing on the BSD path -- so for a BSD algorithm that is not
 * endian-invariant, the HOST picks the mode. apollo_apply_code() does not do
 * it either: front-ends call apollo_set_endianness() themselves from the
 * save's platform (apctl_set_big_endian() in bucanero/apollo-patcher does
 * exactly this, and the web front-end drives it from the patch's directory).
 *
 * The setting also has to be re-asserted after every
 * apollo_free_var_list(), which resets it back to the host's own byte order
 * -- so this driver sets it immediately before each apply, exactly as
 * apctl_apply() does, rather than once around the pair.
 *
 * These are the vectors that hold that contract in place: drop either half
 * of it and the MGS5 PS3 saves below stop decoding, which is precisely the
 * failure a front-end that got this wrong would ship.
 */
static void known_answer_script_endian(const char* label, const char* rel_enc, const char* rel_dec,
                                       const char* dec_script, const char* enc_script,
                                       size_t cmp_from, int big_endian)
{
    uint8_t *enc, *dec, *work;
    size_t n;
    code_entry_t c;
    int saved = apollo_get_endianness();
    int mode = big_endian ? APOLLO_DATA_MODE_BIG : APOLLO_DATA_MODE_LITTLE;

    if (!load_pair(label, rel_enc, rel_dec, &enc, &dec, &n))
        return;

    work = malloc(n);

    memcpy(work, enc, n);
    apollo_free_var_list();
    apollo_set_endianness(mode);
    c = make_bsd_code(dec_script);
    check_u64(__FILE__, __LINE__, label, apollo_apply_bsd_code(&work, n, &c), n);
    check_mem(__FILE__, __LINE__, label, work + cmp_from, dec + cmp_from, n - cmp_from);

    memcpy(work, dec, n);
    apollo_free_var_list();
    apollo_set_endianness(mode);
    c = make_bsd_code(enc_script);
    check_u64(__FILE__, __LINE__, label, apollo_apply_bsd_code(&work, n, &c), n);
    check_mem(__FILE__, __LINE__, label, work, enc, n);

    apollo_free_var_list();
    apollo_set_endianness(saved);
    free(work); free(enc); free(dec);
}

/*
 * Metal Gear Solid V — one vector per key variant and per platform branch.
 *
 * MGS5_TPP is a xorshift keystream, so the same op both ways; the PS3 and PS4
 * branches differ only in whether each word is byte-swapped around the XOR.
 * The key is per title ID (md5 of a title string), which is why the shipped
 * savepatches carry it as a literal — these vectors take each one from the
 * patch for the title the sample came from.
 *
 * Every .dec below carries a valid MD5 of 0x10..EOF in its first 16 bytes, so
 * running the patch's checksum code before ENCRYPT is a no-op on a clean
 * sample and the round-trip still proves the code does not corrupt it.
 */
#define MGS5_DEC(key)           \
    "set range:0x0000,EOF+1\n"  \
    "DECRYPT MGS5_TPP(" key ")"
#define MGS5_ENC(key)           \
    "set range:0x0010,EOF+1\n"  \
    "set [hash]:MD5\n"          \
    "write at 0x0000:[hash]\n"  \
    "set range:0x0000,EOF+1\n"  \
    "ENCRYPT MGS5_TPP(" key ")"

static const char MGS5_TPP_NPUB_DEC[] = MGS5_DEC("0x1FBAB234");
static const char MGS5_TPP_NPUB_ENC[] = MGS5_ENC("0x1FBAB234");
static const char MGS5_TPP_NPEB_DEC[] = MGS5_DEC("0x7B0ED589");
static const char MGS5_TPP_NPEB_ENC[] = MGS5_ENC("0x7B0ED589");
static const char MGS5_GZ_NPEB_DEC[]  = MGS5_DEC("0xAE88976A");
static const char MGS5_GZ_NPEB_ENC[]  = MGS5_ENC("0xAE88976A");
static const char MGS5_GZ_NPUB_DEC[]  = MGS5_DEC("0xC6A8B93D");
static const char MGS5_GZ_NPUB_ENC[]  = MGS5_ENC("0xC6A8B93D");
static const char MGS5_PS4_DEC[]      = MGS5_DEC("0x4131F8BE");
static const char MGS5_PS4_ENC[]      = MGS5_ENC("0x4131F8BE");

TEST(sample_mgs5_tpp_ps3_npub)
{
    known_answer_script_endian("mgs5_tpp TPP_GAM0 (PS3 NPUB31594)",
                        "mgs5-tpp-decrypter/samples/TPP_GAM0.enc",
                        "mgs5-tpp-decrypter/samples/TPP_GAM0.dec",
                        MGS5_TPP_NPUB_DEC, MGS5_TPP_NPUB_ENC, 0, 1);
}

TEST(sample_mgs5_tpp_ps3_npeb)
{
    known_answer_script_endian("mgs5_tpp TPP_GAM0 (PS3 NPEB02140)",
                        "mgs5-tpp-decrypter/samples/TPP_GAM0.NPEB.enc",
                        "mgs5-tpp-decrypter/samples/TPP_GAM0.NPEB.dec",
                        MGS5_TPP_NPEB_DEC, MGS5_TPP_NPEB_ENC, 0, 1);
}

TEST(sample_mgs5_gz_ps3_npeb)
{
    known_answer_script_endian("mgs5_tpp SAVE0.GZ (PS3 NPEB01889)",
                        "mgs5-tpp-decrypter/samples/SAVE0.GZ.NPEB.enc",
                        "mgs5-tpp-decrypter/samples/SAVE0.GZ.NPEB.dec",
                        MGS5_GZ_NPEB_DEC, MGS5_GZ_NPEB_ENC, 0, 1);
}

TEST(sample_mgs5_gz_ps3_npub)
{
    known_answer_script_endian("mgs5_tpp SAVE1.GZ (PS3 NPUB31318)",
                        "mgs5-tpp-decrypter/samples/SAVE1.GZ.NPUB.enc",
                        "mgs5-tpp-decrypter/samples/SAVE1.GZ.NPUB.dec",
                        MGS5_GZ_NPUB_DEC, MGS5_GZ_NPUB_ENC, 0, 1);
}

TEST(sample_mgs5_tpp_ps4_game)
{
    known_answer_script_endian("mgs5_tpp TPP_GAME_DATA0 (PS4 CUSA01140)",
                        "mgs5-tpp-decrypter/samples/TPP_GAME_DATA0.enc",
                        "mgs5-tpp-decrypter/samples/TPP_GAME_DATA0.dec",
                        MGS5_PS4_DEC, MGS5_PS4_ENC, 0, 0);
}

TEST(sample_mgs5_tpp_ps4_config)
{
    known_answer_script_endian("mgs5_tpp TPP_CONFIG_DATA1 (PS4 CUSA01140)",
                        "mgs5-tpp-decrypter/samples/TPP_CONFIG_DATA1.enc",
                        "mgs5-tpp-decrypter/samples/TPP_CONFIG_DATA1.dec",
                        MGS5_PS4_DEC, MGS5_PS4_ENC, 0, 0);
}

TEST(sample_mgs5_tpp_ps4_persona)
{
    known_answer_script_endian("mgs5_tpp PERSONAL_DATA0 (PS4 CUSA01140)",
                        "mgs5-tpp-decrypter/samples/PERSONAL_DATA0.enc",
                        "mgs5-tpp-decrypter/samples/PERSONAL_DATA0.dec",
                        MGS5_PS4_DEC, MGS5_PS4_ENC, 0, 0);
}

/*
 * Dragon Ball Z: Xenoverse 2 — PS4/CUSA05088. Two-stage: a fixed key unwraps
 * the 0x20..0x9F header, and the body key/IV are then read out of the
 * decrypted header at an offset computed from a flag bit at 0x25.
 *
 * The encrypt chain is the patch's full required set in file order -- the
 * custom checksum, the encrypt code (which recomputes it inline and re-wraps
 * the header), then the MD5 over everything from 0x20.
 */

#define DBZXV2_KEYIV                    \
    "set [offset]:read(0x25, 0x01)\n"   \
    "set [offset]:AND:0x04\n"           \
    "set [offset]:[offset]+[offset]+[offset]\n" \
    "set [offset]:[offset]+[offset]+[offset]+[offset]\n" \
    "set [offset]:[offset]+0x3C\n"      \
    "set [key_ctr]:read([offset], 0x20)\n" \
    "set [offset]:[offset]+0x20\n"      \
    "set [iv_ctr]:read([offset], 0x10)\n"

static const char DBZXV2_DEC[] =
    "set range:0x0020,0x009F\n"
    "decrypt AES_CTR(\"PR]-<Q9*WxHsV8rcW!JuH7k_ug:T5ApX\", \"_Y7]mD1ziyH#Ar=0\")\n"
    DBZXV2_KEYIV
    "set range:0x00A0,EOF+1\n"
    "decrypt AES_CTR([key_ctr], [iv_ctr])";

static const char DBZXV2_ENC[] =
    "write at 0x0034:0x0000000000000000\n"
    "set [checkdbz]:dbzxv2_checksum\n"
    "write at 0x0034:[checkdbz]\n"
    DBZXV2_KEYIV
    "set range:0x00A0,EOF+1\n"
    "encrypt AES_CTR([key_ctr], [iv_ctr])\n"
    "set [checkdbz]:dbzxv2_checksum\n"
    "write at 0x0034:[checkdbz]\n"
    "set range:0x0020,0x009F\n"
    "encrypt AES_CTR(\"PR]-<Q9*WxHsV8rcW!JuH7k_ug:T5ApX\", \"_Y7]mD1ziyH#Ar=0\")\n"
    "set pointer:EOF+1\n"
    "set range:0x0020,pointer\n"
    "set [hash]:MD5\n"
    "write at 0x0010:[hash]";

TEST(sample_dbz_xenoverse2)
{
    known_answer_script("aes_ctr SDATA000.DAT",
                        "dbz-xenoverse2-decrypter/samples/SDATA000.DAT.enc",
                        "dbz-xenoverse2-decrypter/samples/SDATA000.DAT.dec",
                        DBZXV2_DEC, DBZXV2_ENC, 0);
}

/*
 * L.A. Noire (PS4) — PS4/CUSA09084. The patch's key is an interactive
 * {LA_NOIRE_AES_CBC256_KEY_OPTION} group with two values, one per file kind;
 * these vectors substitute the chosen value the way apply_tag_opts() would,
 * so each covers the branch a user picking that option gets. (The tag
 * substitution itself is covered in test_parse.c.)
 */
#define LANOIRE_SAVE_KEY    "Wr9uFi4yi*?OESwiavv$ayIAp+u23PIe"
#define LANOIRE_PROFILE_KEY "_!pH4ThU-7N?u&eph4$eaC!aTHaQ5U7u"

#define LANOIRE_DEC(key)                                                    \
    "set range:0x00,EOF+1\n"                                                \
    "DECRYPT aes_cbc(\"" key "\", 0x00000000000000000000000000000000)"
#define LANOIRE_ENC(key)                                                    \
    "set range:0x00,EOF+1\n"                                                \
    "ENCRYPT aes_cbc(\"" key "\", 0x00000000000000000000000000000000)"

static const char LANOIRE_SAVE_DEC[] = LANOIRE_DEC(LANOIRE_SAVE_KEY);
static const char LANOIRE_SAVE_ENC[] = LANOIRE_ENC(LANOIRE_SAVE_KEY);
static const char LANOIRE_PROF_DEC[] = LANOIRE_DEC(LANOIRE_PROFILE_KEY);
static const char LANOIRE_PROF_ENC[] = LANOIRE_ENC(LANOIRE_PROFILE_KEY);

TEST(sample_la_noire_ps4_save)
{
    known_answer_script("aes_cbc lansavegame.ps4.sav",
                        "la-noire-decrypter/samples/lansavegame.ps4.sav.enc",
                        "la-noire-decrypter/samples/lansavegame.ps4.sav.dec",
                        LANOIRE_SAVE_DEC, LANOIRE_SAVE_ENC, 0);
}

TEST(sample_la_noire_ps4_profile)
{
    known_answer_script("aes_cbc lansavegame.ps4.sav.profile",
                        "la-noire-decrypter/samples/lansavegame.ps4.sav.profile.enc",
                        "la-noire-decrypter/samples/lansavegame.ps4.sav.profile.dec",
                        LANOIRE_PROF_DEC, LANOIRE_PROF_ENC, 0);
}

/* Second RE remaster sample — same key covers RE HD Remaster and RE0. */
TEST(sample_re_remaster_data0)
{
    known_answer_script("blowfish_ecb DATA0.DAT0",
                        "re-remaster-decrypter/samples/DATA0.DAT0.enc",
                        "re-remaster-decrypter/samples/DATA0.DAT0.dec",
                        RE_REMASTER_DEC, RE_REMASTER_ENC, 0);
}

/* Second RE4 Remake sample. */
TEST(sample_re4_remake_b)
{
    known_answer_script("blowfish_cbc RE4R b",
                        "ps4-re4r-decrypter/samples/b.enc",
                        "ps4-re4r-decrypter/samples/b.dec",
                        RE4R_DEC, RE4R_ENC, 0);
}

/* ---- Tales of Berseria — PS3/BLJS10330 and PS4/CUSA05105 ---- */

static const char BERSERIA_DEC[] =
    "import berseria\n"
    "berseria.berseria_crypt(berseria.DECRYPT, savedata)";
static const char BERSERIA_ENC[] =
    "import berseria\n"
    "berseria.berseria_crypt(berseria.ENCRYPT, savedata)";

static const char* const BERSERIA_DEC_C[] = { BERSERIA_DEC, NULL };
static const char* const BERSERIA_ENC_C[] = { BERSERIA_ENC, NULL };

TEST(sample_berseria_ps3)
{
    known_answer_python("python berseria SAVE (PS3)",
                        "berseria-decrypter/samples/SAVE.enc",
                        "berseria-decrypter/samples/SAVE.dec",
                        BERSERIA_DEC_C, BERSERIA_ENC_C, 1, 0);
}

TEST(sample_berseria_ps4)
{
    known_answer_python("python berseria SAVEDATA0.DAT (PS4)",
                        "berseria-decrypter/samples/SAVEDATA0.DAT.enc",
                        "berseria-decrypter/samples/SAVEDATA0.DAT.dec",
                        BERSERIA_DEC_C, BERSERIA_ENC_C, 1, 0);
}

/* ---- Dead or Alive 5 — PS3/BLES01623 ---- */

static const char DOA5_DEC[] =
    "import doa5\n"
    "doa5.doa5_crypt(doa5.DECRYPT, savedata)";
static const char DOA5_ENC[] =
    "import doa5\n"
    "doa5.doa5_crypt(doa5.ENCRYPT, savedata)";

static const char* const DOA5_DEC_C[] = { DOA5_DEC, NULL };
static const char* const DOA5_ENC_C[] = { DOA5_ENC, NULL };

TEST(sample_doa5_ps3)
{
    known_answer_python("python doa5 SAVE_SYS.DAT (PS3)",
                        "doa5-decrypter/samples/SAVE_SYS.DAT.PS3.enc",
                        "doa5-decrypter/samples/SAVE_SYS.DAT.PS3.dec",
                        DOA5_DEC_C, DOA5_ENC_C, 1, 0);
}

TEST(sample_doa5_vita)
{
    known_answer_python("python doa5 SAVE_SYS.DAT (Vita)",
                        "doa5-decrypter/samples/SAVE_SYS.DAT.vita.enc",
                        "doa5-decrypter/samples/SAVE_SYS.DAT.vita.dec",
                        DOA5_DEC_C, DOA5_ENC_C, 1, 0);
}

/*
 * Crisis Core: FF VII Reunion — PS4/CUSA31348. The patch's third required
 * code recomputes a checksum over the decrypted body, so the encrypt chain
 * here is checksum-then-encrypt, in file order.
 */
static const char FF7CC_DEC[] =
    "import ff7cc\n"
    "ff7cc.ff7cc_crypt(ff7cc.MODE_DECRYPT, savedata)";
static const char FF7CC_ENC[] =
    "import ff7cc\n"
    "ff7cc.ff7cc_crypt(ff7cc.MODE_ENCRYPT, savedata)";
/*
 * The checksum is computed over the ENCRYPTED body, which is why the patch
 * orders this code after Encrypt rather than before it: the C tool verifies
 * it on the ciphertext before it decrypts anything
 * (ff7cc-decrypter/main.c checks data+0x568 prior to decrypt_data()). Run it
 * on the plaintext instead and it writes a checksum the game will reject.
 */
static const char FF7CC_CSUM[] =
    "import ff7cc\n"
    "import ustruct as struct\n"
    "size_csum = struct.unpack_from(\"<I\", savedata, 0x55C)[0]\n"
    "size_csum += 0x560\n"
    "csum = ff7cc.ff7cc_checksum(savedata[0x568:size_csum])\n"
    "struct.pack_into(\"<I\", savedata, 0x564, csum)";

static const char* const FF7CC_DEC_C[] = { FF7CC_DEC, NULL };
static const char* const FF7CC_ENC_C[] = { FF7CC_ENC, FF7CC_CSUM, NULL };

TEST(sample_ff7cc)
{
    known_answer_python("python ff7cc ue4savegame.ps4.sav",
                        "ff7cc-decrypter/samples/ue4savegame.ps4.sav.enc",
                        "ff7cc-decrypter/samples/ue4savegame.ps4.sav.dec",
                        FF7CC_DEC_C, FF7CC_ENC_C, 1, 0);
}

/*
 * Round-trip-only Python vector: encrypt(decrypt(.enc)) == .enc.
 *
 * For a patch whose DECRYPTED layout legitimately differs from the reference
 * tool's, so there is no known answer to compare against -- see the Monster
 * Hunter World note below. It still proves the pair is a true inverse, which
 * is the property a front-end depends on.
 */
static void roundtrip_python(const char* label, const char* rel_enc,
                             const char* const* dec_chain, const char* const* enc_chain)
{
    const char* root = samples_root();
    const char* patches = getenv("PATCHES");
    uint8_t *enc = NULL, *mid = NULL, *back = NULL;
    size_t nenc = 0, nmid = 0, nback = 0;

    if (!root)
    {
        printf("        (skipped %s: set SAMPLES=/path/to/save-decrypters)\n", label);
        return;
    }
    if (!py_modules_available(patches))
    {
        printf("        (skipped %s: set PATCHES=/path/to/apollo-patches)\n", label);
        return;
    }
    snprintf(g_py_data_path, sizeof(g_py_data_path), "%s/", patches);

    enc = slurp_at(root, rel_enc, &nenc);
    if (!enc)
    {
        printf("        (skipped %s: sample not found)\n", label);
        return;
    }

    if (!py_apply(dec_chain, enc, nenc, &mid, &nmid))
        check_u64(__FILE__, __LINE__, label, 0, 1);
    else
    {
        if (!py_apply(enc_chain, mid, nmid, &back, &nback))
            check_u64(__FILE__, __LINE__, label, 0, 1);
        else
        {
            if (check_u64(__FILE__, __LINE__, label, nback, nenc))
                check_mem(__FILE__, __LINE__, label, back, enc, nenc);
            free(back);
        }
        free(mid);
    }

    apollo_free_var_list();
    unlink(py_tmp_path());
    free(enc);
}

/*
 * Monster Hunter World — PS4/CUSA07713.
 *
 * Round-trip only, because the savepatch and the C tool DISAGREE about where
 * 3128 bytes live in the decrypted file, and neither is this library's bug:
 *
 *   monsterhunter-world-decrypter (iceborne_crypt.h) calls
 *       rotateBuffer(save, 0x600488, 0x61D4C8 + 3128, -3128)
 *   which ROTATES that window left, so the block that was at 0x600488 ends up
 *   at 0x61D4C8 in its decrypted output.
 *
 *   python/mhworld.py instead builds
 *       tmp_save = save[:0x600488] + save[0x6010C0:]
 *   crypts that, and splices it back -- excluding the same 3128 bytes from the
 *   cipher stream but LEAVING them at 0x600488.
 *
 * Both are self-consistent (the patch's decrypt->encrypt reproduces the
 * original save byte for byte, which is what this vector asserts) and their
 * ciphertext agrees, but their plaintexts differ in ~2.4KB around that window.
 * A save editor written against one layout will misread the other, so the two
 * want reconciling upstream; until then, asserting the C tool's .dec here
 * would be asserting the wrong one of two conventions.
 */


static const char MHWORLD_DEC[] =
    "import mhworld\n"
    "mhworld.mhworld_crypt(mhworld.MODE_DECRYPT, savedata)";
static const char MHWORLD_ENC[] =
    "import mhworld\n"
    "mhworld.mhworld_crypt(mhworld.MODE_ENCRYPT, savedata)";

static const char* const MHWORLD_DEC_C[] = { MHWORLD_DEC, NULL };
static const char* const MHWORLD_ENC_C[] = { MHWORLD_ENC, NULL };

TEST(sample_mhworld)
{
    roundtrip_python("python mhworld memory.dat (round-trip)",
                     "monsterhunter-world-decrypter/samples/memory.dat.enc",
                     MHWORLD_DEC_C, MHWORLD_ENC_C);
}

/* ---- Nioh 2 — PS4/CUSA15526. One self-inverse call both ways. ---- */

static const char NIOH2_CRYPT[] =
    "import nioh2\n"
    "nioh2.nioh2_crypt(savedata)";

static const char* const NIOH2_CRYPT_C[] = { NIOH2_CRYPT, NULL };

TEST(sample_nioh2)
{
    known_answer_python("python nioh2 APP.BIN",
                        "nioh2-decrypter/samples/APP.BIN.enc",
                        "nioh2-decrypter/samples/APP.BIN.dec",
                        NIOH2_CRYPT_C, NIOH2_CRYPT_C, 1, 0);
}

TEST(sample_nioh2_sys)
{
    known_answer_python("python nioh2 SYS.APP.BIN",
                        "nioh2-decrypter/samples/SYS.APP.BIN.enc",
                        "nioh2-decrypter/samples/SYS.APP.BIN.dec",
                        NIOH2_CRYPT_C, NIOH2_CRYPT_C, 1, 0);
}

/*
 * L.A. Noire (PS3) — PS3/BLUS30554. The patch branches on an {ST} option;
 * these substitute each value, covering both the savedata and profile keys.
 */
#define LANOIRE_PS3(mode, st)                                       \
    "import lanoire\n"                                              \
    "type = " st "\n"                                               \
    "if (type == 1):\n"                                             \
    "    lanoire.lanoire_save_crypt(lanoire.MODE_" mode ", savedata)\n" \
    "else:\n"                                                       \
    "    lanoire.lanoire_profile_crypt(lanoire.MODE_" mode ", savedata)"

static const char LANOIRE_PS3_SAVE_DEC[] = LANOIRE_PS3("DECRYPT", "1");
static const char LANOIRE_PS3_SAVE_ENC[] = LANOIRE_PS3("ENCRYPT", "1");
static const char LANOIRE_PS3_PROF_DEC[] = LANOIRE_PS3("DECRYPT", "2");
static const char LANOIRE_PS3_PROF_ENC[] = LANOIRE_PS3("ENCRYPT", "2");

static const char* const LANOIRE_PS3_SAVE_DEC_C[] = { LANOIRE_PS3_SAVE_DEC, NULL };
static const char* const LANOIRE_PS3_SAVE_ENC_C[] = { LANOIRE_PS3_SAVE_ENC, NULL };
static const char* const LANOIRE_PS3_PROF_DEC_C[] = { LANOIRE_PS3_PROF_DEC, NULL };
static const char* const LANOIRE_PS3_PROF_ENC_C[] = { LANOIRE_PS3_PROF_ENC, NULL };

TEST(sample_la_noire_ps3_save)
{
    known_answer_python("python lanoire SAVEDATA (PS3)",
                        "la-noire-decrypter/samples/SAVEDATA.enc",
                        "la-noire-decrypter/samples/SAVEDATA.dec",
                        LANOIRE_PS3_SAVE_DEC_C, LANOIRE_PS3_SAVE_ENC_C, 1, 0);
}

TEST(sample_la_noire_ps3_profile)
{
    known_answer_python("python lanoire SAVEDATA.profile (PS3)",
                        "la-noire-decrypter/samples/SAVEDATA.profile.enc",
                        "la-noire-decrypter/samples/SAVEDATA.profile.dec",
                        LANOIRE_PS3_PROF_DEC_C, LANOIRE_PS3_PROF_ENC_C, 1, 0);
}

/*
 * Goat Simulator 3 — PS4/CUSA46680. Decrypts AND unpacks in one code, so the
 * plaintext is an order of magnitude larger than the file (5168 -> 52417).
 */
static const char GS3_DEC[] =
    "import goat_sim3\n"
    "goat_sim3.gs3_crypt(goat_sim3.DECRYPT, savedata)";
static const char GS3_ENC[] =
    "import goat_sim3\n"
    "goat_sim3.gs3_crypt(goat_sim3.ENCRYPT, savedata)";

static const char* const GS3_DEC_C[] = { GS3_DEC, NULL };
static const char* const GS3_ENC_C[] = { GS3_ENC, NULL };

TEST(sample_goat_sim3)
{
    known_answer_python("python goat_sim3 ue4savegame.ps4.sav",
                        "gs3-decrypter/samples/ue4savegame.ps4.sav.enc",
                        "gs3-decrypter/samples/ue4savegame.ps4.sav.dec",
                        GS3_DEC_C, GS3_ENC_C, 0, 0x0E);
}

/*
 * Final Fantasy Pixel Remaster — PS4/CUSA33817. Four required codes: decrypt
 * then decompress on the way in, compress then encrypt on the way out. They
 * are chained into one body here, which is the same thing the engine does
 * when a front-end applies both in order -- each runs in the same globals and
 * hands `savedata` on.
 */
#define FFPIXEL_RIJNDAEL                                                     \
    "import uhashlib\n"                                                      \
    "import rijndael\n"                                                      \
    "password = b'TKX73OHHK1qMonoICbpVT0hIDGe7SkW0'\n"                       \
    "salt     = b'71Ba2p0ULBGaE6oJ7TjCqwsls1jBKmRL'\n"                       \
    "derived_key = uhashlib.pbkdf2_sha1(password, salt, 10, 64)\n"           \
    "key = derived_key[:32]\n"                                               \
    "iv = derived_key[32:]\n"                                                \
    "ctx = rijndael.Rijndael(key, block_size = 32)\n"

static const char FFPIXEL_DECRYPT[] =
    FFPIXEL_RIJNDAEL
    "savedata = ctx.decrypt_cbc(savedata, iv)";
static const char FFPIXEL_DECOMPRESS[] =
    "import uzlib\n"
    "savedata = uzlib.decompress(savedata, -15)";
static const char FFPIXEL_COMPRESS[] =
    "import uzlib\n"
    "compressed = uzlib.compress(savedata, -15, 1)\n"
    "enc_len = (len(compressed) + 31) & ~31\n"
    "savedata = compressed + '\\x00'*(enc_len - len(compressed))";
static const char FFPIXEL_ENCRYPT[] =
    FFPIXEL_RIJNDAEL
    "savedata = ctx.encrypt_cbc(savedata, iv)";

static const char* const FFPIXEL_DEC_C[] = { FFPIXEL_DECRYPT, FFPIXEL_DECOMPRESS, NULL };
static const char* const FFPIXEL_ENC_C[] = { FFPIXEL_COMPRESS, FFPIXEL_ENCRYPT, NULL };

TEST(sample_ff_pixel_remaster)
{
    known_answer_python("python ff-pixel slot1.sav",
                        "ff-pixel-decrypter/samples/slot1.sav.enc",
                        "ff-pixel-decrypter/samples/slot1.sav.dec",
                        FFPIXEL_DEC_C, FFPIXEL_ENC_C, 0, 0);
}

/*
 * Regression: a bignum packed into a fixed-width field must zero-fill the
 * bytes it does not reach.
 *
 * micropy_mpz_as_bytes() emitted only as many bytes as the mpz had digits and
 * returned, leaving the rest of the caller's buffer untouched -- so
 * `struct.pack_into('>I', buf, 0, x)` wrote just the low two bytes of a
 * four-byte field whenever x needed fewer, and the high two kept whatever was
 * there before.
 *
 * It only bites where the value is an mpz at all. MP_SMALL_INT is 31 bits on a
 * 32-bit target and 63 on a 64-bit one, so `0xFFFFFFFF ^ 0xFFFFFFFB` is a
 * bignum on wasm32, PS3, PSP and PS Vita and an ordinary small int on x86_64 --
 * which is why this reproduced under wasm while the native build stayed green,
 * and why it is not a wasm bug: every 32-bit console build had it too.
 *
 * Found via doa5, whose crypt_block() does exactly this over a whole save:
 * `struct.pack_into(byte_order + "I", block, i, word ^ seed)`. Words whose XOR
 * happened to need all four bytes came out right and the rest kept half their
 * ciphertext, corrupting ~80% of the file.
 *
 * Needs no fixtures and no apollo-patches checkout -- ustruct is built in -- so
 * unlike the rest of this file it runs in a plain `make check`.
 */
TEST(python_mpz_pack_into_zero_fills)
{
    static const char src[] =
        "import ustruct as struct\n"
        "big = 0xFFFFFFFF\n"
        "struct.pack_into('>I', savedata,  0, 4)\n"
        "struct.pack_into('>I', savedata,  4, big ^ 0xFFFFFFFB)\n"
        "struct.pack_into('<I', savedata,  8, big ^ 0xFFFFFFFB)\n"
        "struct.pack_into('>I', savedata, 12, big ^ 0x0000FFFB)\n";
    static const char* const chain[] = { src, NULL };

    /* Pre-dirtied, so a byte the engine fails to write shows up as 0xFF. */
    static const uint8_t expect[16] = {
        0x00,0x00,0x00,0x04,   /* small int, always took a different path   */
        0x00,0x00,0x00,0x04,   /* bignum, big-endian    -- was FF FF 00 04  */
        0x04,0x00,0x00,0x00,   /* bignum, little-endian -- was 04 00 FF FF  */
        0xFF,0xFF,0x00,0x04,   /* bignum needing all four bytes: always ok  */
    };
    uint8_t in[16];
    uint8_t* out = NULL;
    size_t n = 0;

    memset(in, 0xFF, sizeof(in));
    g_py_data_path[0] = '\0';   /* nothing here imports a helper module */

    if (!py_apply(chain, in, sizeof(in), &out, &n))
    {
        CHECK_U64("mpz pack_into: script ran", 0, 1);
        return;
    }

    if (check_u64(__FILE__, __LINE__, "mpz pack_into: size", n, sizeof(expect)))
        check_mem(__FILE__, __LINE__, "mpz pack_into zero-fills", out, expect, sizeof(expect));

    free(out);
    apollo_free_var_list();
    unlink(py_tmp_path());
}
