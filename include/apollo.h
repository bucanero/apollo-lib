#ifndef _APOLLO_LIB_H_
#define _APOLLO_LIB_H_

#include <stdint.h>
#include <stdlib.h>

#define APOLLO_LIB_VERSION         "3.0.0"

#define APOLLO_CODE_GAMEGENIE      1
#define APOLLO_CODE_SAVEWIZARD     1
#define APOLLO_CODE_BSD            2
#define APOLLO_CODE_PYTHON         3

#define APOLLO_CODE_FLAG_PARENT    1
#define APOLLO_CODE_FLAG_CHILD     2
#define APOLLO_CODE_FLAG_REQUIRED  4
#define APOLLO_CODE_FLAG_ALERT     8
#define APOLLO_CODE_FLAG_EMPTY     16
#define APOLLO_CODE_FLAG_DISABLED  32
#define APOLLO_CODE_FLAG_ORDER_LE  64
#define APOLLO_CODE_FLAG_ORDER_BE  128

#define APOLLO_DATA_MODE_DEFAULT   0
#define APOLLO_DATA_MODE_LITTLE    1
#define APOLLO_DATA_MODE_BIG       2

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    APOLLO_HOST_SYS_NAME,
    APOLLO_HOST_USERNAME,
    APOLLO_HOST_PSID,
    APOLLO_HOST_ACCOUNT_ID,
    APOLLO_HOST_LAN_ADDR,
    APOLLO_HOST_WLAN_ADDR,
    APOLLO_HOST_TEMP_PATH,
    APOLLO_HOST_DATA_PATH,
} apollo_host_data_t;

/* Direction argument shared by every apollo_crypt_* function that has an
   inverse. Values match the MicroPython ucrypto module's DECRYPT/ENCRYPT, so a
   C call and a Python call read the same way. */
typedef enum
{
    APOLLO_DECRYPT = 0,
    APOLLO_ENCRYPT = 1,
} apollo_crypt_mode_t;

typedef struct list_node_s
{
	void *value;
	struct list_node_s *next;
} list_node_t;

typedef struct list_s
{
	list_node_t *head;
	size_t count;
} list_t;

typedef struct option_value
{
    char * name;
    char * value;
} option_value_t;

typedef struct option_entry
{
    int id;
    int sel;
    char * line;
    list_t * opts;
} option_entry_t;

typedef struct code_entry
{
    uint8_t type;
    uint8_t activated;
    uint16_t flags;
    int options_count;
    char * name;
    char * file;
    char * codes;
    option_entry_t * options;
} code_entry_t;

typedef struct
{
    uint8_t width;
    uint64_t poly;
    uint64_t init;
    uint64_t xorOut;   /* was 'xor' — renamed: 'xor' is a reserved token in C++ */
    uint8_t refIn;
    uint8_t refOut;
} custom_crc_t;

typedef void* (*apollo_host_cb_t)(int info, uint32_t* size);
typedef option_entry_t* (*apollo_get_files_cb_t)(const char*, const char*);

//---  Generic list functions ---

list_t * list_alloc(void);
void list_free(list_t *list);

list_node_t * list_append(list_t *list, void *value);

list_node_t * list_head(list_t *list);
list_node_t * list_tail(list_t *list);
size_t list_count(list_t *list);

list_node_t * list_next(list_node_t *node);
void * list_get(list_node_t *node);
void * list_get_item(list_t *list, size_t item);

void list_bubbleSort(list_t *list, int (*compar)(const void *, const void *));


//---  Generic utility functions ---

uint64_t x_to_u64(const char *hex);
uint8_t * x_to_u8_buffer(const char *hex);

int wildcard_match(const char *data, const char *mask);
int wildcard_match_icase(const char *data, const char *mask);

int read_buffer(const char *file_path, uint8_t **buf, size_t *size);
int write_buffer(const char *file_path, const uint8_t *buf, size_t size);


//---  Apollo patch functions ---

void apollo_free_var_list(void);
void apollo_set_endianness(int endian);
int apollo_get_endianness(void);
size_t apollo_apply_sw_code(uint8_t* data, size_t dsize, const code_entry_t* code);
size_t apollo_apply_bsd_code(uint8_t** data, size_t dsize, const code_entry_t* code);
size_t apollo_apply_py_code(uint8_t** src_data, size_t dsize, const code_entry_t* code);
int apollo_apply_code(const char* file_path, const code_entry_t* code, apollo_host_cb_t host_cb);
int apollo_load_code_list(char* buffer, list_t* list_codes, apollo_get_files_cb_t get_files_cb, const char* save_path);


//---  Apollo crypto functions ---
//
// Functions with an inverse take `mode` first: APOLLO_ENCRYPT or APOLLO_DECRYPT.
// The four without one omit it -- CTR and the two XOR streams are self-inverse,
// and MGS5 TPP only ever encodes.

// AES save data crypto
void apollo_crypt_aes_ecb(apollo_crypt_mode_t mode, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len);
void apollo_crypt_aes_cbc(apollo_crypt_mode_t mode, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);
void apollo_crypt_aes_ctr(uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);

// 3-DES save data crypto
void apollo_crypt_des3_ecb(apollo_crypt_mode_t mode, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len);
void apollo_crypt_des3_cbc(apollo_crypt_mode_t mode, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);

// Blowfish save data crypto
void apollo_crypt_blowfish_ecb(apollo_crypt_mode_t mode, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len);
void apollo_crypt_blowfish_cbc(apollo_crypt_mode_t mode, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);

// Camellia save data crypto
void apollo_crypt_camellia_ecb(apollo_crypt_mode_t mode, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len);

// Diablo 3 save data crypto
void apollo_crypt_diablo3(apollo_crypt_mode_t mode, uint8_t* data, uint32_t size);

// Silent Hill 3 save data crypto
void apollo_crypt_silent_hill3(apollo_crypt_mode_t mode, uint8_t* data, uint32_t size);

// NFS Undercover save data crypto
void apollo_crypt_nfs_undercover(apollo_crypt_mode_t mode, uint8_t* data, uint32_t size);

// Final Fantasy XIII (1/2/3) save data crypto
void apollo_crypt_final_fantasy13(apollo_crypt_mode_t mode, uint32_t game, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len);

// Borderlands 3 save data crypto (`is_data`: non-zero for save data, zero for profile)
void apollo_crypt_borderlands3(apollo_crypt_mode_t mode, uint8_t* buffer, int length, int is_data);

// Monster Hunter save data crypto
void apollo_crypt_monster_hunter(apollo_crypt_mode_t mode, uint8_t* buff, uint32_t size, int ver);

// Metal Gear Solid 2/3 HD save data crypto
void apollo_crypt_mgs(apollo_crypt_mode_t mode, uint8_t* data, int size, const char* key, int keylen);
void apollo_crypt_mgs_base64(apollo_crypt_mode_t mode, uint8_t* data, uint32_t size);

// Metal Gear Solid Peace Walker save data crypto
void apollo_crypt_mgs_pw(apollo_crypt_mode_t mode, uint8_t* data, uint32_t len);

// Metal Gear Solid 5 TPP save data crypto (encode only)
void apollo_crypt_mgs5_tpp(uint8_t* data, uint32_t len, uint32_t key);

// Dynasty Warriors 8 Xtreme Legends save data crypto (self-inverse XOR stream)
void apollo_crypt_dw8xl(uint8_t* data, uint32_t len);

// RGG Studio save data crypto (self-inverse XOR)
void apollo_crypt_rgg_studio(uint8_t* data, uint32_t size, const char* key, int key_len);

//---  offZip/packZip functions ---

#define OFFZIP_WBITS_ZLIB		15
#define OFFZIP_WBITS_DEFLATE	-15

typedef struct offzip_list
{
    void* data;
    uint32_t outlen;
    uint32_t offset;
    uint32_t ziplen;
    int wbits;
    uint32_t* ref_outlen; //pointer to the outlen field of the corresponding variable in the var_list
} offzip_t;

/* One-shot convenience: runs a whole scan and returns the results. Manages its
   own handle, so it needs no offzip_free(). */
offzip_t* offzip_util(const uint8_t *data, size_t dlen, int offset, int wbits, int count);

/* Scanning is session-based: offzip_init() returns an opaque handle that owns
   all of the scan state, so independent scans never interfere. Pass the handle
   to search/verify, and release it with offzip_free(). */
void* offzip_init(const uint8_t *data, size_t dsz, int wbits);
int offzip_search(void *offz_fd);
int offzip_verify(void *offz_fd, uint32_t *offset, uint32_t *inlen, uint32_t *outlen);
void offzip_free(void *offz_fd);

int packzip_util(offzip_t *input, uint32_t offset, uint8_t** output, size_t* outsize);


//---  Apollo checksum functions ---

/* hash calculation for MGS: Peace Walker */
uint32_t apollo_hash_mgspw(const uint8_t* data, int size);

/* hash calculation for Final Fantasy XIII */
uint32_t apollo_hash_ff13(const uint8_t* bytes, uint32_t len);

/* checksum update for Dead Rising */
int apollo_hash_deadrising(uint8_t* data, uint32_t size);

/* checksum calculation for DBZ Xenoverse 2 */
uint64_t apollo_hash_dbzxv2(const uint8_t* data, uint32_t size);

/**
 * This function makes a CRC16 calculation on Length data bytes
 *
 * RETURN VALUE: 16 bit result of CRC calculation
 */
uint16_t apollo_hash_crc16(const uint8_t* message, uint32_t nBytes, custom_crc_t* cfg);

/**
 * This function makes a CRC32 calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t apollo_hash_crc32(const uint8_t* message, uint32_t nBytes, custom_crc_t* cfg);

/**
 * This function makes a CRC64 calculation on Length data bytes
 *
 * RETURN VALUE: 64 bit result of CRC calculation
 */
uint64_t apollo_hash_crc64(const uint8_t *data, uint32_t len, custom_crc_t* cfg);

/**
 * This function makes a "MC02" Electronic Arts hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t apollo_hash_mc02(const uint8_t *data, uint32_t len);

/**
 * This function makes a djb2 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t apollo_hash_djb2(const uint8_t* data, uint32_t len);

/**
 * This function makes a SDBM hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t apollo_hash_sdbm(const uint8_t* data, uint32_t len, uint32_t init);

/**
 * This function makes a FNV-1 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
int apollo_hash_fnv1(const uint8_t* data, uint32_t len, int init);

/**
 * This function makes a Checksum32 calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
int apollo_hash_checksum32(const uint8_t* data, uint32_t len);

/**
 * This function makes Adler16 hash calculation on Length data bytes
 *
 * RETURN VALUE: 16 bit result of CRC calculation
 */
uint16_t apollo_hash_adler16(const uint8_t *data, size_t len);

/**
 * This function makes Final Fantasy X hash calculation on Length data bytes
 *
 * RETURN VALUE: 16 bit result of CRC calculation
 */
uint16_t apollo_hash_ffx(const uint8_t* data, uint32_t len);

/**
 * This function makes Kingdom Hearts 2.5 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t apollo_hash_kh25(const uint8_t* data, uint32_t len);

/**
 * This function makes Kingdom Hearts Chain of Memories hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t apollo_hash_khcom(const uint8_t* data, uint32_t len);

/**
 * This function makes Jenkins Lookup3 little2 hash calculation on Length data bytes
 *
 * RETURN VALUES: 32 bit results of CRC calculation
 */
void apollo_hash_lookup3_little2(const uint8_t *k, size_t length, uint32_t *pc, uint32_t *pb);

/**
 * This function makes Samurai Warriors 4 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result array of CRC calculation
 */
int apollo_hash_sw4(const uint8_t* data, uint32_t size, uint32_t* crcs);

/**
 * This function makes MGS2 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
int apollo_hash_mgs2(const uint8_t* data, uint32_t len);

/**
 * This function makes Tears to Tiara 2 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t apollo_hash_tiara2(const uint8_t* data, uint32_t len);

/**
 * This function makes Castlevania LOS 1/2 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
int apollo_hash_castlevania(const uint8_t* Bytes, uint32_t length);

/**
 * This function makes Tales of Zestiria hash calculation on Length data bytes
 *
 * RETURN VALUE: 20 byte result array of SHA1 calculation
 */
void apollo_hash_toz(const uint8_t* data, uint32_t len, uint8_t* sha_hash);

/**
 * This function brute-force a CRC32 hash calculation to match newcrc on Length data bytes
 *
 * RETURN VALUE: 32 bit data result to update offset
 */
int apollo_hash_force_crc32(const uint8_t *data, uint32_t length, uint32_t offset, uint32_t newcrc);

/**
 * This function makes Murmur3 32 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t apollo_hash_murmur3_32(const uint8_t *data, size_t len, uint32_t h);

/**
 * This function makes Jenkins hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of hash calculation
 */
uint32_t apollo_hash_jhash(const uint8_t *data, uint32_t length, uint32_t initval);

/**
 * This function makes Jenkins one-at-a-time hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of hash calculation
 */
uint32_t apollo_hash_jenkins_oaat(const uint8_t* data, size_t length, uint32_t init);

uint32_t apollo_hash_md5_xor(const uint8_t* data, uint32_t len);
uint64_t apollo_hash_sha1_xor64(const uint8_t* data, uint32_t len);

int apollo_hash_pbkdf2_sha1(const void *Pwd, size_t Plen, const void *Salt, size_t Slen, unsigned int count, uint8_t *DK, size_t dkLen);
int apollo_hash_pbkdf2_sha256(const void *Pwd, size_t Plen, const void *Salt, size_t Slen, unsigned int count, uint8_t *DK, size_t dkLen);

uint32_t apollo_hash_add(const uint8_t* data, uint32_t len);
uint32_t apollo_hash_wadd(const uint8_t* data, uint32_t len, int is_le);
uint32_t apollo_hash_dwadd(const uint8_t* data, uint32_t len, int is_le);
uint32_t apollo_hash_qwadd(const uint8_t* data, uint32_t len);
uint32_t apollo_hash_wsub(const uint8_t* data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* !_APOLLO_LIB_H_ */
