/*
 * offZip session vectors.
 *
 * offzip.c held ten file-scope globals until the scan state moved into the
 * handle that offzip_init() already returned (it used to hand back a pointer
 * to the single global MEMFILE). Nothing in the suite covered it at the time,
 * which is how that went unnoticed — these vectors close the gap.
 *
 * Build-flag invariant: offzip reads zlib streams through zlib itself and
 * reports offsets/lengths as host integers, so the LE and BE builds must agree
 * on every value here.
 */
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "test_common.h"

#define OZ_PAD      0x40        /* filler between planted streams          */
#define OZ_BUFSZ    0x8000
#define OZ_MINZIP   32          /* offzip skips candidate blocks below this */

/* Plants `n` deflate streams in a zeroed buffer, each preceded by OZ_PAD bytes
 * of filler. Returns the buffer; fills offs[]/zlens[]/rawlens[] with what was
 * planted so the expectations are derived, not hard-coded to a zlib version. */
static uint8_t* plant(int n, uint32_t* offs, uint32_t* zlens, uint32_t* rawlens)
{
    uint8_t* buf = calloc(1, OZ_BUFSZ);
    size_t   pos = OZ_PAD;

    for (int i = 0; i < n; i++)
    {
        uint8_t  raw[1024];
        uint8_t  comp[2048];
        uLongf   clen = sizeof(comp);

        /* deterministic, compressible payload — distinct per block */
        for (size_t k = 0; k < sizeof(raw); k++)
            raw[k] = (uint8_t)('A' + ((k / 8 + i) % 26));

        if (compress(comp, &clen, raw, sizeof(raw)) != Z_OK)
        {
            free(buf);
            return NULL;
        }

        memcpy(buf + pos, comp, clen);
        offs[i]    = (uint32_t) pos;
        zlens[i]   = (uint32_t) clen;
        rawlens[i] = (uint32_t) sizeof(raw);
        pos += clen + OZ_PAD;
    }

    return buf;
}

/* A session walks every planted stream, in order, reporting exact offsets. */
TEST(offzip_session_finds_planted_streams)
{
    uint32_t offs[3], zlens[3], rawlens[3];
    uint8_t* buf = plant(3, offs, zlens, rawlens);
    void*    fd;
    int      found = 0;

    if (!CHECK_U64("plant() built a buffer", buf != NULL, 1))
        return;

    fd = offzip_init(buf, OZ_BUFSZ, OFFZIP_WBITS_ZLIB);
    if (!CHECK_U64("offzip_init returns a handle", fd != NULL, 1))
    {
        free(buf);
        return;
    }

    while (offzip_search(fd) == 0 && found < 3)
    {
        uint32_t off = 0, in = 0, out = 0;

        if (offzip_verify(fd, &off, &in, &out) != 0)
            continue;

        CHECK_U64("stream offset", off, offs[found]);
        CHECK_U64("compressed length", in, zlens[found]);
        CHECK_U64("uncompressed length", out, rawlens[found]);
        found++;
    }

    CHECK_U64("all three streams found", found, 3);

    offzip_free(fd);
    free(buf);
}

/* offzip_free(NULL) must be a no-op, so error paths can free unconditionally. */
TEST(offzip_free_null_is_safe)
{
    offzip_free(NULL);
    CHECK_U64("offzip_free(NULL) returned", 1, 1);
}

/* The one-shot helper manages its own handle and reports the same geometry,
 * and the data it hands back must actually inflate to the planted bytes. */
TEST(offzip_util_matches_session)
{
    uint32_t offs[2], zlens[2], rawlens[2];
    uint8_t* buf = plant(2, offs, zlens, rawlens);
    offzip_t* list;
    int i;

    if (!CHECK_U64("plant() built a buffer", buf != NULL, 1))
        return;

    list = offzip_util(buf, OZ_BUFSZ, 0, OFFZIP_WBITS_ZLIB, 0);
    if (!CHECK_U64("offzip_util returns a list", list != NULL, 1))
    {
        free(buf);
        return;
    }

    for (i = 0; i < 2 && list[i].outlen; i++)
    {
        uint8_t expect[1024];

        CHECK_U64("util offset", list[i].offset, offs[i]);
        CHECK_U64("util compressed length", list[i].ziplen, zlens[i]);
        CHECK_U64("util uncompressed length", list[i].outlen, rawlens[i]);
        CHECK_U64("util wbits", list[i].wbits, OFFZIP_WBITS_ZLIB);

        for (size_t k = 0; k < sizeof(expect); k++)
            expect[k] = (uint8_t)('A' + ((k / 8 + i) % 26));

        CHECK_MEM("util inflated payload", list[i].data, expect, sizeof(expect));
    }

    CHECK_U64("util found both streams", i, 2);

    for (i = 0; list[i].outlen; i++)
        free(list[i].data);
    free(list);
    free(buf);
}

/*
 * The point of the handle: two sessions over the same input advance
 * independently. Against the old globals this could not hold — both handles
 * were the same pointer, so the scans shared one cursor and one zlib stream.
 */
TEST(offzip_sessions_are_independent)
{
    uint32_t offs[3], zlens[3], rawlens[3];
    uint8_t* buf = plant(3, offs, zlens, rawlens);
    void *a, *b;

    if (!CHECK_U64("plant() built a buffer", buf != NULL, 1))
        return;

    a = offzip_init(buf, OZ_BUFSZ, OFFZIP_WBITS_ZLIB);
    b = offzip_init(buf, OZ_BUFSZ, OFFZIP_WBITS_ZLIB);
    if (!CHECK_U64("both handles created", (a != NULL && b != NULL), 1))
    {
        offzip_free(a);
        offzip_free(b);
        free(buf);
        return;
    }

    /* Run A one stream ahead, then interleave: B must still see stream 0. */
    {
        uint32_t oa = 0, ia = 0, ua = 0;
        CHECK_U64("A search 0", offzip_search(a), 0);
        offzip_verify(a, &oa, &ia, &ua);
        CHECK_U64("A sees stream 0", oa, offs[0]);
    }
    {
        uint32_t ob = 0, ib = 0, ub = 0;
        CHECK_U64("B search 0", offzip_search(b), 0);
        offzip_verify(b, &ob, &ib, &ub);
        CHECK_U64("B still sees stream 0, not A's position", ob, offs[0]);
    }
    {
        uint32_t oa = 0, ia = 0, ua = 0;
        CHECK_U64("A search 1", offzip_search(a), 0);
        offzip_verify(a, &oa, &ia, &ua);
        CHECK_U64("A advanced to stream 1", oa, offs[1]);
    }
    {
        uint32_t ob = 0, ib = 0, ub = 0;
        CHECK_U64("B search 1", offzip_search(b), 0);
        offzip_verify(b, &ob, &ib, &ub);
        CHECK_U64("B advanced on its own cursor", ob, offs[1]);
    }

    offzip_free(a);
    offzip_free(b);
    free(buf);
}

/* A stream shorter than the g_minzip candidate window is skipped by design. */
TEST(offzip_ignores_subminimum_blocks)
{
    uint8_t* buf = calloc(1, OZ_BUFSZ);
    uint8_t  tiny[4] = { 'a', 'b', 'c', 'd' };
    uint8_t  comp[64];
    uLongf   clen = sizeof(comp);
    void*    fd;

    CHECK_U64("tiny payload compressed", compress(comp, &clen, tiny, sizeof(tiny)), Z_OK);
    CHECK_U64("tiny stream is under the scan window", clen < OZ_MINZIP, 1);

    memcpy(buf + OZ_PAD, comp, clen);

    fd = offzip_init(buf, OZ_BUFSZ, OFFZIP_WBITS_ZLIB);
    if (!CHECK_U64("offzip_init returns a handle", fd != NULL, 1))
    {
        free(buf);
        return;
    }

    CHECK_U64("no stream reported", offzip_search(fd), -1);

    offzip_free(fd);
    free(buf);
}
