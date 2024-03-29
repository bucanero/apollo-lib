
// Naughty Dog
#define SECRET_KEY      "(SH[@2>r62%5+QKpy|g6"
#define SHA1_HMAC_KEY   "xM;6X%/p^L/:}-5QoA+K8:F*M!~sb(WK<E%6sW_un0a[7Gm6,()kHoXY+yI/s;Ba"

// DmC
#define DMC_BF_KEY      "8jdf*jsd@dfd8*P:QcNt"

// NFS Rivals
#define NFS_BF_KEY      "\x23\x91\xF2\x01\xB3\x6C\x85\xE8\x1B\x12\x72\xD6\x90\xFF\xA5\x45"

// NFS Undercover
static const char NFS_XOR_KEY[] = {0x21, 0xF3, 0xC6, 0xD2, 0x08, 0x63, 0xAA, 0xAC, 0x38, 0xE2, 0x20, 0x62, 0x0D, 0x0D, 0x4D, 0x52};

// Metal Gear Solid 2/3 HD
#define MGS2_KEY        "2MetalOfSolidSonsLibertyGear"
#define MGS3_KEY        "3MetalSolidSnakeEaterGear"
static const char MGS2_ALPHABET[] = "ghijklmn01234567opqrstuvEFGHIJKL89abcdefUVWXYZ_.wxyzABCDMNOPQRST";
static const char MGS3_ALPHABET[] = "ghijklmn01234567opqrstuvEFGHIJKL89abcdefUVWXYZ+-wxyzABCDMNOPQRST";

// Silent Hill 3
#define SH3_KEY1        0x5b6c3a2aL
#define SH3_KEY2        0x100000000L

// Diablo 3
#define DIABLO3_KEY1    0x305f92d8
#define DIABLO3_KEY2    0x2ec9a01b

// Dynasty Warriors 8 Xtreme Legends
#define DW8XL_KEY1      0x13100200
#define DW8XL_KEY2      0x41c64e6d

// Final Fantasy XIII
#define FFXIII_KEY      0x1317fb09b9b42080L
#define FFXIII_2_KEY    0x9B1F01011A6438B0L
#define FFXIII_3_KEY    0x36545e6ceb9a705fL
#define FFXIII_CONST    0xA1652347

#define DES_BLOCK_SIZE  8
#define AES_BLOCK_SIZE  16
#define XOR_BLOCK_SIZE  16
#define CAMELLIA_BLOCK_SIZE 16

// Monster Hunter PSP
#define MH_KEY_DEF0        0xdfa3
#define MH_KEY_DEF1        0x215f
#define MH_KEY_MOD0        0xffef
#define MH_KEY_MOD1        0xff8f

/* \MHFU Save Tools\data\MHFUdic_de.bin (8/10/2014 7:10:44 PM)
   StartOffset(h): 00000000, EndOffset(h): 000000FF, Length(h): 00000100 */
static const uint8_t MH2_DEC_TABLE[256] = {
    0x56, 0x7C, 0x9A, 0x8A, 0x1F, 0x38, 0x10, 0x3F, 0x65, 0xEA, 0x40, 0xF8,
    0x5C, 0xFC, 0xA2, 0x76, 0xC7, 0x01, 0x8B, 0xEB, 0xAB, 0xCD, 0xD7, 0x14,
    0x98, 0xBF, 0xA6, 0x52, 0xE9, 0x85, 0xF6, 0x48, 0x89, 0x9C, 0x7E, 0x06,
    0x7A, 0xE6, 0x8F, 0x1A, 0xB0, 0x1D, 0x69, 0xF4, 0x3E, 0xAC, 0x4D, 0x04,
    0x55, 0x28, 0xC2, 0x23, 0x9B, 0xD6, 0x3D, 0xED, 0xC5, 0x32, 0x00, 0x6C,
    0x77, 0x47, 0xD3, 0xC8, 0xD0, 0x0B, 0x87, 0x7D, 0xFB, 0xCC, 0xC0, 0x1B,
    0x93, 0x24, 0x75, 0xEC, 0xF0, 0x37, 0x8D, 0xE5, 0x84, 0xDC, 0xF3, 0x4C,
    0x44, 0x7B, 0xD1, 0x0C, 0x49, 0x41, 0xBA, 0x1E, 0x09, 0x0D, 0x22, 0x11,
    0x68, 0xA0, 0x6B, 0xA5, 0xBD, 0x82, 0xB3, 0x83, 0x36, 0xD8, 0xB7, 0x08,
    0xC9, 0xB8, 0x03, 0x30, 0x81, 0xFE, 0x02, 0xFD, 0xE2, 0xB1, 0x2D, 0x3A,
    0x61, 0xA1, 0x05, 0x27, 0x70, 0x59, 0xE8, 0x78, 0x97, 0x2E, 0x15, 0xF9,
    0x2B, 0xDA, 0x19, 0x33, 0x18, 0x4E, 0x7F, 0x9D, 0xBB, 0xE1, 0x20, 0xDB,
    0x67, 0xB4, 0x39, 0xA3, 0x46, 0x8E, 0x86, 0xBE, 0x51, 0x3C, 0xC6, 0x74,
    0x50, 0xA7, 0xAF, 0xC4, 0x80, 0x66, 0xDD, 0xA4, 0xAE, 0x26, 0x4A, 0x6D,
    0x0F, 0xB2, 0x4B, 0x35, 0x5E, 0x2F, 0x21, 0x63, 0x64, 0xE4, 0x0E, 0xA8,
    0xCE, 0x2A, 0xE0, 0x29, 0x91, 0x96, 0x73, 0x16, 0x43, 0xB5, 0x62, 0x5A,
    0x34, 0x54, 0xF2, 0x95, 0x5D, 0xDE, 0x79, 0x1C, 0xD5, 0x12, 0x0A, 0xFA,
    0xE3, 0xA9, 0x88, 0xEE, 0x94, 0xCB, 0x17, 0xB6, 0x6A, 0x99, 0x53, 0x92,
    0x13, 0x6E, 0xEF, 0x25, 0xAD, 0xFF, 0xCF, 0x9E, 0x31, 0x45, 0xCA, 0x8C,
    0x90, 0xD2, 0xAA, 0xBC, 0x5B, 0x9F, 0x60, 0x4F, 0x72, 0x6F, 0xDF, 0x58,
    0x07, 0xF5, 0xC3, 0x5F, 0x57, 0x71, 0x2C, 0xF1, 0xF7, 0xD4, 0x42, 0xB9,
    0xD9, 0xE7, 0xC1, 0x3B
};

static const uint8_t MH3_DEC_TABLE[256] = {
    0xa6, 0x21, 0x22, 0xf9, 0x3d, 0xfb, 0xf5, 0x06, 0x87, 0x54, 0x76, 0x75, 0xed, 0x16, 0x33, 0x2e, 
    0x5a, 0x17, 0x50, 0x1c, 0xde, 0xaf, 0x73, 0x39, 0xb5, 0x28, 0xd9, 0xf1, 0xcd, 0x98, 0x5f, 0x2d, 
    0x78, 0x62, 0x29, 0xc9, 0xfd, 0xea, 0x32, 0xcc, 0x31, 0x70, 0x34, 0x61, 0xae, 0x4d, 0xfe, 0xc2, 
    0x45, 0x24, 0xf8, 0xfc, 0xd7, 0x2f, 0xd8, 0x26, 0x59, 0xa0, 0xbd, 0xa5, 0x01, 0x18, 0xa1, 0x95, 
    0xee, 0x4b, 0x1a, 0x7a, 0x5b, 0xdb, 0xf0, 0x27, 0xe6, 0xf4, 0xb2, 0xad, 0x4a, 0x14, 0x9a, 0x20, 
    0xb9, 0x36, 0x4f, 0x3b, 0xca, 0xe4, 0x41, 0x85, 0x3a, 0x46, 0x5c, 0xcb, 0x3c, 0xec, 0x63, 0x84, 
    0x66, 0xc4, 0xeb, 0x25, 0xaa, 0x7e, 0xc3, 0x47, 0x9b, 0x74, 0x8b, 0x5d, 0x23, 0x8f, 0x72, 0x81, 
    0x8c, 0x56, 0xc6, 0xd4, 0x40, 0x60, 0xa2, 0x6d, 0xe5, 0xb0, 0x15, 0x58, 0x52, 0x0f, 0x7d, 0x67, 
    0xef, 0xd2, 0x0b, 0xcf, 0x0d, 0xd6, 0x2b, 0x0a, 0x9f, 0x80, 0x5e, 0x3f, 0x71, 0x68, 0x05, 0x86, 
    0xbb, 0x38, 0xce, 0x7f, 0xf3, 0x83, 0x03, 0x48, 0xd3, 0xfa, 0x35, 0xdf, 0x44, 0x7c, 0x82, 0x93, 
    0x53, 0xa3, 0x09, 0x6a, 0x77, 0x13, 0x55, 0xdd, 0xbe, 0x00, 0x9c, 0xb4, 0xe3, 0x9e, 0xbc, 0x96, 
    0xa7, 0xe2, 0x42, 0x4e, 0x37, 0x0e, 0xf6, 0x9d, 0x1b, 0x2a, 0x79, 0xbf, 0xdc, 0xa9, 0x88, 0x4c, 
    0xd0, 0xab, 0xc5, 0x69, 0xd1, 0xe1, 0xac, 0xe9, 0xc1, 0xb6, 0x7b, 0x57, 0x90, 0x07, 0x30, 0x92, 
    0xb7, 0x1e, 0xda, 0x49, 0x0c, 0xb8, 0xa4, 0xff, 0xb3, 0xf2, 0x64, 0x8a, 0xc7, 0x02, 0x6c, 0x08, 
    0x6f, 0xf7, 0x89, 0xc0, 0x10, 0xa8, 0x8d, 0xba, 0x91, 0x43, 0x6b, 0x3e, 0xd5, 0x65, 0xe0, 0x6e, 
    0x12, 0x2c, 0x94, 0x99, 0xe8, 0x1f, 0xc8, 0x11, 0x8e, 0xb1, 0xe7, 0x51, 0x1d, 0x97, 0x04, 0x19, 
};

// Metal Gear Solid Peace Walker
static const uint32_t MGS_PW_TABLE[256] = { 
	0x50b85761, 0x27bf67f7, 0xbeb6364d, 0xc9b106db, 0x57d59378, 0x20d2a3ee, 0xb9dbf254, 0xcedcc2c2, 
	0x5e63df53, 0x2964efc5, 0xb06dbe7f, 0xc76a8ee9, 0x590e1b4a, 0x2e092bdc, 0xb7007a66, 0xc0074af0, 
	0x4d0f4705, 0x3a087793, 0xa3012629, 0xd40616bf, 0x4a62831c, 0x3d65b38a, 0xa46ce230, 0xd36bd2a6, 
	0x43d4cf37, 0x34d3ffa1, 0xaddaae1b, 0xdadd9e8d, 0x44b90b2e, 0x33be3bb8, 0xaab76a02, 0xddb05a94, 
	0x6bd677a9, 0x1cd1473f, 0x85d81685, 0xf2df2613, 0x6cbbb3b0, 0x1bbc8326, 0x82b5d29c, 0xf5b2e20a, 
	0x650dff9b, 0x120acf0d, 0x8b039eb7, 0xfc04ae21, 0x62603b82, 0x15670b14, 0x8c6e5aae, 0xfb696a38, 
	0x766167cd, 0x166575b, 0x986f06e1, 0xef683677, 0x710ca3d4, 0x60b9342, 0x9f02c2f8, 0xe805f26e, 
	0x78baefff, 0xfbddf69, 0x96b48ed3, 0xe1b3be45, 0x7fd72be6, 0x8d01b70, 0x91d94aca, 0xe6de7a5c, 
	0x266416f1, 0x51632667, 0xc86a77dd, 0xbf6d474b, 0x2109d2e8, 0x560ee27e, 0xcf07b3c4, 0xb8008352, 
	0x28bf9ec3, 0x5fb8ae55, 0xc6b1ffef, 0xb1b6cf79, 0x2fd25ada, 0x58d56a4c, 0xc1dc3bf6, 0xb6db0b60, 
	0x3bd30695, 0x4cd43603, 0xd5dd67b9, 0xa2da572f, 0x3cbec28c, 0x4bb9f21a, 0xd2b0a3a0, 0xa5b79336, 
	0x35088ea7, 0x420fbe31, 0xdb06ef8b, 0xac01df1d, 0x32654abe, 0x45627a28, 0xdc6b2b92, 0xab6c1b04, 
	0x1d0a3639, 0x6a0d06af, 0xf3045715, 0x84036783, 0x1a67f220, 0x6d60c2b6, 0xf469930c, 0x836ea39a, 
	0x13d1be0b, 0x64d68e9d, 0xfddfdf27, 0x8ad8efb1, 0x14bc7a12, 0x63bb4a84, 0xfab21b3e, 0x8db52ba8, 
	0xbd265d, 0x77ba16cb, 0xeeb34771, 0x99b477e7, 0x7d0e244, 0x70d7d2d2, 0xe9de8368, 0x9ed9b3fe, 
	0xe66ae6f, 0x79619ef9, 0xe068cf43, 0x976fffd5, 0x90b6a76, 0x7e0c5ae0, 0xe7050b5a, 0x90023bcc, 
	0xbd00d441, 0xca07e4d7, 0x530eb56d, 0x240985fb, 0xba6d1058, 0xcd6a20ce, 0x54637174, 0x236441e2, 
	0xb3db5c73, 0xc4dc6ce5, 0x5dd53d5f, 0x2ad20dc9, 0xb4b6986a, 0xc3b1a8fc, 0x5ab8f946, 0x2dbfc9d0, 
	0xa0b7c425, 0xd7b0f4b3, 0x4eb9a509, 0x39be959f, 0xa7da003c, 0xd0dd30aa, 0x49d46110, 0x3ed35186, 
	0xae6c4c17, 0xd96b7c81, 0x40622d3b, 0x37651dad, 0xa901880e, 0xde06b898, 0x470fe922, 0x3008d9b4, 
	0x866ef489, 0xf169c41f, 0x686095a5, 0x1f67a533, 0x81033090, 0xf6040006, 0x6f0d51bc, 0x180a612a, 
	0x88b57cbb, 0xffb24c2d, 0x66bb1d97, 0x11bc2d01, 0x8fd8b8a2, 0xf8df8834, 0x61d6d98e, 0x16d1e918, 
	0x9bd9e4ed, 0xecded47b, 0x75d785c1, 0x2d0b557, 0x9cb420f4, 0xebb31062, 0x72ba41d8, 0x5bd714e, 
	0x95026cdf, 0xe2055c49, 0x7b0c0df3, 0xc0b3d65, 0x926fa8c6, 0xe5689850, 0x7c61c9ea, 0xb66f97c, 
	0xcbdc95d1, 0xbcdba547, 0x25d2f4fd, 0x52d5c46b, 0xccb151c8, 0xbbb6615e, 0x22bf30e4, 0x55b80072, 
	0xc5071de3, 0xb2002d75, 0x2b097ccf, 0x5c0e4c59, 0xc26ad9fa, 0xb56de96c, 0x2c64b8d6, 0x5b638840, 
	0xd66b85b5, 0xa16cb523, 0x3865e499, 0x4f62d40f, 0xd10641ac, 0xa601713a, 0x3f082080, 0x480f1016, 
	0xd8b00d87, 0xafb73d11, 0x36be6cab, 0x41b95c3d, 0xdfddc99e, 0xa8daf908, 0x31d3a8b2, 0x46d49824, 
	0xf0b2b519, 0x87b5858f, 0x1ebcd435, 0x69bbe4a3, 0xf7df7100, 0x80d84196, 0x19d1102c, 0x6ed620ba, 
	0xfe693d2b, 0x896e0dbd, 0x10675c07, 0x67606c91, 0xf904f932, 0x8e03c9a4, 0x170a981e, 0x600da888, 
	0xed05a57d, 0x9a0295eb, 0x30bc451, 0x740cf4c7, 0xea686164, 0x9d6f51f2, 0x4660048, 0x736130de, 
	0xe3de2d4f, 0x94d91dd9, 0xdd04c63, 0x7ad77cf5, 0xe4b3e956, 0x93b4d9c0, 0xabd887a, 0x7dbab8ec };

// Borderlands 3
// https://github.com/HackerSmacker/CSave/
static const char BL3_PROFILE_PREFIX_PS4[] = {
	0xad, 0x1e, 0x60, 0x4e, 0x42, 0x9e, 0xa9, 0x33, 0xb2, 0xf5, 0x01, 0xe1, 0x02, 0x4d, 0x08, 0x75,
	0xb1, 0xad, 0x1a, 0x3d, 0xa1, 0x03, 0x6b, 0x1a, 0x17, 0xe6, 0xec, 0x0f, 0x60, 0x8d, 0xb4, 0xf9};

static const char BL3_PROFILE_XOR_PS4[] = {
	0xba, 0x0e, 0x86, 0x1d, 0x58, 0xe1, 0x92, 0x21, 0x30, 0xd6, 0xcb, 0xf0, 0xd0, 0x82, 0xd5, 0x58,
	0x36, 0x12, 0xe1, 0xf6, 0x39, 0x44, 0x88, 0xea, 0x4e, 0xfb, 0x04, 0x74, 0x07, 0x95, 0x3a, 0xa2};

static const char BL3_DATA_PREFIX_PS4[] = {
	0xd1, 0x7b, 0xbf, 0x75, 0x4c, 0xc1, 0x80, 0x30, 0x37, 0x92, 0xbd, 0xd0, 0x18, 0x3e, 0x4a, 0x5f,
	0x43, 0xa2, 0x46, 0xa0, 0xed, 0xdb, 0x2d, 0x9f, 0x56, 0x5f, 0x8b, 0x3d, 0x6e, 0x73, 0xe6, 0xb8};

static const char BL3_DATA_XOR_PS4[] = {
	0xfb, 0xfd, 0xfd, 0x51, 0x3a, 0x5c, 0xdb, 0x20, 0xbb, 0x5e, 0xc7, 0xaf, 0x66, 0x6f, 0xb6, 0x9a,
	0x9a, 0x52, 0x67, 0x0f, 0x19, 0x5d, 0xd3, 0x84, 0x15, 0x19, 0xc9, 0x4a, 0x79, 0x67, 0xda, 0x6d};

// Patapon 3
#define PATAPON3_CAMELLIA_KEY    "SVsyE56pniSRS9dIPTiE8ApDaUnN0AEa"

/*
// MGSV_TPP_PS3KEY		0x1FBAB234
// MGSV_TPP_PS4KEY		0x4131F8BE

// GTA 5
uint8_t GTAV_PS3_KEY[32] = {
		0x16, 0x85, 0xFF, 0xA3, 0x8D, 0x01, 0x0F, 0x0D, 0xFE, 0x66, 0x1C, 0xF9, 0xB5, 0x57, 0x2C, 0x50,
		0x0D, 0x80, 0x26, 0x48, 0xDB, 0x37, 0xB9, 0xED, 0x0F, 0x48, 0xC5, 0x73, 0x42, 0xC0, 0x22, 0xF5};

uint8_t SHA1_HMAC_KEY[64] = {
	0x78, 0x4D, 0x3B, 0x36, 0x58, 0x25, 0x2F, 0x70, 0x5E, 0x4C, 0x2F, 0x3A, 0x7D, 0x2D, 0x35, 0x51, 
	0x6F, 0x41, 0x2B, 0x4B, 0x38, 0x3A, 0x46, 0x2A, 0x4D, 0x21, 0x7E, 0x73, 0x62, 0x28, 0x57, 0x4B, 
	0x3C, 0x45, 0x25, 0x36, 0x73, 0x57, 0x5F, 0x75, 0x6E, 0x30, 0x61, 0x5B, 0x37, 0x47, 0x6D, 0x36, 
	0x2C, 0x28, 0x29, 0x6B, 0x48, 0x6F, 0x58, 0x59, 0x2B, 0x79, 0x49, 0x2F, 0x73, 0x3B, 0x42, 0x61};
*/
