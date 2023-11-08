/**********************************************************************
 *
 * Filename:    crc.h
 * 
 * Description: Slow and fast implementations of the CRC standards.
 *
 * 
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#ifndef CRC_H_INCLUDED
#define CRC_H_INCLUDED


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/* The golden ration: an arbitrary value */
#define JHASH_GOLDEN_RATIO                  0x9e3779b9

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
