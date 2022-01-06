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

#define BE16(...)
#define LE16(var)		var = ES16(var)
#define BE32(...)
#define LE32(var)		var = ES32(var)
#define BE64(...)
#define LE64(var)		var = ES64(var)
#else

#define BE16(var)		var = ES32(var)
#define LE16(...)
#define BE32(var)		var = ES32(var)
#define LE32(...)
#define BE64(var)		var = ES64(var)
#define LE64(...)
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
