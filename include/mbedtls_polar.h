 // Copyright (C) 2024 Damian Parrino. All rights reserved.
 //
 // This file is part of Apollo Library.
 //
 // Apollo Library is free software: you can redistribute it and/or modify
 // it under the terms of the GNU General Public License as published by
 // the Free Software Foundation, either version 3 of the License, or
 // (at your option) any later version.
 //
 // Apollo Library is distributed in the hope that it will be useful,
 // but WITHOUT ANY WARRANTY; without even implied warranty of
 // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 // GNU General Public License for more details.
 //
 // You should have received a copy of the GNU General Public License
 // along with Apollo Library.  If not, see <http://www.gnu.org/licenses/>.
 //**********************************************************************/

// map mbedTLS names to polarSSL names
#ifdef _USE_MBEDTLS

#define sha256         mbedtls_sha256
#define sha512         mbedtls_sha512
#define sha1           mbedtls_sha1
#define sha1_context   mbedtls_sha1_context
#define sha1_starts    mbedtls_sha1_starts
#define sha1_update    mbedtls_sha1_update
#define sha1_finish    mbedtls_sha1_finish

#define sha1_hmac(key, kl, in, il, out)      mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), key, kl, in, il, out)

#define md                  mbedtls_md
#define md5                 mbedtls_md5
#define md_info_from_type   mbedtls_md_info_from_type

#define POLARSSL_MD_MD4     MBEDTLS_MD_MD4
#define POLARSSL_MD_MD2     MBEDTLS_MD_MD2

#define BLOWFISH_BLOCKSIZE  MBEDTLS_BLOWFISH_BLOCKSIZE
#define BLOWFISH_DECRYPT    MBEDTLS_BLOWFISH_DECRYPT
#define BLOWFISH_ENCRYPT    MBEDTLS_BLOWFISH_ENCRYPT

#define blowfish_context    mbedtls_blowfish_context
#define blowfish_init       mbedtls_blowfish_init
#define blowfish_setkey     mbedtls_blowfish_setkey
#define blowfish_crypt_ecb  mbedtls_blowfish_crypt_ecb
#define blowfish_crypt_cbc  mbedtls_blowfish_crypt_cbc

#define CAMELLIA_DECRYPT    MBEDTLS_CAMELLIA_DECRYPT
#define CAMELLIA_ENCRYPT    MBEDTLS_CAMELLIA_ENCRYPT

#define camellia_context    mbedtls_camellia_context
#define camellia_init       mbedtls_camellia_init
#define camellia_setkey_dec mbedtls_camellia_setkey_dec
#define camellia_setkey_enc mbedtls_camellia_setkey_enc
#define camellia_crypt_ecb  mbedtls_camellia_crypt_ecb

#define AES_DECRYPT      MBEDTLS_AES_DECRYPT
#define AES_ENCRYPT      MBEDTLS_AES_ENCRYPT

#define aes_context      mbedtls_aes_context
#define aes_init         mbedtls_aes_init
#define aes_setkey_dec   mbedtls_aes_setkey_dec
#define aes_setkey_enc   mbedtls_aes_setkey_enc
#define aes_crypt_ecb    mbedtls_aes_crypt_ecb
#define aes_crypt_cbc    mbedtls_aes_crypt_cbc
#define aes_crypt_ctr    mbedtls_aes_crypt_ctr

#define DES_DECRYPT      MBEDTLS_DES_DECRYPT
#define DES_ENCRYPT      MBEDTLS_DES_ENCRYPT
#define DES_KEY_SIZE     MBEDTLS_DES_KEY_SIZE

#define des_context      mbedtls_des_context
#define des_init         mbedtls_des_init
#define des_setkey_dec   mbedtls_des_setkey_dec
#define des_setkey_enc   mbedtls_des_setkey_enc
#define des_crypt_ecb    mbedtls_des_crypt_ecb

#define des3_context     mbedtls_des3_context
#define des3_init        mbedtls_des3_init
#define des3_set3key_dec mbedtls_des3_set3key_dec
#define des3_set3key_enc mbedtls_des3_set3key_enc
#define des3_crypt_cbc   mbedtls_des3_crypt_cbc

#endif  // _USE_MBEDTLS
