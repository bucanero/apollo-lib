#ifndef _TYPES_H_
#define _TYPES_H_

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <dbglogger.h>
#define LOG dbglogger_log

#ifdef __PPU__
#include <ppu-types.h>

#define MEM16(...)
#define MEM32(...)
#define MEM64(...)
#define BE16(...)
#define BE32(...)
#define BE64(...)
#define LE16(var)		var = ES16(var)
#define LE32(var)		var = ES32(var)
#define LE64(var)		var = ES64(var)
#define PADDING(X)		X

#elif __PS4__
#define MEM16(...)
#define MEM32(...)
#define MEM64(...)
#define BE16(var)		var = ES16(var)
#define BE32(var)		var = ES32(var)
#define BE64(var)		var = ES64(var)
#define LE16(...)
#define LE32(...)
#define LE64(...)
#define PADDING(X)		0

#elif __PS3_PC__
#define MEM16(var)		var = ES16(var)
#define MEM32(var)		var = ES32(var)
#define MEM64(var)		var = ES64(var)
#define BE16(var)		var = ES16(var)
#define BE32(var)		var = ES32(var)
#define BE64(var)		var = ES64(var)
#define LE16(...)
#define LE32(...)
#define LE64(...)
#define PADDING(X)		X

#else
#define MEM16(...)
#define MEM32(...)
#define MEM64(...)
#define BE16(var)		var = ES16(var)
#define BE32(var)		var = ES32(var)
#define BE64(var)		var = ES64(var)
#define LE16(...)
#define LE32(...)
#define LE64(...)
#define PADDING(X)		0
#endif

/*
 * Offset to the least-significant bytes of a HOST-native integer when
 * truncating it to fewer bytes (e.g. keeping the low 2 bytes of a uint32_t).
 *
 * Unlike PADDING(), which follows the TARGET save-data byte order, this depends
 * on the byte order of the machine actually running the code: only a real
 * big-endian host (PS3/PPU) stores an integer's low-order bytes at the higher
 * address. The PS3-on-PC build (__PS3_PC__) simulates big-endian save DATA but
 * still runs on a little-endian host, so the offset must be 0 there — using
 * PADDING() would slice the wrong (high-order) bytes of the accumulator.
 */
#ifdef __PPU__
#define HOST_LSB(X)		(X)
#else
#define HOST_LSB(X)		0
#endif

/*
 * Offset to the MOST-significant bytes of a host-native integer — the
 * complement of HOST_LSB(). On a big-endian host they sit at the front
 * (offset 0); on a little-endian host at the back. Used by left(), which keeps
 * the leftmost / most-significant bytes of a value regardless of host.
 */
#ifdef __PPU__
#define HOST_MSB(X)		0
#else
#define HOST_MSB(X)		(X)
#endif

#if !defined(MAX_PATH)
#	define MAX_PATH 260
#endif


#define ES16(_val) \
	((uint16_t)(((((uint16_t)_val) & 0xff00) >> 8) | \
	       ((((uint16_t)_val) & 0x00ff) << 8)))

#define ES32(_val) \
	((uint32_t)(((((uint32_t)_val) & 0xff000000) >> 24) | \
	       ((((uint32_t)_val) & 0x00ff0000) >> 8 ) | \
	       ((((uint32_t)_val) & 0x0000ff00) << 8 ) | \
	       ((((uint32_t)_val) & 0x000000ff) << 24)))

#define ES64(_val) \
	((uint64_t)(((((uint64_t)_val) & 0xff00000000000000ull) >> 56) | \
	       ((((uint64_t)_val) & 0x00ff000000000000ull) >> 40) | \
	       ((((uint64_t)_val) & 0x0000ff0000000000ull) >> 24) | \
	       ((((uint64_t)_val) & 0x000000ff00000000ull) >> 8 ) | \
	       ((((uint64_t)_val) & 0x00000000ff000000ull) << 8 ) | \
	       ((((uint64_t)_val) & 0x0000000000ff0000ull) << 24) | \
	       ((((uint64_t)_val) & 0x000000000000ff00ull) << 40) | \
	       ((((uint64_t)_val) & 0x00000000000000ffull) << 56)))

#endif /* !_TYPES_H_ */
