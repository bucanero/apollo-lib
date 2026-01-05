#ifndef CRC_H_INCLUDED
#define CRC_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * read_le_uint16: read an unsigned 16 bits Little Endian
 * value from a buffer
 */
#define read_le_uint16(buf) \
    ((uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8))

#define read_be_uint16(buf) \
    ((uint16_t)(buf[1]) | ((uint16_t)(buf[0]) << 8))

/*
 * read_le_uint32: read an unsigned 32 bits Little Endian
 * value from a buffer
 */
#define read_le_uint32(buf) \
    ((uint32_t)(buf[0]) | ((uint32_t)(buf[1]) << 8) | \
    ((uint32_t)(buf[2]) << 16) | ((uint32_t)(buf[3]) << 24))

#define read_be_uint32(buf) \
    ((uint32_t)(buf[3]) | ((uint32_t)(buf[2]) << 8) | \
    ((uint32_t)(buf[1]) << 16) | ((uint32_t)(buf[0]) << 24))

/*
 * read_le_uint64: read an unsigned 64 bits Little Endian
 * value from a buffer
 */
#define read_le_uint64(buf) \
    ((uint64_t)(buf[0]) | ((uint64_t)(buf[1]) << 8) | \
    ((uint64_t)(buf[2]) << 16) | ((uint64_t)(buf[3]) << 24) | \
    ((uint64_t)(buf[4]) << 32) | ((uint64_t)(buf[5]) << 40) | \
    ((uint64_t)(buf[6]) << 48) | ((uint64_t)(buf[7]) << 56))

#define read_be_uint64(buf) \
    ((uint64_t)(buf[7]) | ((uint64_t)(buf[6]) << 8) | \
    ((uint64_t)(buf[5]) << 16) | ((uint64_t)(buf[4]) << 24) | \
    ((uint64_t)(buf[3]) << 32) | ((uint64_t)(buf[2]) << 40) | \
    ((uint64_t)(buf[1]) << 48) | ((uint64_t)(buf[0]) << 56))

/* FNV-1 init arbitrary value */
#define FNV1_INIT_VALUE                     0x811c9dc5

/* The golden ration: an arbitrary value */
#define JHASH_GOLDEN_RATIO                  0x9e3779b9

/* Jenkins Lookup3 init arbitrary value */
#define LOOKUP3_INIT_VALUE                  0xDEADBEEF

#define MOD_ADLER_16                        251
#define MOD_ADLER_32                        65521

/* ---------- Defines for 16-bit CRC/XMODEM calculation (Not reflected) --------------------------------------------------------- */
#define CRC_16_RESULT_WIDTH                 16u
#define CRC_16_POLYNOMIAL                   0x1021u
#define CRC_16_INIT_VALUE                   0x0000u
#define CRC_16_XOR_VALUE                    0x0000u

// 16-bit CCITT
#define CRC_16_INIT_CCITT                   0xFFFFu

/* ---------- Defines for 32-bit CRC/CCITT calculation (Reflected) -------------------------------------------------------------- */
#define CRC_32_RESULT_WIDTH                 32u
#define CRC_32_POLYNOMIAL                   0x04C11DB7u
#define CRC_32_INIT_VALUE                   0xFFFFFFFFu
#define CRC_32_XOR_VALUE                    0xFFFFFFFFu

// Reversed polynomial
#define CRC_32_REVERSED_POLY                0xEDB88320u

/* ---------- Defines for 64-bit CRC calculation -------------------------------------------------------------- */
#define CRC_64_RESULT_WIDTH                 64u
#define CRC_64_ECMA182_POLY                 0x42F0E1EBA9EA3693
#define CRC_64_ECMA182_INIT_VALUE           0x0000000000000000
#define CRC_64_ECMA182_XOR_VALUE            0x0000000000000000

// 64-bit GO-ISO
#define CRC_64_ISO_POLY                     0x000000000000001B
#define CRC_64_ISO_INIT_VALUE               0xFFFFFFFFFFFFFFFF
#define CRC_64_ISO_XOR_VALUE                0xFFFFFFFFFFFFFFFF


#ifdef __cplusplus
}
#endif

#endif // CRC_H_INCLUDED
