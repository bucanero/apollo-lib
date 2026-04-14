/*
    Copyright 2004-2015 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl-2.0.txt
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <zlib.h>

#include "apollo.h"
#include <dbglogger.h>
#define LOG dbglogger_log

#define VER         "0.3.1"
#define min(a,b)   (((a)<(b))?(a):(b))


static uint8_t* zipit(uint8_t *in_data, uint32_t in_size, uint32_t *out_size, int wbits, int flags, int store);
static uint32_t zlib_compress(uint8_t *in, int insz, uint8_t *out, int outsz, int wbits, int flags, int store);

/*
            "Usage: %s [options] <input> <output>\n"
            "\n"
            "Options:\n"
            "-o OFF   offset of the output file where storing the raw zipped data\n"
            "-c       recreate from scratch the output file if already exists\n"
            "         keep this option in mind if you want to re-create a file because the\n"
            "         main usage of this tool is reinjecting files inside archives!\n"
            "-w BITS  windowBits value (usually -15 to -8 and 8 to 15), default is %d\n"
            "         negative for raw deflate and positive for zlib rfc1950 (78...)\n"
            "         if 0 then it will perform the LZMA compression\n"
            "-m MODE  mode:\n"
            "         zlib/deflate            LZMA\n"
            "           %d default strategy     %d normal (default)\n"
            "           %d filtered             %d 86_header\n"
            "           %d huffman only         %d 86_decoder\n"
            "           %d rle                  %d 86_dechead\n"
            "           %d fixed                %d efs\n"
            "-0       store only, no compression\n"
            "-f       use AdvanceComp compression which is faster\n"
            "\n"
            "By default this tool \"injects\" the new compressed data in the output file if\n"
            "already exists, useful for modifying archives of unknown formats replacing\n"
            "only the data that has been modified without touching the rest.\n"
            "The tool uses zopfli or AdvanceCOMP for zlib/deflate compression but switches\n"
            "to zlib if the -m option is used.\n"
            "\n", argv[0],
            wbits,
            Z_DEFAULT_STRATEGY,     LZMA_FLAGS_NONE,
            Z_FILTERED,             LZMA_FLAGS_86_HEADER,
            Z_HUFFMAN_ONLY,         LZMA_FLAGS_86_DECODER,
            Z_RLE,                  LZMA_FLAGS_86_DECODER | LZMA_FLAGS_86_HEADER,
            Z_FIXED,                LZMA_FLAGS_EFS
        );
*/

static int packzip_compress(offzip_t* data_in, uint8_t **obuf, size_t *olen) {
    void *ptr;
    uint8_t* in_data = data_in->data;

    LOG("- offset        0x%08x", data_in->offset);
    LOG("- windowbits    %d", data_in->wbits);
    LOG("- zip size      0x%08x / %u", data_in->ziplen, data_in->ziplen);

    if (*olen > data_in->offset)
    {
        memset(*obuf + data_in->offset, 0, min(*olen - data_in->offset, data_in->ziplen));
    }

    // if ref_outlen is set, then use references to read the original data and outlen from the variable list
    if (data_in->ref_outlen) {
        in_data = *(uint8_t**)data_in->data;
        data_in->outlen = *data_in->ref_outlen;
    }

    uint8_t* zdata = zipit(in_data, data_in->outlen, &data_in->ziplen, data_in->wbits, Z_DEFAULT_STRATEGY, 0);
    if(!zdata) {
        LOG("- the compression failed");
        return 0;
    }

    LOG("- output size   0x%08x / %u", data_in->ziplen, data_in->ziplen);

    if (*olen < (data_in->ziplen + data_in->offset))
    {
        ptr = realloc(*obuf, data_in->ziplen + data_in->offset);
        if (!ptr) {
            LOG("Error: memory allocation failed");
            free(zdata);
            return 0;
        }
        *obuf = ptr;
        *olen = data_in->ziplen + data_in->offset;
        LOG("- resizing %u bytes", *olen);
    }

    memcpy(*obuf + data_in->offset, zdata, data_in->ziplen);
    free(zdata);

    return(1);
}

int packzip_util(offzip_t *offzip, uint32_t offset, uint8_t** output, size_t* outsize) {
    int i = 0;

    LOG("PackZip " VER " by Luigi Auriemma / aluigi@autistici.org / aluigi.org");

    for (i = 0; offzip[i].data; i++)
    {
        if (!offset || offset == offzip[i].offset)
        {
            LOG("Packing #%d at 0x%08X (%d)...", i, offzip[i].offset, offzip[i].offset);

            if (!packzip_compress(&offzip[i], output, outsize))
            {
                LOG("Error: unable to compress file (#%d)", i);
                return 0;
            }
        }
    }

    return (i);
}

static uint8_t* zipit(uint8_t *in_data, uint32_t in_size, uint32_t *out_size, int wbits, int flags, int store) {
    int     ret = 0;
    uint8_t *out_data;

    if(!in_data) return(NULL);
    if(!wbits) {
        LOG(" LZMA (Not supported)");
        return(NULL);
    }

    *out_size = compressBound(in_size);
    out_data = (uint8_t *)malloc(*out_size);

    if(!out_data) return(NULL);

    LOG("- input size    0x%08x / %u", in_size, in_size);

    if(wbits > 0) {
        LOG("- compression   ZLIB");
    } else {
        LOG("- compression   DEFLATE");
    }

    ret = zlib_compress(in_data, in_size, out_data, *out_size, wbits, flags, store);
    if(ret <= 0) {
        free(out_data);
        out_data = NULL;
        *out_size = 0;
    } else {
        *out_size = ret;
    }

    return(out_data);
}

static uint32_t zlib_compress(uint8_t *in, int insz, uint8_t *out, int outsz, int wbits, int flags, int store) {
    z_stream    z;
    uint32_t    ret;
    int     zerr;

    z.zalloc = Z_NULL;
    z.zfree  = Z_NULL;
    z.opaque = Z_NULL;
    if(deflateInit2(&z, store ? Z_NO_COMPRESSION : Z_BEST_COMPRESSION, Z_DEFLATED, wbits, 9, flags)) {
        LOG("Error: zlib initialization error");
        return(-1);
    }

    z.next_in   = in;
    z.avail_in  = insz;
    z.next_out  = out;
    z.avail_out = outsz;
    zerr = deflate(&z, Z_FINISH);
    if(zerr != Z_STREAM_END) {
        LOG("Error: zlib error, %s", z.msg ? z.msg : "");
        deflateEnd(&z);
        return(-1);
    }

    ret = z.total_out;
    deflateEnd(&z);
    return(ret);
}
