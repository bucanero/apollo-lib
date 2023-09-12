#ifndef _APOLLO_LIB_H_
#define _APOLLO_LIB_H_

#include <stdint.h>
#include <stdlib.h>

#define APOLLO_LIB_VERSION         "0.5.0"

#define APOLLO_CODE_GAMEGENIE      1
#define APOLLO_CODE_BSD            2

#define APOLLO_CODE_FLAG_PARENT    1
#define APOLLO_CODE_FLAG_CHILD     2
#define APOLLO_CODE_FLAG_REQUIRED  4
#define APOLLO_CODE_FLAG_ALERT     8
#define APOLLO_CODE_FLAG_EMPTY     16
#define APOLLO_CODE_FLAG_ENABLED   32

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct option_entry
{
    char * line;
    char * * name;
    char * * value;
    int id;
    int size;
    int sel;
} option_entry_t;

typedef struct code_entry
{
    uint8_t type;
    uint8_t flags;
    char * name;
    char * file;
    uint8_t activated;
    int options_count;
    char * codes;
    option_entry_t * options;
} code_entry_t;

typedef struct
{
    uint8_t width;
    uint64_t poly;
    uint64_t init;
    uint64_t xor;
    uint8_t refIn;
    uint8_t refOut;
} custom_crc_t;

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
int write_buffer(const char *file_path, uint8_t *buf, size_t size);


//---  Apollo patch functions ---

void free_patch_var_list();
int apply_bsd_patch_code(const char* file_path, code_entry_t* code);
int apply_ggenie_patch_code(const char* file_path, code_entry_t* code);
int apply_cheat_patch_code(const char* file_path, const char* title_id, code_entry_t* code, const char* tmp_dir);
void load_patch_code_list(char* buffer, list_t* list_codes, option_entry_t* (*get_files_opt)(const char*, const char*), const char* save_path);


//---  Apollo encryption functions ---

// Diablo 3 save data encryption
void diablo_decrypt_data(uint8_t* data, uint32_t size);
void diablo_encrypt_data(uint8_t* data, uint32_t size);

// Blowfish ECB save data encryption
void blowfish_ecb_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);
void blowfish_ecb_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);

// AES ECB save data encryption
void aes_ecb_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);
void aes_ecb_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);

// DES ECB save data encryption
void des_ecb_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);
void des_ecb_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);

// 3-DES CBC save data encryption
void des3_cbc_decrypt(uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);
void des3_cbc_encrypt(uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);

// NFS Undercover save data encryption
void nfsu_decrypt_data(uint8_t* data, uint32_t size);
void nfsu_encrypt_data(uint8_t* data, uint32_t size);

// Silent Hill 3 save data encryption
void sh3_decrypt_data(uint8_t* data, uint32_t size);
void sh3_encrypt_data(uint8_t* data, uint32_t size);

// Dynasty Warriors 8 Xtreme Legends save data encryption
void dw8xl_encode_data(uint8_t* data, uint32_t len);

// Metal Gear Solid 2/3 HD save data encryption
void mgs_Decrypt(uint8_t* data, int size, const char* key, int keylen);
void mgs_Encrypt(uint8_t* data, int size, const char* key, int keylen);
void mgs_DecodeBase64(uint8_t* data, uint32_t size);
void mgs_EncodeBase64(uint8_t* data, uint32_t size);

// Metal Gear Solid Peace Walker save data encryption
void mgspw_Encrypt(uint32_t* data, uint32_t len);
void mgspw_Decrypt(uint32_t* data, uint32_t len);

// Metal Gear Solid 5 TPP save data encryption
void mgs5tpp_encode_data(uint32_t* data, uint32_t len, uint32_t key);

// Final Fantasy XIII (1/2/3) save data encryption
void ff13_decrypt_data(uint32_t game, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len);
void ff13_encrypt_data(uint32_t game, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len);

// Borderlands 3 save data encryption
void borderlands3_Decrypt(uint8_t* buffer, int length, int mode);
void borderlands3_Encrypt(uint8_t* buffer, int length, int mode);


//---  offZip/packZip functions ---

#define OFFZIP_WBITS_ZLIB		15
#define OFFZIP_WBITS_DEFLATE	-15

int offzip_util(const char *input, const char *output_dir, const char *basename, int wbits);
int packzip_util(const char *input, const char *output, uint32_t offset, int wbits);


//---  Apollo checksum functions ---

/* hash calculation for MGS: Peace Walker */
uint32_t mgspw_Checksum(const uint8_t* data, int size);

/* hash calculation for Final Fantasy XIII */
uint32_t ff13_checksum(const uint8_t* bytes, uint32_t len);

/* Rockstar custom CHKS calculation */
uint32_t rockstar_chks(const uint8_t* data, uint32_t len);

/**
 * This function makes a CRC16 calculation on Length data bytes
 *
 * RETURN VALUE: 16 bit result of CRC calculation
 */
uint16_t crc16_hash(const uint8_t* message, uint32_t nBytes, custom_crc_t* cfg);

/**
 * This function makes a CRC32 calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t crc32_hash(const uint8_t* message, uint32_t nBytes, custom_crc_t* cfg);

/**
 * This function makes a CRC64 calculation on Length data bytes
 *
 * RETURN VALUE: 64 bit result of CRC calculation
 */
uint64_t crc64_hash(const uint8_t *data, uint32_t len, custom_crc_t* cfg);

/**
 * This function makes a "MC02" Electronic Arts hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t MC02_hash(const uint8_t *data, uint32_t len);

/**
 * This function makes a SDBM hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t sdbm_hash(const uint8_t* data, uint32_t len, uint32_t init);

/**
 * This function makes a FNV-1 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
int fnv1_hash(const uint8_t* data, uint32_t len, int init);

/**
 * This function makes a Checksum32 calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
int Checksum32_hash(const uint8_t* data, uint32_t len);

/**
 * This function makes Adler16 hash calculation on Length data bytes
 *
 * RETURN VALUE: 16 bit result of CRC calculation
 */
uint16_t adler16(const uint8_t *data, size_t len);

/**
 * This function makes Final Fantasy X hash calculation on Length data bytes
 *
 * RETURN VALUE: 16 bit result of CRC calculation
 */
uint16_t ffx_hash(const uint8_t* data, uint32_t len);

/**
 * This function makes Kingdom Hearts 2.5 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t kh25_hash(const uint8_t* data, uint32_t len);

/**
 * This function makes Kingdom Hearts Chain of Memories hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t kh_com_hash(const uint8_t* data, uint32_t len);

/**
 * This function makes DuckTales hash calculation on Length data bytes
 *
 * RETURN VALUE: 64 bit result of CRC calculation
 */
uint64_t duckTales_hash(const uint8_t* data, uint32_t len);

/**
 * This function makes Samurai Warriors 4 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result array of CRC calculation
 */
int sw4_hash(const uint8_t* data, uint32_t size, uint32_t* crcs);

/**
 * This function makes MGS2 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
int mgs2_hash(const uint8_t* data, uint32_t len);

/**
 * This function makes Tears to Tiara 2 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t tiara2_hash(const uint8_t* data, uint32_t len);

/**
 * This function makes Castlevania LOS 1/2 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
int castlevania_hash(const uint8_t* Bytes, uint32_t length);

/**
 * This function makes Tales of Zestiria hash calculation on Length data bytes
 *
 * RETURN VALUE: 20 byte result array of SHA1 calculation
 */
void toz_hash(const uint8_t* data, uint32_t len, uint8_t* sha_hash);

/**
 * This function brute-force a CRC32 hash calculation to match newcrc on Length data bytes
 *
 * RETURN VALUE: 32 bit data result to update offset
 */
int force_crc32(const uint8_t *data, uint32_t length, uint32_t offset, uint32_t newcrc);

#ifdef __cplusplus
}
#endif

#endif /* !_APOLLO_LIB_H_ */
