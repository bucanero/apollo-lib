#ifndef _APOLLO_LIB_H_
#define _APOLLO_LIB_H_

#include <stdint.h>
#include <stdlib.h>

#define APOLLO_LIB_VERSION         "2.0.2"

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
    uint64_t xor;
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

void free_patch_var_list(void);
size_t apply_sw_patch_code(uint8_t* data, size_t dsize, const code_entry_t* code);
size_t apply_bsd_patch_code(uint8_t** data, size_t dsize, const code_entry_t* code);
size_t apply_py_script_code(uint8_t** src_data, size_t dsize, const code_entry_t* code);
int apply_cheat_patch_code(const char* file_path, const code_entry_t* code, apollo_host_cb_t host_cb);
int load_patch_code_list(char* buffer, list_t* list_codes, apollo_get_files_cb_t get_files_cb, const char* save_path);


//---  Apollo encryption functions ---

// Diablo 3 save data encryption
void diablo_decrypt_data(uint8_t* data, uint32_t size);
void diablo_encrypt_data(uint8_t* data, uint32_t size);

// Blowfish ECB save data encryption
void blowfish_ecb_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);
void blowfish_ecb_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);

// Blowfish CBC save data encryption
void blowfish_cbc_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);
void blowfish_cbc_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);

// AES ECB save data encryption
void aes_ecb_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);
void aes_ecb_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);

// AES CBC save data encryption
void aes_cbc_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);
void aes_cbc_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);

// AES CTR save data encryption
void aes_ctr_xcrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len);

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

// Camellia ECB save data encryption
void camellia_ecb_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);
void camellia_ecb_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len);

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

// RGG Studio save data encryption
void rgg_xor_data(uint8_t* data, uint32_t size, const char* key, int key_len);

// Final Fantasy XIII (1/2/3) save data encryption
void ff13_decrypt_data(uint32_t game, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len);
void ff13_encrypt_data(uint32_t game, uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len);

// Borderlands 3 save data encryption
void borderlands3_Decrypt(uint8_t* buffer, int length, int mode);
void borderlands3_Encrypt(uint8_t* buffer, int length, int mode);

// Monster Hunter save data encryption
void monsterhunter_decrypt_data(uint8_t* buff, uint32_t size, int ver);
void monsterhunter_encrypt_data(uint8_t* buff, uint32_t size, int ver);

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

offzip_t* offzip_util(const uint8_t *data, size_t dlen, int offset, int wbits, int count);
void offzip_free(void);
int offzip_init(size_t dsz, int wbits);
int offzip_search(const uint8_t *data);
int offzip_verify(const uint8_t *data, uint32_t *offset, uint32_t *inlen, uint32_t *outlen);
int packzip_util(offzip_t *input, uint32_t offset, uint8_t** output, size_t* outsize);


//---  Apollo checksum functions ---

/* hash calculation for MGS: Peace Walker */
uint32_t mgspw_Checksum(const uint8_t* data, int size);

/* hash calculation for Final Fantasy XIII */
uint32_t ff13_checksum(const uint8_t* bytes, uint32_t len);

/* checksum update for Dead Rising */
int deadrising_checksum(uint8_t* data, uint32_t size);

/* checksum calculation for DBZ Xenoverse 2 */
uint64_t dbzxv2_checksum(const uint8_t* data, uint32_t size);

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
 * This function makes a djb2 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t djb2_hash(const uint8_t* data, uint32_t len);

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
 * This function makes Jenkins Lookup3 little2 hash calculation on Length data bytes
 *
 * RETURN VALUES: 32 bit results of CRC calculation
 */
void lookup3_hashlittle2(const uint8_t *k, size_t length, uint32_t *pc, uint32_t *pb);

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

/**
 * This function makes Murmur3 32 hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of CRC calculation
 */
uint32_t murmur3_32(const uint8_t *data, size_t len, uint32_t h);

/**
 * This function makes Jenkins hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of hash calculation
 */
uint32_t jhash(const uint8_t *data, uint32_t length, uint32_t initval);

/**
 * This function makes Jenkins one-at-a-time hash calculation on Length data bytes
 *
 * RETURN VALUE: 32 bit result of hash calculation
 */
uint32_t jenkins_oaat_hash(const uint8_t* data, size_t length, uint32_t init);

uint32_t md5_xor_hash(const uint8_t* data, uint32_t len);
uint64_t sha1_xor64_hash(const uint8_t* data, uint32_t len);

int pbkdf2_sha1(const void *Pwd, size_t Plen, const void *Salt, size_t Slen, unsigned int count, uint8_t *DK, size_t dkLen);
int pbkdf2_sha256(const void *Pwd, size_t Plen, const void *Salt, size_t Slen, unsigned int count, uint8_t *DK, size_t dkLen);

uint32_t add_hash(const uint8_t* data, uint32_t len);
uint32_t wadd_hash(const uint8_t* data, uint32_t len, int is_le);
uint32_t dwadd_hash(const uint8_t* data, uint32_t len, int is_le);
uint32_t qwadd_hash(const uint8_t* data, uint32_t len);
uint32_t wsub_hash(const uint8_t* data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* !_APOLLO_LIB_H_ */
