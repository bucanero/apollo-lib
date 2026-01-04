/*
    Copyright 2004-2019 Luigi Auriemma

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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <zlib.h>
#include <dirent.h>

#include "apollo.h"
#include <dbglogger.h>
#define LOG dbglogger_log


#define VER             "0.3.5"
#define INSZ            0x800   // the amount of bytes we want to decompress each time
#define OUTSZ           0x10000 // the buffer used for decompressing the data
#define FBUFFSZ         0x40000 // this buffer is used for reading, faster
#define SHOWX           0x7fff  // AND to show the current scanned offset each SHOWX offsets
#define MAX_RESULTS     0x40

#define Z_INIT_ERROR    -1000
#define Z_END_ERROR     -1001
#define Z_RESET_ERROR   -1002

#define g_minzip        32


static int buffread(const uint8_t *fd, uint8_t *buff, int size);
static void buffseek(const uint8_t *fd, int off, int mode);
static void buffinc(int increase);
static int unzip_all(const uint8_t *fd, offzip_t* out_list);
static int unzip(const uint8_t *fd, uint32_t *inlen, uint32_t *outlen, uint8_t **dump);
static int zlib_err(int err);


static z_stream    z;
static uint32_t g_offset = 0,
        g_filebuffoff   = 0,
        g_filebuffsz    = 0;
static int g_zipwbits   = 0;
static int g_count      = 0;
static size_t g_size    = 0;
static uint8_t *g_in    = NULL,
        *g_out          = NULL,
        *g_filebuff     = NULL;


int offzip_init(size_t dsz, int wbits) {
    g_size      = dsz;
    g_zipwbits  = wbits;
    g_filebuffoff = 0;
    g_filebuffsz  = 0;
    g_offset      = 0;

    g_in        = malloc(INSZ);
    g_out       = malloc(OUTSZ);
    g_filebuff  = malloc(FBUFFSZ);
    if(!g_in || !g_out || !g_filebuff) {
        LOG("Error: unable to create buffers");
        return (Z_INIT_ERROR);
    }

    memset(&z, 0, sizeof(z));
    if(inflateInit2(&z, g_zipwbits) != Z_OK) {
        free(g_in);
        free(g_out);
        free(g_filebuff);
        g_in        = NULL;
        g_out       = NULL;
        g_filebuff  = NULL;
        return (Z_INIT_ERROR);
    }

    return Z_OK;
}

void offzip_free(void) {
    inflateEnd(&z);
    free(g_in);
    free(g_out);
    free(g_filebuff);

    g_in        = NULL;
    g_out       = NULL;
    g_filebuff  = NULL;
}

offzip_t* offzip_util(const uint8_t* data, size_t dlen, int offset, int wbits, int count) {
    offzip_t *ofz = NULL;
    int     files;

    LOG("Offzip "VER" by Luigi Auriemma / aluigi@autistici.org / aluigi.org");

/*
            "Usage: %s [options] <input> <output/dir> <offset>\n"
            "\n"
            "Options:\n"
            "-s       search for possible zip/gzip data in the input file, the scan starts\n"
            "         from the specified offset and finishs when something is found\n"
            "         the output field is ignored so you can use any name you want\n"
            "-S       as above but continues the scan (just like -a but without extraction)\n"
            "-a       unzip all the possible zip data found in the file. the output\n"
            "         directory where are unzipped the files is identified by <output>\n"
            "         all the output filenames contain the offset where they have been found\n"
            "-A       as above but without unzipping data, the output files will contain the\n"
            "         same original zipped data, just like a simple data dumper\n"
            "-1       related to -a/-A, generates one unique output file instead of many\n"
            "-m SIZE  lets you to decide the length of the zip block to check if it is a\n"
            "         valid zip data. default is %d. use a higher value to reduce the number\n"
            "         of false positive or a smaller one (eg 16) to see small zip data too\n"
            "-z NUM   this option is needed to specify a windowBits value. If you don't find\n"
            "         zip data in a file (like a classical zip file) try to set it to -15\n"
            "         valid values go from -8 to -15 and from 8 to 15. Default is 15\n"
            "-q       quiet, all the verbose error messages will be suppressed (-Q for more)\n"
            "-r       don't remove the invalid uncompressed files generated with -a and -A\n"
            "-x       visualize hexadecimal numbers\n"
            "-L FILE  dump the list of \"0xoffset zsize size\" in the specified FILE\n"
            "\n"
            "Note: offset is a decimal number or a hex number if you add a 0x before it\n"
            "      examples: 1234 or 0x4d2\n"
            "\n", argv[0], g_minzip);
*/

    if(offzip_init(dlen, wbits) != Z_OK)
    {
        LOG("Error: unable to create buffers");
        return 0;
    }

    g_count = count;
    g_offset = offset;

    ofz = calloc(count ? (count+1) : (MAX_RESULTS+1), sizeof(offzip_t));
    if(!ofz) {
        LOG("Error: unable to create offzip list");
        offzip_free();
        return 0;
    }

    LOG("- zip data to check:  %d bytes", g_minzip);
    LOG("- zip windowBits:     %d", g_zipwbits);
    LOG("- seek offset:        0x%08x  (%u)", g_offset, g_offset);
    LOG("- scan count :        %d", g_count);
    buffseek(data, g_offset, SEEK_SET);

    LOG("+------------+-------------+-------------------------+");
    LOG("| hex_offset | blocks_dots | zip_size --> unzip_size |");
    LOG("+------------+-------------+-------------------------+");

    files = unzip_all(data, ofz); //ZIPDOFILE
    if(files) {
        LOG("- %u valid zip blocks found", files);
    } else {
        LOG("- no valid full zip data found");
        free(ofz);
        ofz = NULL;
    }

    offzip_free();

    return(ofz);
}

static int memread(uint8_t *buff, int size, const uint8_t *fd) {
    int ret = size;

    if((size + g_offset) > g_size) {
        ret = g_size - g_offset;
    }

    memcpy(buff, fd + g_offset, ret);
    return ret;
}

static int buffread(const uint8_t *fd, uint8_t *buff, int size) {
    int     len,
            rest,
            ret;

    rest = g_filebuffsz - g_filebuffoff;

    ret = size;
    if(rest < size) {
        ret = size - rest;
        memmove(g_filebuff, g_filebuff + g_filebuffoff, rest);
        len = memread(g_filebuff + rest, FBUFFSZ - rest, fd);
        g_filebuffoff = 0;
        g_filebuffsz  = rest + len;
        if(len < ret) {
            ret = rest + len;
        } else {
            ret = size;
        }
    }

    memcpy(buff, g_filebuff + g_filebuffoff, ret);
    return ret;
}

static void buffseek(const uint8_t *fd, int off, int mode) {
    if(mode != SEEK_SET || off > g_size)
    {
        LOG("Error: buffseek");
        return;
    }
    g_filebuffoff = 0;
    g_filebuffsz  = 0;
    g_offset      = off;
}

static void buffinc(int increase) {
    g_filebuffoff += increase;
    g_offset      += increase;
}

int offzip_search(const uint8_t *fd) {
    int     len,
            zerr,
            ret;

    for(ret = - 1; (len = buffread(fd, g_in, g_minzip)) >= g_minzip; buffinc(1)) {
        z.next_in   = g_in;
        z.avail_in  = len;
        z.next_out  = g_out;
        z.avail_out = OUTSZ;

        inflateReset(&z);
        zerr = inflate(&z, Z_SYNC_FLUSH);

        if(zerr == Z_OK) {  // do not use Z_STREAM_END here! gives only troubles!!!
            LOG("Zip found at 0x%08x offset", g_offset);

            ret = 0;
            break;
        }

        if(!(g_offset & SHOWX))
            LOG("Scanned 0x%08x offset", g_offset);
    }
    return ret;
}

static int unzip_all(const uint8_t *fd, offzip_t* out_list) {
    uint8_t  *fdo = NULL;
    uint32_t inlen,
            outlen;
    int     extracted = 0;

    while(!offzip_search(fd) && ((!g_count && extracted < MAX_RESULTS) || (g_count && extracted < g_count))) {
        LOG("Unzip (0x%08x) to %08" PRIx32 ".dat", g_offset, g_offset);

        if(unzip(fd, &inlen, &outlen, &fdo) != Z_OK) {
            // error during unzip
            free(fdo);
            fdo = NULL;
            continue;
        }

        out_list[extracted].data    = fdo;
        out_list[extracted].offset  = (g_offset - inlen);
        out_list[extracted].wbits   = g_zipwbits;
        out_list[extracted].ziplen  = inlen;
        out_list[extracted].outlen  = outlen;

        extracted++;
        LOG("#%d: %u --> %u", extracted, inlen, outlen);
    }

    return extracted;
}

static int unzip(const uint8_t *fd, uint32_t *inlen, uint32_t *outlen, uint8_t **dump) {
    void *ptr;
    uint32_t oldsz = 0,
            oldoff,
            len;
    int     ret     = -1,
            zerr    = Z_OK;

    *dump = NULL;
    oldoff = g_offset;
    inflateReset(&z);

    for(; (len = buffread(fd, g_in, INSZ)); buffinc(len)) {
        //if(g_quiet >= 0) fputc('.', stderr);

        z.next_in   = g_in;
        z.avail_in  = len;
        do {
            z.next_out  = g_out;
            z.avail_out = OUTSZ;
            zerr = inflate(&z, Z_SYNC_FLUSH);

            ptr = realloc(*dump, z.total_out);
            if(!ptr) {
                LOG("Error: unable to realloc memory for unzip data");
                return -1;
            }
            *dump = ptr;
            memcpy((*dump) + oldsz, g_out, z.total_out - oldsz);
            oldsz = z.total_out;

            if(zerr != Z_OK) {      // inflate() return value MUST be handled now
                if(zerr == Z_STREAM_END) {
                    ret = 0;
                } else {
                    zlib_err(zerr);
                }
                break;
            }
            ret = 0;    // it's better to return 0 even if the z stream is incomplete... or not?
        } while(z.avail_in);

        if(zerr != Z_OK) break;     // Z_STREAM_END included, for avoiding "goto"
    }

    *inlen  = z.total_in;
    *outlen = z.total_out;
    if(!ret) {
        oldoff += z.total_in;
    } else {
        oldoff++;
    }
    buffseek(fd, oldoff, SEEK_SET);
    return ret;
}

static int zlib_err(int zerr) {
    switch(zerr) {
        case Z_DATA_ERROR: {
            LOG("- zlib Z_DATA_ERROR, the data in the file is not in zip format"
                "  or uses a different windowBits value (-z). Try to use -z %d",
                -g_zipwbits);
            break;
        }
        case Z_NEED_DICT: {
            LOG("- zlib Z_NEED_DICT, you need to set a dictionary (option not available)");
            break;
        }
        case Z_MEM_ERROR: {
            LOG("- zlib Z_MEM_ERROR, insufficient memory");
            break;
        }
        case Z_BUF_ERROR: {
            LOG("- zlib Z_BUF_ERROR, the output buffer for zlib decompression is not enough");
            break;
        }
        case Z_INIT_ERROR: {
            LOG("Error: zlib initialization error (inflateInit2)");
            break;
        }
        case Z_END_ERROR: {
            LOG("Error: zlib free error (inflateEnd)");
            break;
        }
        case Z_RESET_ERROR: {
            LOG("Error: zlib reset error (inflateReset)");
            break;
        }
        default: {
            LOG("Error: zlib unknown error %d", zerr);
            break;
        }
    }
    return 0;
}

int offzip_verify(const uint8_t *fd, uint32_t *offset, uint32_t *inlen, uint32_t *outlen) {
    uint32_t oldoff,
            len;
    int     ret     = -1,
            zerr    = Z_OK;

    oldoff = g_offset;
    *offset = g_offset;
    inflateReset(&z);

    for(; (len = buffread(fd, g_in, INSZ)); buffinc(len)) {
        z.next_in   = g_in;
        z.avail_in  = len;
        do {
            z.next_out  = g_out;
            z.avail_out = OUTSZ;
            zerr = inflate(&z, Z_SYNC_FLUSH);

            if(zerr != Z_OK) {      // inflate() return value MUST be handled now
                if(zerr == Z_STREAM_END) {
                    ret = 0;
                } else {
                    zlib_err(zerr);
                }
                break;
            }
            ret = 0;    // it's better to return 0 even if the z stream is incomplete... or not?
        } while(z.avail_in);

        if(zerr != Z_OK) break;     // Z_STREAM_END included, for avoiding "goto"
    }

    *inlen  = z.total_in;
    *outlen = z.total_out;
    if(!ret) {
        oldoff += z.total_in;
    } else {
        oldoff++;
    }
    buffseek(fd, oldoff, SEEK_SET);
    return ret;
}
