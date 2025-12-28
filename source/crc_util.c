#include <stdio.h>
#include <polarssl/sha1.h>
#include "apollo.h"
#include "crc_util.h"

#define SW4_OFF_1      0x00004
#define SW4_OFF_2      0x000A8
#define SW4_OFF_3      0x00C16
#define SW4_OFF_JP     0x58298


/**********************************************************************
 * Description: Slow and fast implementations of the CRC standards.
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#define CREATE_CRC_FUNCTION(UINT, CRC_WIDTH) \
    UINT crc##CRC_WIDTH##_hash (const uint8_t* data, uint32_t len, custom_crc_t* cfg) \
    { \
        UINT crc = (UINT)cfg->init; \
        /* Perform modulo-2 division, a byte at a time. */ \
        while (len--) { \
            /* Bring the next byte into the remainder. */ \
            crc ^= ((UINT)(cfg->refIn ? (uint8_t)reflect(*data++, 8) : *data++) << (CRC_WIDTH - 8)); \
            /* Perform modulo-2 division, a bit at a time. */ \
            for (uint8_t bit = 8; bit > 0; --bit) \
                /* Try to divide the current data bit. */ \
                crc = (crc & ((UINT)1 << (CRC_WIDTH - 1))) ? (crc << 1) ^ (UINT)cfg->poly : (crc << 1); \
        } \
        /* The final remainder is the CRC result. */ \
        if (cfg->refOut) \
            crc = (UINT)reflect(crc, CRC_WIDTH); \
        return (crc ^ (UINT)cfg->xor); \
    }

static uint64_t reflect(uint64_t data, uint8_t nBits)
{
    uint64_t reflection = 0;
    uint8_t bit;
    /*
     * Reflect the data about the center bit.
     */
    for (bit = 0; bit < nBits; ++bit)
    {
        /*
         * If the LSB bit is set, set the reflection of it.
         */
        if (data & 0x01)
            reflection |= ((uint64_t)1 << ((nBits - 1) - bit));

        data = (data >> 1);
    }

    return (reflection);

}   /* reflect() */

/* crc16_hash() */
CREATE_CRC_FUNCTION(uint16_t, 16)

/* crc32_hash() */
CREATE_CRC_FUNCTION(uint32_t, 32)

/* crc64_hash() */
CREATE_CRC_FUNCTION(uint64_t, 64)

/* 
 * CRC-32 forcer (C)
 * https://github.com/nayuki/Nayuki-web-published-code/blob/master/forcing-a-files-crc-to-any-value/forcecrc32.c
 * 
 * Copyright (c) 2022 Project Nayuki
 * https://www.nayuki.io/page/forcing-a-files-crc-to-any-value
 */
 
// Returns polynomial x multiplied by polynomial y modulo the generator polynomial.
static uint64_t multiply_mod(uint64_t x, uint64_t y) {
	// Russian peasant multiplication algorithm
	uint64_t z = 0;
	while (y != 0) {
		z ^= x * (y & 1);
		y >>= 1;
		x <<= 1;
		if (((x >> 32) & 1) != 0)
			x ^= (CRC_32_POLYNOMIAL + 0x100000000);
	}
	return z;
}

// Returns polynomial x to the power of natural number y modulo the generator polynomial.
static uint64_t pow_mod(uint64_t x, uint64_t y) {
	// Exponentiation by squaring
	uint64_t z = 1;
	while (y != 0) {
		if ((y & 1) != 0)
			z = multiply_mod(z, x);
		x = multiply_mod(x, x);
		y >>= 1;
	}
	return z;
}

static int get_degree(uint64_t x) {
	int result = -1;
	for (; x != 0; x >>= 1)
		result++;
	return result;
}

// Computes polynomial x divided by polynomial y, returning the quotient and remainder.
static void divide_and_remainder(uint64_t x, uint64_t y, uint64_t* q, uint64_t* r) {
	if (y == 0) {
		// ERROR "Division by zero"
		return;
	}
	if (x == 0) {
		*q = 0;
		*r = 0;
		return;
	}

	int ydeg = get_degree(y);
	uint64_t z = 0;
	for (int i = get_degree(x) - ydeg; i >= 0; i--) {
		if (((x >> (i + ydeg)) & 1) != 0) {
			x ^= y << i;
			z |= (uint64_t)1 << i;
		}
	}
	*q = z;
	*r = x;
}

// Returns the reciprocal of polynomial x with respect to the generator polynomial.
static uint64_t reciprocal_mod(uint64_t x) {
	// Based on a simplification of the extended Euclidean algorithm
	uint64_t y = x;
	x = (CRC_32_POLYNOMIAL + 0x100000000);
	uint64_t a = 0;
	uint64_t b = 1;
	while (y != 0) {
		uint64_t q, r;
		divide_and_remainder(x, y, &q, &r);
		uint64_t c = a ^ multiply_mod(q, b);
		x = y;
		y = r;
		a = b;
		b = c;
	}
	if (x == 1)
		return a;
	else {
		// ERROR "Reciprocal does not exist"
		return 0;
	}
}

int force_crc32(const uint8_t *data, uint32_t length, uint32_t offset, uint32_t newcrc) {
	int ret = 0;
	custom_crc_t cfg = {
		.init = CRC_32_INIT_VALUE,
		.poly = CRC_32_POLYNOMIAL,
		.xor = CRC_32_XOR_VALUE,
		.refIn = 1,
		.refOut = 0,
	};

	if (length < 4 || offset > length - 4) {
		// Error: Byte offset plus 4 exceeds file length
		return 0;
	}

	// Read entire file and calculate original CRC-32 value.
	uint32_t delta = (uint32_t)reflect(newcrc, 32) ^ crc32_hash(data, length, &cfg);
	// Compute the change to make
	delta = (uint32_t)multiply_mod(reciprocal_mod(pow_mod(2, (length - offset) * 8)), delta);
	delta = (uint32_t)reflect(delta, 32);

	// Patch 4 bytes in the file
	for (int i = 0; i < 4; i++) {
		ret |= (data[offset + i] ^ ((delta >> (i * 8)) & 0xFF)) << (24 - i*8);
	}

	return ret;
}

/* ------------------------------------------------------------------ */

static void generate_crc16_table(uint16_t poly, uint16_t* crc_table)
{
    for (int i = 0; i < 256; ++i)
    {
        uint16_t r = i << 8;
        
        for (int j = 0; j < 8; ++j)
            r = (r & 0x8000) ? (r << 1) ^ poly : (r << 1);
        
        crc_table[i] = r;
    }
}

static void generate_crc32_table(uint32_t poly, uint32_t* crc_table)
{
    for (int i = 0; i < 256; ++i)
    {
        uint32_t r = i << 24;
        
        for (int j = 0; j < 8; ++j)
            r = (r & 0x80000000) ? (r << 1) ^ poly : (r << 1);
        
        crc_table[i] = r;
    }
}

static uint32_t add_csum(const uint8_t* data, uint32_t len)
{
    uint32_t checksum = 0;

    while (len--)
        checksum += *data++;

    return checksum;
}

// Custom CRC table for Kingdom Hearts 2.5
static void kh25_crc32_table(uint32_t poly, uint32_t* crc_table)
{
    for (int x = 0; x < 0x100; x++)
    {
        int r = x << 24;
        for (int j = 0; j < 0xff; j++)
            r = (r << 1) ^ (r < 0 ? poly : 0);

        crc_table[x] = r;
    }
}

uint32_t kh25_hash(const uint8_t* data, uint32_t len)
{
    uint32_t crc32_table[256];
    uint32_t crc = CRC_32_INIT_VALUE;

    kh25_crc32_table(CRC_32_POLYNOMIAL, crc32_table);

    for (int i = 0; i < 8; i++, len--)
        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *data++) & 0xFF];

    // skip crc bytes (0x8..0xB)
    len -= 4;
    data += 4;

    while (len--)
        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *data++) & 0xFF];

    return (~crc);
}

uint32_t kh_com_hash(const uint8_t* data, uint32_t len)
{
    int csum = CRC_32_INIT_VALUE;

    while (len--)
    {
        csum ^= (*data++ << 31);
        csum = (csum << 1) ^ (csum < 0 ? CRC_32_POLYNOMIAL : 0);
    }

    return (~csum);
}

// "MC02" Electronic Arts hash table
// https://gist.github.com/Experiment5X/5025310 / https://ideone.com/cy2rM7
// I have no clue how this works and understand absolutely none of the math behind it.
// I just reversed it from Dead Space 3.
uint32_t MC02_hash(const uint8_t *pb, uint32_t cb)
{
	uint32_t MC02_table[0x100];
	generate_crc32_table(CRC_32_POLYNOMIAL, MC02_table);

	if (cb < 4)
		return 0;

	uint32_t rotatedThird = (pb[2] << 8) | (pb[2] >> 24);
	uint32_t ORedFirstPair = ((pb[0] << 24) | (pb[0] >> 8)) | ((pb[1] << 16) | (pb[1] >> 16));

	uint32_t seedValue = ~((rotatedThird | ORedFirstPair) | pb[3]);
	pb += 4;
	cb -= 4;

	for (uint32_t i = 0; i < cb; i++)
	{
		uint32_t lookedUpValue = MC02_table[((seedValue >> 22) & 0x3FC) >> 2];
		uint32_t insertedNum = pb[i] | (seedValue << 8);
		seedValue = lookedUpValue ^ insertedNum;
	}

	return ~seedValue;
}

// http://www.cse.yorku.ca/~oz/hash.html#djb2
uint32_t djb2_hash(const uint8_t* data, uint32_t len)
{
    uint32_t hash = 5381;

    while (len--)
        hash = ((hash << 5) + hash) + *data++; /* hash * 33 + c */

    return hash;
}

// http://www.cse.yorku.ca/~oz/hash.html#sdbm
uint32_t sdbm_hash(const uint8_t* data, uint32_t len, uint32_t init)
{
    uint32_t crc = init;
    
    while (len--)
        crc = (crc * 0x1003f) + *data++;

    return (crc);
}

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
int fnv1_hash(const uint8_t* data, uint32_t size, int init)
{
    int sum = init;

    while (size--)
        sum = (sum * 0x1000193) ^ *data++;

    return (sum);
}

int Checksum32_hash(const uint8_t* data, uint32_t size)
{
    int sum = 0;

    while (size--)
        sum += (signed char) *data++;

    return sum;
}

uint16_t ffx_hash(const uint8_t* data, uint32_t len)
{
    uint16_t crc16_table[0x100];
    uint16_t crc = CRC_16_INIT_CCITT;

    // FFX-2 & FFX (or PS2 Bug?) - Table Last Entry Should be 0x1EF0, but is 0x0000
    generate_crc16_table(CRC_16_POLYNOMIAL, crc16_table);
    crc16_table[0xFF] = 0;

    while (len--)
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ *data++) & 0xFF];

    return (~crc);
}

// lookup3.c, by Bob Jenkins, May 2006, Public Domain.
// https://www.burtleburtle.net/bob/c/lookup3.c
#define __rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define lookup3_mix(a,b,c) \
{ \
  a -= c;  a ^= __rot(c, 4);  c += b; \
  b -= a;  b ^= __rot(a, 6);  a += c; \
  c -= b;  c ^= __rot(b, 8);  b += a; \
  a -= c;  a ^= __rot(c,16);  c += b; \
  b -= a;  b ^= __rot(a,19);  a += c; \
  c -= b;  c ^= __rot(b, 4);  b += a; \
}

#define lookup3_final(a,b,c) \
{ \
  c ^= b; c -= __rot(b,14); \
  a ^= c; a -= __rot(c,11); \
  b ^= a; b -= __rot(a,25); \
  c ^= b; c -= __rot(b,16); \
  a ^= c; a -= __rot(c,4);  \
  b ^= a; b -= __rot(a,14); \
  c ^= b; c -= __rot(b,24); \
}

/*
 * hashlittle2: return 2 32-bit hash values
 *
 * This is identical to hashlittle(), except it returns two 32-bit hash
 * values instead of just one.  This is good enough for hash table
 * lookup with 2^^64 buckets, or if you want a second hash if you're not
 * happy with the first, or if you want a probably-unique 64-bit ID for
 * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 */
void lookup3_hashlittle2(
  const uint8_t *k,      /* the key to hash */
  size_t      length,    /* length of the key */
  uint32_t   *pc,        /* IN: primary initval, OUT: primary hash */
  uint32_t   *pb)        /* IN: secondary initval, OUT: secondary hash */
{
    uint32_t a,b,c;                                          /* internal state */

    /* Set up the internal state */
    a = b = c = LOOKUP3_INIT_VALUE + ((uint32_t)length) + *pc;
    c += *pb;

    /* need to read the key one byte at a time */
    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
        a += k[0];
        a += ((uint32_t)k[1])<<8;
        a += ((uint32_t)k[2])<<16;
        a += ((uint32_t)k[3])<<24;
        b += k[4];
        b += ((uint32_t)k[5])<<8;
        b += ((uint32_t)k[6])<<16;
        b += ((uint32_t)k[7])<<24;
        c += k[8];
        c += ((uint32_t)k[9])<<8;
        c += ((uint32_t)k[10])<<16;
        c += ((uint32_t)k[11])<<24;
        lookup3_mix(a,b,c);
        length -= 12;
        k += 12;
    }
    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

    lookup3_final(a,b,c);
    *pc=c; *pb=b;
}

int sw4_hash(const uint8_t* data, uint32_t size, uint32_t* crcs)
{
	if (size < SW4_OFF_JP)
	{
		memset(crcs, 0, 4 * sizeof(uint32_t));
		return 0;
	}

	uint32_t num1, num2, num3, num4, num5, num6;
	uint8_t is_jp = (*(uint32_t*)(data + SW4_OFF_JP) != 0);

	num2 = add_csum(data + SW4_OFF_1 + 4, SW4_OFF_2 - (SW4_OFF_1 + 4));
	num3 = add_csum(data + SW4_OFF_2 + 4, 2284 - (SW4_OFF_2 + 4));

	if (is_jp)
	{
		num1 = add_csum(data + SW4_OFF_3 + 4, 30294 - (SW4_OFF_3 + 4));
		num4 = add_csum(data + 33630, 361106 - 33630);
		num5 = add_csum(data + 30294, 33631 - 30294);
		num6 = add_csum(data + SW4_OFF_1 + 4, 30294 - (SW4_OFF_1 + 4));
	}
	else
	{
		num1 = add_csum(data + SW4_OFF_3 + 4, 30934 - (SW4_OFF_3 + 4));
		num4 = add_csum(data + 34270, 361778 - 34270);
		num5 = add_csum(data + 30934, 34271 - 30934);
		num6 = add_csum(data + SW4_OFF_1 + 4, 30934 - (SW4_OFF_1 + 4));
	}

	crcs[0] = 0x7FFFFFFF & (num5 + (num1 + num2) * num3);
	crcs[1] = 0x7FFFFFFF & (num2 * (num1 + num3 + num4));
    crcs[2] = num6;
	crcs[3] = 0x7FFFFFFF & (crcs[0] + crcs[1] + num6);

    return is_jp;
}

int mgs2_hash(const uint8_t* data, uint32_t size)
{
    int num, crc = -1;

    while (size--)
    {
        crc = crc ^ *data++ << 24 >> 24;
        for (int j = 0; j < 8; j++)
        {
            num = crc & 1;
            crc = crc >> 1 & 0x7fffffff;

            if (num != 0)
                crc ^= CRC_32_REVERSED_POLY;
        }
    }

    return ~crc;
}

uint32_t tiara2_hash(const uint8_t* data, uint32_t len)
{
	uint32_t crc = 1;
	uint32_t add = 0x3428;

	while (len--)
	{
		add += *data++;
		crc = (crc * add) + add;
	}

	return (crc);
}

static void _toz_sha1(const uint8_t* data, uint32_t length, const char* key, uint8_t* hash_out)
{
    sha1_context ctx;

    sha1_starts(&ctx);
    sha1_update(&ctx, data, length);
    sha1_update(&ctx, (const unsigned char*)key, strlen(key));
    sha1_finish(&ctx, hash_out);
}

// https://github.com/bucanero/ps3-save-decrypters/blob/master/toz-checksum-fixer/samples/Crypto.txt
void toz_hash(const uint8_t* data, uint32_t len, uint8_t* hash)
{
    const char array[8][4] = {"SRA", "ROS", "MIC", "LAI", "EDN", "DEZ", "ZAB", "ALI"};

    _toz_sha1(data, len, "TO12", hash);

    for (int i = 0; i < 100; i++)
        _toz_sha1(hash, 20, array[i % 8], hash);

    return;
}

uint16_t adler16(const uint8_t *data, size_t len)
/* 
    where data is the location of the data in physical memory and 
    len is the length of the data in bytes 
*/
{
    uint32_t a = 1, b = 0;

    // Process each byte of the data in order
    while (len--)
    {
        a = (a + *data++) % MOD_ADLER_16;
        b = (b + a) % MOD_ADLER_16;
    }

    return ((b << 8) | a);
}

int deadrising_checksum(uint8_t* data, uint32_t size)
{
    uint16_t sumL, sumH;
    uint8_t* e = data + size;

    for (uint8_t* s = data; s < e; s += 4)
    {
        sumL = sumH = 0;
        for (int n = 0; n < 16; ++n, ++s)
        {
            sumL += *s;
            sumH += (uint16_t) ((n % 2) == 0 ? *s : -*s);
        }

        s[0] = sumL & 0xFF;
        s[1] = (sumL >> 8) & 0xFF;
        s[2] = sumH & 0xFF;
        s[3] = (sumH >> 8) & 0xFF;
    }

    return (size/4);
}

int castlevania_hash(const uint8_t* Bytes, uint32_t length)
{
	int num = 0;
	int num2 = 0;

	for (uint32_t i = 0; i < length; i += 2)
	{
		num += ((int)Bytes[i] ^ (i & 255));
		num2 += ((int)Bytes[i + 1] ^ ((i + 1) & 255));
	}

	return (num + num2);
}

uint64_t dbzxv2_checksum(const uint8_t* data, uint32_t size)
{
    int i;
    const uint8_t* header = data + 0x20;
    uint8_t out[8] = {0};

    // Checksum 8 calculated over decrypted data
    if (memcmp(&header[0x14], out, sizeof(out)) == 0)
    {
        out[7] = 0xFF;
        // For checksum 8
        for (int i = 5; i < (int)(size / 0x20); i++)
            out[1] += data[i * 0x20];

        return *(uint64_t*)out;
    }

    // reload checksum 8
    out[1] = header[0x1A];

    // Checksums 1-7 calculated over encrypted data
    // For checksum 7
    for (i = 0; i < 14; i++)
        out[6] += header[0x6 + i];

    // For checksum 6
    for (i = 0; i < 8; i++)
        out[5] += header[0x1C + (i * 4)];

    // For checksum 5
    for (i = 0; i < 8; i++)
        out[4] += header[0x4C + (i * 4)];

    /// For checksum 4
    for (i = 0; i < 4; i++)
        out[3] += header[0x3C + (i * 4)];

    // For checksum 3
    for (i = 0; i < 4; i++)
        out[2] += header[0x6C + (i * 4)];

    // For checksum 2
    for (i = 5; i < (int)(size / 0x20); i++)
        out[0] += data[i * 0x20];

    // For checksum 1
    out[7] = header[0x5];
    for (i = 0; i < 7; i++)
        out[7] += out[i];

    return *(uint64_t*)out;
}

// https://www.burtleburtle.net/bob/hash/doobs.html
uint32_t jenkins_oaat_hash(const uint8_t* data, size_t length, uint32_t hash)
{
    while (length--)
    {
        hash += (signed char) *data++;
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    return hash;
}

// https://en.wikipedia.org/wiki/MurmurHash
static inline uint32_t murmur_32_scramble(uint32_t k)
{
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t h)
{
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        k = key[0] | (key[1] << 8) | (key[2] << 16) | (key[3] << 24);
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);
    /* Finalize. */
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

/* NOTE: Arguments are modified. */
#define __jhash_mix(a, b, c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

/* The most generic version, hashes an arbitrary sequence
 * of bytes.  No alignment or length assumptions are made about
 * the input key.
 * https://kernel.googlesource.com/pub/scm/linux/kernel/git/klassert/ipsec/+/refs/tags/v2.6.19-rc3/include/linux/jhash.h
 */
uint32_t jhash(const uint8_t *k, uint32_t length, uint32_t initval)
{
    uint32_t a, b, c, len;
    len = length;
    a = b = JHASH_GOLDEN_RATIO;
    c = initval;

    while (len >= 12) {
        a += (k[0] +((uint32_t)k[1]<<8) +((uint32_t)k[2]<<16) +((uint32_t)k[3]<<24));
        b += (k[4] +((uint32_t)k[5]<<8) +((uint32_t)k[6]<<16) +((uint32_t)k[7]<<24));
        c += (k[8] +((uint32_t)k[9]<<8) +((uint32_t)k[10]<<16)+((uint32_t)k[11]<<24));
        __jhash_mix(a,b,c);
        k += 12;
        len -= 12;
    }
    c += length;

    switch (len) {
        case 11: c += ((uint32_t)k[10]<<24);
        case 10: c += ((uint32_t)k[9]<<16);
        case 9 : c += ((uint32_t)k[8]<<8);
        case 8 : b += ((uint32_t)k[7]<<24);
        case 7 : b += ((uint32_t)k[6]<<16);
        case 6 : b += ((uint32_t)k[5]<<8);
        case 5 : b += k[4];
        case 4 : a += ((uint32_t)k[3]<<24);
        case 3 : a += ((uint32_t)k[2]<<16);
        case 2 : a += ((uint32_t)k[1]<<8);
        case 1 : a += k[0];
    };
    __jhash_mix(a,b,c);

    return c;
}

uint32_t add_hash(const uint8_t* data, uint32_t len)
{
    uint32_t checksum = 0;

    while (len--)
        checksum += *data++;

    return checksum;
}

uint32_t wadd_hash(const uint8_t* data, uint32_t len, int is_le)
{
    uint32_t checksum = 0;
    len /= 2;

    while (len--)
    {
        checksum += is_le ? read_le_uint16(data) : read_be_uint16(data);
        data += 2;
    }

    return checksum;
}

uint32_t dwadd_hash(const uint8_t* data, uint32_t len, int is_le)
{
    uint32_t checksum = 0;
    len /= 4;

    while (len--)
    {
        checksum += is_le ? read_le_uint32(data) : read_be_uint32(data);
        data += 4;
    }

    return checksum;
}

uint32_t qwadd_hash(const uint8_t* data, uint32_t len)
{
    uint32_t checksum = 0;
    len /= 8;
    data += 4; // skip lower dword

    while (len--)
    {
        checksum += read_be_uint32(data);
        data += 8;
    }

    return checksum;
}

uint32_t wsub_hash(const uint8_t* data, uint32_t len)
{
    uint32_t checksum = 0;
    len /= 2;

    while (len--)
    {
        checksum -= read_be_uint16(data);
        data += 2;
    }

    return checksum;
}
