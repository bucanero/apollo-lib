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
#define FBUFFSZ         0x10000 // this buffer is used for reading, faster
#define SHOWX           0x7fff  // AND to show the current scanned offset each SHOWX offsets
#define MAX_RESULTS     0x40

#define Z_INIT_ERROR    -1000
#define Z_END_ERROR     -1001
#define Z_RESET_ERROR   -1002

#define g_minzip        32

// Custom structure to hold memory buffer information
typedef struct {
    unsigned char *buffer;  // pointer to the memory buffer
    size_t size;            // total size of the buffer
    size_t position;        // current read position
} MEMFILE;

/*
 * All scan state lives in this struct, one per offzip session, so two scans can
 * run side by side without touching each other. offzip_init() returns it as the
 * opaque handle the API already had -- it used to hand back a pointer to the
 * single global instead.
 */
typedef struct {
    MEMFILE  memfd;         // memory file descriptor for reading the input data
    z_stream z;
    uint32_t offset;
    uint32_t filebuffoff;
    uint32_t filebuffsz;
    int      zipwbits;
    int      count;
    uint8_t *in;
    uint8_t *out;
    uint8_t *filebuff;
} offzip_ctx;

static int buffread(offzip_ctx *ctx, uint8_t *buff, int size);
static void buffseek(offzip_ctx *ctx, int off, int mode);
static void buffinc(offzip_ctx *ctx, int increase);
static int unzip_all(offzip_ctx *ctx, offzip_t* out_list);
static int unzip(offzip_ctx *ctx, uint32_t *inlen, uint32_t *outlen, uint8_t **dump);
static int zlib_err(const offzip_ctx *ctx, int err);


// Custom memory file operations
// Read from memory buffer
static size_t memfread(void *ptr, size_t size, size_t nmemb, MEMFILE *mf) {
    size_t total_bytes = size * nmemb;
    size_t bytes_available = mf->size - mf->position;
    size_t bytes_to_read = (total_bytes < bytes_available) ? total_bytes : bytes_available;
    
    if (bytes_to_read > 0) {
        memcpy(ptr, mf->buffer + mf->position, bytes_to_read);
        mf->position += bytes_to_read;
    }
    
    // Return number of complete items read
    return bytes_to_read / size;
}

// Seek within memory buffer
static int memfseek(MEMFILE *mf, long offset, int whence) {
    size_t new_position;
    
    switch (whence) {
        case SEEK_SET:
            new_position = offset;
            break;
        case SEEK_CUR:
        case SEEK_END:
        default:
            return -1;
    }
    
    if (new_position > mf->size) {
        return -1;
    }
    
    mf->position = new_position;
    return 0;
}

// Get current position
static long memftell(MEMFILE *mf) {
    return (long)mf->position;
}

void* offzip_init(const uint8_t *data, size_t dsz, int wbits) {
    offzip_ctx *ctx = calloc(1, sizeof(offzip_ctx));

    if(!ctx) {
        LOG("Error: unable to allocate offzip context");
        return NULL;
    }

    ctx->memfd.position = 0;
    ctx->memfd.size     = dsz;
    ctx->memfd.buffer   = (uint8_t*)data;
    ctx->zipwbits       = wbits;

    if(inflateInit2(&ctx->z, ctx->zipwbits) != Z_OK) {
        free(ctx);
        return NULL;
    }

    ctx->in        = malloc(INSZ);
    ctx->out       = malloc(OUTSZ);
    ctx->filebuff  = malloc(FBUFFSZ);
    if(!ctx->in || !ctx->out || !ctx->filebuff) {
        offzip_free(ctx);
        return NULL;
    }

    return ctx;
}

void offzip_free(void *handle) {
    offzip_ctx *ctx = handle;

    if(!ctx) return;

    inflateEnd(&ctx->z);
    free(ctx->in);
    free(ctx->out);
    free(ctx->filebuff);
    free(ctx);
}

offzip_t* offzip_util(const uint8_t* data, size_t dlen, int offset, int wbits, int count) {
    offzip_ctx *ctx;
    offzip_t *ofz = NULL;
    int     files;

    LOG("Offzip "VER" by Luigi Auriemma / aluigi@autistici.org / aluigi.org");

    ctx = offzip_init(data, dlen, wbits);
    if(!ctx)
    {
        LOG("Error: unable to create buffers");
        return NULL;
    }

    ctx->count  = count;
    ctx->offset = offset;

    ofz = calloc(count ? (count+1) : (MAX_RESULTS+1), sizeof(offzip_t));
    if(!ofz) {
        LOG("Error: unable to create offzip list");
        offzip_free(ctx);
        return NULL;
    }

    LOG("- zip data to check:  %d bytes", g_minzip);
    LOG("- zip windowBits:     %d", ctx->zipwbits);
    LOG("- seek offset:        0x%08x  (%u)", ctx->offset, ctx->offset);
    LOG("- scan count :        %d", ctx->count);
    buffseek(ctx, ctx->offset, SEEK_SET);

    LOG("+------------+-------------+-------------------------+");
    LOG("| hex_offset | blocks_dots | zip_size --> unzip_size |");
    LOG("+------------+-------------+-------------------------+");

    files = unzip_all(ctx, ofz); //ZIPDOFILE
    if(files) {
        LOG("- %u valid zip blocks found", files);
    } else {
        LOG("- no valid full zip data found");
        free(ofz);
        ofz = NULL;
    }

    offzip_free(ctx);

    return(ofz);
}

static int buffread(offzip_ctx *ctx, uint8_t *buff, int size) {
    int     len,
            rest,
            ret;

    rest = ctx->filebuffsz - ctx->filebuffoff;

    ret = size;
    if(rest < size) {
        ret = size - rest;
        memmove(ctx->filebuff, ctx->filebuff + ctx->filebuffoff, rest);
        len = memfread(ctx->filebuff + rest, 1, FBUFFSZ - rest, &ctx->memfd);
        ctx->filebuffoff = 0;
        ctx->filebuffsz  = rest + len;
        if(len < ret) {
            ret = rest + len;
        } else {
            ret = size;
        }
    }

    memcpy(buff, ctx->filebuff + ctx->filebuffoff, ret);
    return ret;
}

static void buffseek(offzip_ctx *ctx, int off, int mode) {
    if(memfseek(&ctx->memfd, off, mode) < 0)
    {
        LOG("Error: buffseek");
        return;
    }
    ctx->filebuffoff = 0;
    ctx->filebuffsz  = 0;
    ctx->offset      = memftell(&ctx->memfd);
}

static void buffinc(offzip_ctx *ctx, int increase) {
    ctx->filebuffoff += increase;
    ctx->offset      += increase;
}

int offzip_search(void *fd) {
    offzip_ctx *ctx = fd;
    int     len,
            zerr,
            ret;

    for(ret = - 1; (len = buffread(ctx, ctx->in, g_minzip)) >= g_minzip; buffinc(ctx, 1)) {
        ctx->z.next_in   = ctx->in;
        ctx->z.avail_in  = len;
        ctx->z.next_out  = ctx->out;
        ctx->z.avail_out = OUTSZ;

        inflateReset(&ctx->z);
        zerr = inflate(&ctx->z, Z_SYNC_FLUSH);

        if(zerr == Z_OK) {  // do not use Z_STREAM_END here! gives only troubles!!!
            LOG("Zip found at 0x%08x offset", ctx->offset);

            ret = 0;
            break;
        }

        if(!(ctx->offset & SHOWX))
            LOG("Scanned 0x%08x offset", ctx->offset);
    }
    return ret;
}

static int unzip_all(offzip_ctx *ctx, offzip_t* out_list) {
    uint8_t  *fdo = NULL;
    uint32_t inlen,
            outlen;
    int     extracted = 0;

    while(!offzip_search(ctx) && ((!ctx->count && extracted < MAX_RESULTS) || (ctx->count && extracted < ctx->count))) {
        LOG("Unzip (0x%08x) to %08" PRIx32 ".dat", ctx->offset, ctx->offset);

        if(unzip(ctx, &inlen, &outlen, &fdo) != Z_OK) {
            // error during unzip
            free(fdo);
            fdo = NULL;
            continue;
        }

        out_list[extracted].data    = fdo;
        out_list[extracted].offset  = (ctx->offset - inlen);
        out_list[extracted].wbits   = ctx->zipwbits;
        out_list[extracted].ziplen  = inlen;
        out_list[extracted].outlen  = outlen;

        extracted++;
        LOG("#%d: %u --> %u", extracted, inlen, outlen);
    }

    return extracted;
}

static int unzip(offzip_ctx *ctx, uint32_t *inlen, uint32_t *outlen, uint8_t **dump) {
    void *ptr;
    uint32_t oldsz = 0,
            oldoff,
            len;
    int     ret     = -1,
            zerr    = Z_OK;

    *dump = NULL;
    oldoff = ctx->offset;
    inflateReset(&ctx->z);

    for(; (len = buffread(ctx, ctx->in, INSZ)); buffinc(ctx, len)) {
        ctx->z.next_in   = ctx->in;
        ctx->z.avail_in  = len;
        do {
            ctx->z.next_out  = ctx->out;
            ctx->z.avail_out = OUTSZ;
            zerr = inflate(&ctx->z, Z_SYNC_FLUSH);

            ptr = realloc(*dump, ctx->z.total_out);
            if(!ptr) {
                LOG("Error: unable to realloc memory for unzip data");
                return -1;
            }
            *dump = ptr;
            memcpy((*dump) + oldsz, ctx->out, ctx->z.total_out - oldsz);
            oldsz = ctx->z.total_out;

            if(zerr != Z_OK) {      // inflate() return value MUST be handled now
                if(zerr == Z_STREAM_END) {
                    ret = 0;
                } else {
                    zlib_err(ctx, zerr);
                }
                break;
            }
            ret = 0;    // it's better to return 0 even if the z stream is incomplete... or not?
        } while(ctx->z.avail_in);

        if(zerr != Z_OK) break;     // Z_STREAM_END included, for avoiding "goto"
    }

    *inlen  = ctx->z.total_in;
    *outlen = ctx->z.total_out;
    if(!ret) {
        oldoff += ctx->z.total_in;
    } else {
        oldoff++;
    }
    buffseek(ctx, oldoff, SEEK_SET);
    return ret;
}

static int zlib_err(const offzip_ctx *ctx, int zerr) {
    switch(zerr) {
        case Z_DATA_ERROR: {
            LOG("- zlib Z_DATA_ERROR, the data in the file is not in zip format"
                "  or uses a different windowBits value (-z). Try to use -z %d",
                -ctx->zipwbits);
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

int offzip_verify(void *fd, uint32_t *offset, uint32_t *inlen, uint32_t *outlen) {
    offzip_ctx *ctx = fd;
    uint32_t oldoff,
            len;
    int     ret     = -1,
            zerr    = Z_OK;

    oldoff = ctx->offset;
    *offset = ctx->offset;
    inflateReset(&ctx->z);

    for(; (len = buffread(ctx, ctx->in, INSZ)); buffinc(ctx, len)) {
        ctx->z.next_in   = ctx->in;
        ctx->z.avail_in  = len;
        do {
            ctx->z.next_out  = ctx->out;
            ctx->z.avail_out = OUTSZ;
            zerr = inflate(&ctx->z, Z_SYNC_FLUSH);

            if(zerr != Z_OK) {      // inflate() return value MUST be handled now
                if(zerr == Z_STREAM_END) {
                    ret = 0;
                } else {
                    zlib_err(ctx, zerr);
                }
                break;
            }
            ret = 0;    // it's better to return 0 even if the z stream is incomplete... or not?
        } while(ctx->z.avail_in);

        if(zerr != Z_OK) break;     // Z_STREAM_END included, for avoiding "goto"
    }

    *inlen  = ctx->z.total_in;
    *outlen = ctx->z.total_out;
    if(!ret) {
        oldoff += ctx->z.total_in;
    } else {
        oldoff++;
    }
    buffseek(ctx, oldoff, SEEK_SET);
    return ret;
}
