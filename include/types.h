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
#define HOST_LSB(X)		(X)
#else
#define HOST_LSB(X)		0
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

typedef enum
{
	APOLLO_ENDIAN_LITTLE = 0,
	APOLLO_ENDIAN_BIG = 1,
} apollo_endianness_t;

static inline apollo_endianness_t apollo_get_host_endianness(void)
{
	const uint16_t value = 0x0102;
	return (((const uint8_t*) &value)[0] == 0x01) ? APOLLO_ENDIAN_BIG : APOLLO_ENDIAN_LITTLE;
}

static inline uint16_t apollo_convert_u16(uint16_t value, apollo_endianness_t from, apollo_endianness_t to)
{
	return (from == to) ? value : ES16(value);
}

static inline uint32_t apollo_convert_u32(uint32_t value, apollo_endianness_t from, apollo_endianness_t to)
{
	return (from == to) ? value : ES32(value);
}

static inline uint64_t apollo_convert_u64(uint64_t value, apollo_endianness_t from, apollo_endianness_t to)
{
	return (from == to) ? value : ES64(value);
}

static inline uint64_t apollo_read_uint(const void* src, size_t size, apollo_endianness_t endian)
{
	const uint8_t* bytes = (const uint8_t*) src;
	uint64_t value = 0;

	if (endian == APOLLO_ENDIAN_BIG)
	{
		for (size_t i = 0; i < size; i++)
			value = (value << 8) | bytes[i];
	}
	else
	{
		for (size_t i = 0; i < size; i++)
			value |= ((uint64_t) bytes[i]) << (8 * i);
	}

	return value;
}

static inline uint16_t apollo_read_u16(const void* src, apollo_endianness_t endian)
{
	return (uint16_t) apollo_read_uint(src, sizeof(uint16_t), endian);
}

static inline uint32_t apollo_read_u32(const void* src, apollo_endianness_t endian)
{
	return (uint32_t) apollo_read_uint(src, sizeof(uint32_t), endian);
}

static inline uint64_t apollo_read_u64(const void* src, apollo_endianness_t endian)
{
	return apollo_read_uint(src, sizeof(uint64_t), endian);
}

static inline void apollo_write_uint(void* dst, uint64_t value, size_t size, apollo_endianness_t endian)
{
	uint8_t* bytes = (uint8_t*) dst;

	if (endian == APOLLO_ENDIAN_BIG)
	{
		for (size_t i = 0; i < size; i++)
			bytes[i] = (uint8_t) ((value >> (8 * (size - i - 1))) & 0xFF);
	}
	else
	{
		for (size_t i = 0; i < size; i++)
			bytes[i] = (uint8_t) ((value >> (8 * i)) & 0xFF);
	}
}

static inline void apollo_write_u16(void* dst, uint16_t value, apollo_endianness_t endian)
{
	apollo_write_uint(dst, value, sizeof(uint16_t), endian);
}

static inline void apollo_write_u32(void* dst, uint32_t value, apollo_endianness_t endian)
{
	apollo_write_uint(dst, value, sizeof(uint32_t), endian);
}

static inline void apollo_write_u64(void* dst, uint64_t value, apollo_endianness_t endian)
{
	apollo_write_uint(dst, value, sizeof(uint64_t), endian);
}

#define BE16(var)		do { (var) = apollo_convert_u16((uint16_t) (var), apollo_get_host_endianness(), APOLLO_ENDIAN_BIG); } while (0)
#define BE32(var)		do { (var) = apollo_convert_u32((uint32_t) (var), apollo_get_host_endianness(), APOLLO_ENDIAN_BIG); } while (0)
#define BE64(var)		do { (var) = apollo_convert_u64((uint64_t) (var), apollo_get_host_endianness(), APOLLO_ENDIAN_BIG); } while (0)
#define LE16(var)		do { (var) = apollo_convert_u16((uint16_t) (var), apollo_get_host_endianness(), APOLLO_ENDIAN_LITTLE); } while (0)
#define LE32(var)		do { (var) = apollo_convert_u32((uint32_t) (var), apollo_get_host_endianness(), APOLLO_ENDIAN_LITTLE); } while (0)
#define LE64(var)		do { (var) = apollo_convert_u64((uint64_t) (var), apollo_get_host_endianness(), APOLLO_ENDIAN_LITTLE); } while (0)

#endif /* !_TYPES_H_ */
