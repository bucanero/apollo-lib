/*
*
*	Custom PS3 Save Decrypters - (c) 2020 by Bucanero - www.bucanero.com.ar
*
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <polarssl/aes.h>
#include <polarssl/des.h>
#include <polarssl/md5.h>
#include <polarssl/blowfish.h>
#include "keys.h"
#include "types.h"


void blowfish_ecb_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len)
{
	blowfish_context ctx;

	LOG("Decrypting Blowfish ECB data (%d bytes)", len);

	blowfish_init(&ctx);
	blowfish_setkey(&ctx, key, key_len * 8);
	len = len / BLOWFISH_BLOCKSIZE;

	while (len--)
	{
		blowfish_crypt_ecb(&ctx, BLOWFISH_DECRYPT, data, data);
		data += BLOWFISH_BLOCKSIZE;
	}

	return;
}

void blowfish_ecb_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len)
{
	blowfish_context ctx;

	LOG("Encrypting Blowfish ECB data (%d bytes)", len);

	blowfish_init(&ctx);
	blowfish_setkey(&ctx, key, key_len * 8);
	len = len / BLOWFISH_BLOCKSIZE;

	while (len--)
	{
		blowfish_crypt_ecb(&ctx, BLOWFISH_ENCRYPT, data, data);
		data += BLOWFISH_BLOCKSIZE;
	}

	return;
}

void diablo_decrypt_data(uint8_t* data, uint32_t size)
{
	uint32_t xor_key1 = DIABLO3_KEY1;
	uint32_t xor_key2 = DIABLO3_KEY2;
	uint32_t tmp;

	LOG("[*] Total Decrypted Size 0x%X (%d bytes)", size, size);

	for (uint32_t i = 0; i < size; i++)
	{
		data[i] ^= (xor_key1 & 0xFF);
		tmp = data[i] ^ xor_key1;
		xor_key1 = xor_key1 >> 8 | xor_key2 << 0x18;
		xor_key2 = xor_key2 >> 8 | tmp << 0x18;
	}

	LOG("[*] Decrypted File Successfully!");
	return;
}

void diablo_encrypt_data(uint8_t* data, uint32_t size)
{
	uint32_t xor_key1 = DIABLO3_KEY1;
	uint32_t xor_key2 = DIABLO3_KEY2;
	uint32_t tmp;

	LOG("[*] Total Encrypted Size 0x%X (%d bytes)", size, size);

	for (uint32_t i = 0; i < size; i++)
	{
		tmp = data[i] ^ xor_key1;
		data[i] ^= (xor_key1 & 0xFF);
		xor_key1 = xor_key1 >> 8 | xor_key2 << 0x18;
		xor_key2 = xor_key2 >> 8 | tmp << 0x18;
	}

	LOG("[*] Encrypted File Successfully!");
	return;
}

void aes_ecb_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len)
{
	aes_context ctx;

	LOG("Decrypting AES ECB data (%d bytes)", len);

	aes_init(&ctx);
	aes_setkey_dec(&ctx, key, key_len * 8);
	len = len / AES_BLOCK_SIZE;

	while (len--)
	{
		aes_crypt_ecb(&ctx, AES_DECRYPT, data, data);
		data += AES_BLOCK_SIZE;
	}

	return;
}

void aes_ecb_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len)
{
	aes_context ctx;

	LOG("Encrypting AES ECB data (%d bytes)", len);

	aes_init(&ctx);
	aes_setkey_enc(&ctx, key, key_len * 8);
	len = len / AES_BLOCK_SIZE;

	while (len--)
	{
		aes_crypt_ecb(&ctx, AES_ENCRYPT, data, data);
		data += AES_BLOCK_SIZE;
	}

	return;
}

void des_ecb_decrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len)
{
	des_context ctx;

	if (key_len != DES_KEY_SIZE)
		return;

	LOG("Decrypting DES ECB data (%d bytes)", len);

	des_init(&ctx);
	des_setkey_dec(&ctx, key);
	len = len / DES_BLOCK_SIZE;

	while (len--)
	{
		des_crypt_ecb(&ctx, data, data);
		data += DES_BLOCK_SIZE;
	}

	return;
}

void des_ecb_encrypt(uint8_t* data, uint32_t len, uint8_t* key, uint32_t key_len)
{
	des_context ctx;

	if (key_len != DES_KEY_SIZE)
		return;

	LOG("Encrypting DES ECB data (%d bytes)", len);

	des_init(&ctx);
	des_setkey_enc(&ctx, key);
	len = len / DES_BLOCK_SIZE;

	while (len--)
	{
		des_crypt_ecb(&ctx, data, data);
		data += DES_BLOCK_SIZE;
	}

	return;
}

void des3_cbc_decrypt(uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len)
{
	des3_context ctx;

	if (key_len != DES_KEY_SIZE*3 || iv_len != DES_KEY_SIZE)
		return;

	LOG("Decrypting 3-DES CBC data (%d bytes)", len);

	des3_init(&ctx);
	des3_set3key_dec(&ctx, key);
	des3_crypt_cbc(&ctx, DES_DECRYPT, len, iv, data, data);

	return;
}

void des3_cbc_encrypt(uint8_t* data, uint32_t len, const uint8_t* key, uint32_t key_len, uint8_t* iv, uint32_t iv_len)
{
	des3_context ctx;

	if (key_len != DES_KEY_SIZE*3 || iv_len != DES_KEY_SIZE)
		return;

	LOG("Encrypting 3-DES CBC data (%d bytes)", len);

	des3_init(&ctx);
	des3_set3key_enc(&ctx, key);
	des3_crypt_cbc(&ctx, DES_ENCRYPT, len, iv, data, data);

	return;
}

static void xor_block(const uint8_t* in, uint8_t* out)
{
	for (int i = 0; i < XOR_BLOCK_SIZE; i++)
		out[i] ^= in[i];
}

void nfsu_decrypt_data(uint8_t* data, uint32_t size)
{
	uint8_t xor_key[XOR_BLOCK_SIZE];
	uint8_t tmp[XOR_BLOCK_SIZE];

	LOG("[*] Total Decrypted Size Is 0x%X (%d bytes)", size, size);

	// init xor key
	memcpy(xor_key, NFS_XOR_KEY, XOR_BLOCK_SIZE);
	xor_block(data, xor_key);
	md5(xor_key, XOR_BLOCK_SIZE, xor_key);

	size /= XOR_BLOCK_SIZE;
	
	while (size--)
	{
		data += XOR_BLOCK_SIZE;

		md5(data, XOR_BLOCK_SIZE, tmp);
		xor_block(xor_key, data);
		memcpy(xor_key, tmp, XOR_BLOCK_SIZE);
	}

	return;
}

void nfsu_encrypt_data(uint8_t* data, uint32_t size)
{
	uint8_t xor_key[XOR_BLOCK_SIZE];

	LOG("[*] Total Encrypted Size Is 0x%X (%d bytes)", size, size);

	// init xor key
	memcpy(xor_key, NFS_XOR_KEY, XOR_BLOCK_SIZE);
	xor_block(data, xor_key);
	md5(xor_key, XOR_BLOCK_SIZE, xor_key);

	size /= XOR_BLOCK_SIZE;

	while (size--)
	{
		data += XOR_BLOCK_SIZE;

		xor_block(xor_key, data);
		md5(data, XOR_BLOCK_SIZE, xor_key);
	}

	return;
}

void sh3_decrypt_data(uint8_t* data, uint32_t size)
{
	uint32_t input, out;
	uint64_t key2 = SH3_KEY2;

	LOG("[*] Total Decrypted Size Is 0x%X (%d bytes)", size, size);

	size /= 4;

	while (size--)
	{
		input = *(uint32_t*) data;
		BE32(input);
		out = (uint32_t)((input ^ (uint64_t)(key2 - SH3_KEY1)) & 0xFFFFFFFF);
		BE32(out);
		memcpy(data, &out, sizeof(uint32_t));

		key2 = (uint64_t)(input << 5 | input >> 27) + (uint64_t)SH3_KEY2;
		data += 4;
	}

	return;
}

void sh3_encrypt_data(uint8_t* data, uint32_t size)
{
	uint32_t input, out;
	uint64_t key2 = SH3_KEY2;

	LOG("[*] Total Encrypted Size Is 0x%X (%d bytes)", size, size);

	size /= 4;

	while (size--)
	{
		input = *(uint32_t*) data;
		BE32(input);
		out = (uint32_t)((input ^ (uint64_t)(key2 - SH3_KEY1)) & 0xFFFFFFFF);

		key2 = (uint64_t)(out << 5 | out >> 27) + (uint64_t)SH3_KEY2;
		BE32(out);
		memcpy(data, &out, sizeof(uint32_t));
		data += 4;
	}

	return;
}

static void ff13_init_key(uint8_t* key_table, uint32_t ff_game, const uint64_t* kdata)
{
	uint32_t init[2];
	uint64_t ff_key = FFXIII_KEY;

	if (ff_game > 1)
	{
		ff_key = (kdata[0] ^ kdata[1]);
		BE64(ff_key);
		ff_key |= 1L;
		ff_key ^= (ff_game == 2) ? FFXIII_2_KEY : FFXIII_3_KEY;
	}

	ff_key = ((ff_key & 0xFF00000000000000) >> 16) | ((ff_key & 0x0000FF0000000000) << 16) | (ff_key & 0x00FF00FFFFFFFFFF);
	ff_key = ((ff_key & 0x00000000FF00FF00) >>  8) | ((ff_key & 0x0000000000FF00FF) <<  8) | (ff_key & 0xFFFFFFFF00000000);
	LE64(ff_key);

	memcpy(key_table, &ff_key, sizeof(uint64_t));

	key_table[0] += 0x45;
	
	for (int j = 1; j < 8; j++)
	{
		init[0] = key_table[j-1] + key_table[j];
		init[1] = ((key_table[j-1] << 2) & 0x3FC);
		key_table[j] = ((((init[0] + 0xD4) ^ init[1]) & 0xFF) ^ 0x45);
	}

	for (int j = 0; j < 31; j++)
	{
		ff_key = *(uint64_t*)(key_table + j*8);
		LE64(ff_key);
		ff_key = ff_key + (uint64_t)((ff_key << 2) & 0xFFFFFFFFFFFFFFFC);
		LE64(ff_key);

		memcpy(key_table + (j+1)*8, &ff_key, sizeof(uint64_t));
	}
}

uint32_t ff13_checksum(const uint8_t* bytes, uint32_t len)
{
	uint32_t ff_csum = 0;
	len /= 4;

	while (len--)
	{
		ff_csum += bytes[0];
		bytes += 4;
	}

	return (ff_csum);
}

void ff13_decrypt_data(uint32_t type, uint8_t* MemBlock, uint32_t size, const uint8_t* key, uint32_t key_len)
{
	uint8_t KeyBlocksArray[32][8];
	uint32_t *csum, ff_csum;

	if (type != 1 && key_len != 16)
		return;

	LOG("[*] Total Decrypted Size Is 0x%X (%d bytes)", size, size);

	memset(KeyBlocksArray, 0, sizeof(KeyBlocksArray));
	ff13_init_key(&KeyBlocksArray[0][0], type, (uint64_t*) key);

	///DECODING THE ENCODED INFORMATION NOW IN MemBlock
	uint32_t ByteCounter = 0, BlockCounter = 0, KeyBlockCtr = 0;
	uint32_t Gear1 = 0, Gear2 = 0;
	uint32_t TBlockA = 0, TBlockB = 0, KBlockA = 0, KBlockB = 0;
	uint8_t CarryFlag = 0;
	uint8_t IntermediateCarrier = 0, OldMemblockValue = 0;

	union u {
		uint64_t Cog64B;
		uint32_t Cog32BArray[2];
	} o;

	///OUTERMOST LOOP OF THE DECODER
	for (; ByteCounter < size; BlockCounter++, KeyBlockCtr++, ByteCounter += 8)
	{
		if(KeyBlockCtr > 31)
			KeyBlockCtr = 0;

		o.Cog64B = (uint64_t)ByteCounter << 0x14;
		LE64(o.Cog64B);
		LE32(o.Cog32BArray[0]);
		LE32(o.Cog32BArray[1]);
		Gear1 = o.Cog32BArray[0] | (ByteCounter << 0x0A) | ByteCounter; ///Will this work badly when Gear1 becomes higher than 7FFFFFFF?

		CarryFlag = (Gear1 > ~FFXIII_CONST) ? 1 : 0;

		Gear1 = Gear1 + FFXIII_CONST;
		Gear2 = (BlockCounter*2 | o.Cog32BArray[1]) + CarryFlag;

		///THE INNER LOOP OF THE DECODER
		for(int i = 0, BlockwiseByteCounter = 0; BlockwiseByteCounter < 8;)
		{
			if(i == 0 && BlockwiseByteCounter == 0)
			{
				OldMemblockValue = MemBlock[ByteCounter];
				MemBlock[ByteCounter] = 0x45 ^ BlockCounter ^ MemBlock[ByteCounter];
				i++;
			}
			else if(i == 0 && BlockwiseByteCounter < 8)
			{
				IntermediateCarrier = MemBlock[ByteCounter] ^ OldMemblockValue;
				OldMemblockValue = MemBlock[ByteCounter];
				MemBlock[ByteCounter] = IntermediateCarrier;
				i++;
			}
			else if(i < 9 && BlockwiseByteCounter < 8)
			{
				MemBlock[ByteCounter] = 0x78 + MemBlock[ByteCounter] - KeyBlocksArray[KeyBlockCtr][i-1];
				i++;
			}
			else if(i == 9)
			{
				i = 0;
				ByteCounter++;
				BlockwiseByteCounter++;
			}
		}
		///EXITING THE INNER LOOP OF THE DECOER

		///RESUMING THE OUTER LOOP
		ByteCounter -=8;

		TBlockA = *(uint32_t*) &MemBlock[ByteCounter];
		TBlockB = *(uint32_t*) &MemBlock[ByteCounter+4];
		LE32(TBlockA);
		LE32(TBlockB);

		KBlockA = *(uint32_t*) &KeyBlocksArray[KeyBlockCtr][0];
		KBlockB = *(uint32_t*) &KeyBlocksArray[KeyBlockCtr][4];
		LE32(KBlockA);
		LE32(KBlockB);

		CarryFlag = (TBlockA < KBlockA) ? 1 : 0;

		TBlockA = (KBlockA ^ Gear1 ^ (TBlockA - KBlockA));
		TBlockB = (KBlockB ^ Gear2 ^ (TBlockB - KBlockB - CarryFlag));
		LE32(TBlockA);
		LE32(TBlockB);

		memcpy(&MemBlock[ByteCounter],   &TBlockB, sizeof(uint32_t));
		memcpy(&MemBlock[ByteCounter+4], &TBlockA, sizeof(uint32_t));
	}
	///EXITING THE OUTER LOOP. FILE HAS NOW BEEN FULLY DECODED.

	ff_csum = ff13_checksum(MemBlock, ByteCounter - 8);
	LE32(ff_csum);
	csum = (uint32_t*)(MemBlock + ByteCounter - 4);

	if (*csum == ff_csum)
		LOG("[*] Decrypted File Successfully!");
	else
		LOG("[!] Decrypted data did not pass file integrity check. (Expected: %08X Got: %08X)", *csum, ff_csum);

	return;
}

void ff13_encrypt_data(uint32_t type, uint8_t* MemBlock, uint32_t size, const uint8_t* key, uint32_t key_len)
{
	uint8_t KeyBlocksArray[32][8];

	if (type != 1 && key_len != 16)
		return;

	LOG("[*] Total Encrypted Size Is 0x%X (%d bytes)", size, size);

	memset(KeyBlocksArray, 0, sizeof(KeyBlocksArray));
	ff13_init_key(&KeyBlocksArray[0][0], type, (uint64_t*) key);

	///ENCODE the file, now that all the changes have been made and the checksum has been updated.
	uint32_t ByteCounter = 0, BlockCounter = 0, KeyBlockCtr = 0;
	uint32_t Gear1 = 0, Gear2 = 0;
	uint32_t TBlockA = 0, TBlockB = 0, KBlockA = 0, KBlockB = 0;
	uint8_t CarryFlag = 0;
	uint8_t OldMemblockValue = 0;

	union u {
		uint64_t Cog64B;
		uint32_t Cog32BArray[2];
	} o;

	///OUTERMOST LOOP OF THE ENCODER
	for (; ByteCounter < size; BlockCounter++, KeyBlockCtr++)
	{
		if(KeyBlockCtr > 31)
			KeyBlockCtr = 0;

		o.Cog64B = (uint64_t)ByteCounter << 0x14;
		LE64(o.Cog64B);
		LE32(o.Cog32BArray[0]);
		LE32(o.Cog32BArray[1]);
		Gear1 = o.Cog32BArray[0] | (ByteCounter << 0x0A) | ByteCounter; ///Will this work badly when Gear1 becomes higher than 7FFFFFFF?

		CarryFlag = (Gear1 > ~FFXIII_CONST) ? 1 : 0;

		Gear1 = Gear1 + FFXIII_CONST;
		Gear2 = (BlockCounter*2 | o.Cog32BArray[1]) + CarryFlag;

		KBlockA = *(uint32_t*) &KeyBlocksArray[KeyBlockCtr][0];
		KBlockB = *(uint32_t*) &KeyBlocksArray[KeyBlockCtr][4];
		LE32(KBlockA);
		LE32(KBlockB);

		TBlockB = *(uint32_t*) &MemBlock[ByteCounter];
		TBlockA = *(uint32_t*) &MemBlock[ByteCounter+4];
		LE32(TBlockA);
		LE32(TBlockB);

		TBlockB ^= (KBlockB ^ Gear2);
		TBlockA ^= (KBlockA ^ Gear1);

		///Reverse of TBlockA < KBlockA from the Decoder.
		CarryFlag = (TBlockA > ~KBlockA) ? 1 : 0;

		TBlockB += (KBlockB + CarryFlag);      ///Reversed from subtraction to addition.
		TBlockA += KBlockA;                    ///Reversed from subtraction to addition.
		LE32(TBlockA);
		LE32(TBlockB);

		memcpy(&MemBlock[ByteCounter],   &TBlockA, sizeof(uint32_t));
		memcpy(&MemBlock[ByteCounter+4], &TBlockB, sizeof(uint32_t));

		///INNER LOOP OF ENCODER
		for(int i = 8, BlockwiseByteCounter = 0; BlockwiseByteCounter < 8;)
		{
			if(i != 0 && BlockwiseByteCounter < 8)
			{
				MemBlock[ByteCounter] = MemBlock[ByteCounter] + KeyBlocksArray[KeyBlockCtr][i-1] - 0x78;
				i--;
			}
			else if(i == 0 && BlockwiseByteCounter==0)
			{
				MemBlock[ByteCounter] = 0x45 ^ BlockCounter ^ MemBlock[ByteCounter];
				OldMemblockValue = MemBlock[ByteCounter];
				BlockwiseByteCounter++;
				ByteCounter++;
				i = 8;
			}
			else if(i == 0 && BlockwiseByteCounter < 8)
			{
				MemBlock[ByteCounter] = MemBlock[ByteCounter] ^ OldMemblockValue;
				OldMemblockValue = MemBlock[ByteCounter];
				i = 8;
				BlockwiseByteCounter++;
				ByteCounter++;
			}
		}
		///EXITING INNER LOOP OF ENCODER
	}
	///EXITING OUTER LOOP
	///ENCODING FINISHED

	LOG("[*] Encrypted File Successfully!");
	return;
}

void mgs_Decrypt(uint8_t* data, int size, const char* key, int keylen)
{
    LOG("[*] Total Decrypted Size Is 0x%X (%d bytes)", size, size);

    for (int i = size - 1; i > 0; i--)
        data[i] -= (key[i % keylen] + data[i-1]);
    
    data[0] -= key[0];
    return;
}

void mgs_Encrypt(uint8_t* data, int size, const char* key, int keylen)
{
    LOG("[*] Total Encrypted Size Is 0x%X (%d bytes)", size, size);

    data[0] += key[0];

    for (int i = 1; i < size; i++)
        data[i] += (key[i % keylen] + data[i-1]);

    return;
}

void mgs_EncodeBase64(uint8_t* data, uint32_t size)
{
    int i, j, k;
    const char *chars;
    uint8_t tmpArray[28];

    LOG("[*] Total Encoded Size Is 0x%X (%d bytes)", size, size);

    if (size != 32)
        return;

    uint8_t type = data[31];
    chars = (type == 2) ? MGS2_ALPHABET : MGS3_ALPHABET;

    data[31] = 0;
    data[20] = 0;
    for (j = 0; j < 20; j++)
        data[20] ^= data[j];

    for (i = 0, j = 0, k = 0; j < 28; j++)
    {
        if (k == 0 || k == 1)
            tmpArray[j] = (uint8_t)(data[i] >> (2-k));

        else if (k == 2)
            tmpArray[j] = (uint8_t)(data[i] & 63);

        else if (k <= 7)
            tmpArray[j] = (uint8_t)(data[i + 1] >> (10-k)) | ((data[i] & ((1 << (8-k)) - 1)) << (k-2));

        k += 6;
        if (k >= 8)
        {
            k -= 8;
            i++;
        }
    }

    data[0] = (type == 2) ? 68 : 0x5F;
    for (i = 0, j = 0; i < 28; i++, j += (i % 4) ? 0 : 28)
        data[i + 1] = (uint8_t)chars[(tmpArray[i] + j +  7 * (i % 4)) & 63];

    return;
}

void mgs_DecodeBase64(uint8_t* data, uint32_t size)
{
    int i, j, k, m;
    const char *chars;
    uint8_t b64_table[0x100];
    uint8_t tmpArray[0x20];

    LOG("[*] Total Decoded Size Is 0x%X (%d bytes)", size, size);

    if (size != 32)
        return;

    uint8_t type = (data[0] == 0x5F) + 2;
    memset(tmpArray, 0, sizeof(tmpArray));
    memset(b64_table, 0xFF, sizeof(b64_table));
    chars = (type == 2) ? MGS2_ALPHABET : MGS3_ALPHABET;

    for (k = 0; k < 64; k++)
        b64_table[(uint8_t)chars[k]] = (uint8_t)k;

    for (j = 0, m = 0; j < 196; j += 7, m++)
    {
        k = b64_table[data[1 + m]];

        if (k == 0xff)
            return;

        data[m] = (uint8_t)((k - j) & 63);
    }

    for (j = 0, k = 0, m = 0; m < 21; m++)
    {
        if (j <= 5)
            tmpArray[m] = (uint8_t)(data[k] & 63 >> (j & 31)) << ((j + 2) & 31);

        i = ~j + 7;
        j = 0;
        k++;

        if (i < 2)
        {
            tmpArray[m] |= (uint8_t)(data[k] << ((~i + 3) & 31));
            i += 6;
            k++;
        }
        i -= 2;

        if (i == 0)
        {
            tmpArray[m] |= (uint8_t)(data[k]);
            k++;
            j = 0;
        }
        else if (i <= 5)
        {
            j = 6 - i;
            tmpArray[m] |= (uint8_t)(data[k] >> (i & 31));
        }
    }
    memcpy(data, tmpArray, sizeof(tmpArray));
    data[31] = type;

    return;
}

uint32_t mgspw_Checksum(const uint8_t* data, int size)
{
    uint32_t csum = -1;

    while (size--)
        csum = MGS_PW_TABLE[(uint8_t)(*data++ ^ csum)] ^ csum >> 8 ^ MGS_PW_TABLE[0];

    return ~csum;
}

static void mgspw_DeEncryptBlock(uint32_t* data, int size, uint32_t* pwSalts)
{
	for (int i = 0; i < size; i++)
	{
		BE32(data[i]);
		data[i] ^= pwSalts[0];
		BE32(data[i]);
		pwSalts[0] = pwSalts[0] * 0x2e90edd + pwSalts[1];
	}
}

static void mgspw_SetSalts(uint32_t* pwSalts, const uint32_t *data)
{
    uint32_t offset, d0 = data[0], d1 = data[1];

    BE32(d0);
    BE32(d1);
    offset = (d1 | 0xAD47DE8F) ^ d0;
    d0 = data[offset + 2];
    d1 = data[offset + 3];
    BE32(d0);
    BE32(d1);

    pwSalts[0] = (d0 ^ 0x1327de73) ^ (d1 ^ 0x2d71d26c);
    d1 = data[offset + 7];
    BE32(d1);
    pwSalts[1] = pwSalts[0] * (d1 ^ 0xBC4DEFA2);
    pwSalts[0] = (pwSalts[0] ^ 0x6576) << 16 | pwSalts[0];
}

static void mgspw_SwapBlock(uint32_t* data, int len)
{
    for (int i = 0; i < len; i++)
    	data[i] = ES32(data[i]);
}

void mgspw_Decrypt(uint32_t* data, uint32_t size)
{
    uint32_t salts[2] = {0, 0};

    LOG("[*] Total Decrypted Size Is 0x%X (%d bytes)", size, size);

    if (size < 0x35998)
        return;

    mgspw_SwapBlock(data, 0xd676);
    mgspw_SetSalts(salts, data);
    mgspw_DeEncryptBlock(data + 16, 0xD666, salts);

    mgspw_SetSalts(salts, data + 0xD676);
    mgspw_DeEncryptBlock(data + 0xD686, 0x3C34, salts);
    mgspw_SwapBlock(data + 17, 0xd665);

    salts[0] = mgspw_Checksum((uint8_t*)data + 68, 0x1af24);
    BE32(salts[0]);
    if (salts[0] != data[14])
        LOG("[!] Checksum error (%x)", 68);

    salts[0] = mgspw_Checksum((uint8_t*)data + 0x1af68, 0x1c00);
    BE32(salts[0]);
    if (salts[0] != data[15])
        LOG("[!] Checksum error (%x)", 0x1af68);

    salts[0] = mgspw_Checksum((uint8_t*)data + 0x1cb68, 0x18e68);
    BE32(salts[0]);
    if (salts[0] != data[12])
        LOG("[!] Checksum error (%x)", 0x1cb68);

    salts[0] = mgspw_Checksum((uint8_t*)data + 0x35a18, 0xf0d0);
    BE32(salts[0]);
    if (salts[0] != data[0xD683])
        LOG("[!] Checksum error (%x)", 0x35a18);

    LOG("[*] Decrypted File Successfully!");
    return;
}

void mgspw_Encrypt(uint32_t* data, uint32_t size)
{
    uint32_t salts[2] = {0, 0};

    LOG("[*] Total Encrypted Size Is 0x%X (%d bytes)", size, size);

    if (size < 0x35998)
        return;

    mgspw_SwapBlock(data + 17, 0xd665);
    mgspw_SetSalts(salts, data + 0xD676);
    mgspw_DeEncryptBlock(data + 0xD686, 0x3C34, salts);

    mgspw_SetSalts(salts, data);
    mgspw_DeEncryptBlock(data + 16, 0xD666, salts);
    mgspw_SwapBlock(data, 0xD676);

    LOG("[*] Encrypted File Successfully!");
    return;
}

void dw8xl_encode_data(uint8_t* data, uint32_t size)
{
	uint32_t xor_key = DW8XL_KEY1;

	LOG("[*] Total Encoded Size Is 0x%X (%d bytes)", size, size);

    for(uint32_t i = 0; i <= size; i++)
    {
        xor_key = (xor_key * DW8XL_KEY2) + 0x3039;
        data[i] ^= ((xor_key >> 16) & 0xff);
    }

	LOG("[*] Encoded File Successfully!");
	return;
}

void borderlands3_Decrypt(uint8_t* buffer, int length, int mode)
{
	char b;
	const char* XorMagic = mode ? BL3_DATA_XOR_PS4 : BL3_PROFILE_XOR_PS4;
	const char* PrefixMagic = mode ? BL3_DATA_PREFIX_PS4 : BL3_PROFILE_PREFIX_PS4;

	LOG("[*] Total Decrypted Size Is 0x%X (%d bytes)", length, length);

	for(int i = length - 1; i >= 0; i--)
	{
		b = (i < 32) ? PrefixMagic[i] : buffer[i - 32];
		b ^= XorMagic[i % 32];
		buffer[i] ^= b;
	}

	LOG("[*] Decrypted File Successfully!");
	return;
}

void borderlands3_Encrypt(uint8_t* buffer, int length, int mode)
{
	char b;
	const char* XorMagic = mode ? BL3_DATA_XOR_PS4 : BL3_PROFILE_XOR_PS4;
	const char* PrefixMagic = mode ? BL3_DATA_PREFIX_PS4 : BL3_PROFILE_PREFIX_PS4;

	LOG("[*] Total Encrypted Size Is 0x%X (%d bytes)", length, length);

	for(int i = 0; i < length; i++)
	{
		b = (i < 32) ? PrefixMagic[i] : buffer[i - 32];
		b ^= XorMagic[i % 32];
		buffer[i] ^= b;
	}

	LOG("[*] Encrypted File Successfully!");
	return;
}

void mgs5tpp_encode_data(uint32_t* data, uint32_t len, uint32_t key)
{
	LOG("[*] Total Encoded Size: 0x%X (%d bytes)", len, len);

	len /= 4;
	for (uint32_t i = 0; i < len; i++)
	{
		key ^= (key << 13);
		key ^= (key >> 7);
		key ^= (key << 5);

		MEM32(data[i]);
		data[i] ^= key;
		MEM32(data[i]);
	}

	LOG("[*] Encoded File Successfully!");
	return;
}

static void mh_init_key(uint32_t* mh_key, uint32_t seed)
{
    // Initialize the XOR cipher key using a seed
    mh_key[0] = seed >> 16;
    if (mh_key[0] == 0)
        mh_key[0] = MH_KEY_DEF0;

    mh_key[1] = seed & 0xffff;
    if (mh_key[1] == 0)
        mh_key[1] = MH_KEY_DEF1;
}

static uint32_t mh_next_key(uint32_t* mh_key)
{
    // Calculate a new XOR cipher key based on the previous key
    mh_key[0] *= MH_KEY_DEF0;
    mh_key[0] %= MH_KEY_MOD0;
    mh_key[1] *= MH_KEY_DEF1;
    mh_key[1] %= MH_KEY_MOD1;

    return ((mh_key[0] << 16) + mh_key[1]);
}

static void mh_buffer_translate(uint8_t* data, int len, const uint8_t* table)
{
    for(int i=0; i < len; i++)
        data[i] = table[data[i]];
}

static void mh_xor_block(uint8_t* data, int len, int lba)
{
    uint32_t keys[2];
    uint32_t* buff = (uint32_t*) data;
    len /= 4;

    // Use the block address to seed the XOR cipher key
    mh_init_key(keys, lba);

    // Apply an XOR cipher to the data using a new key every 4 bytes
    for (int i=0; i < len; i++)
    {
        LE32(buff[i]);
        buff[i] ^= mh_next_key(keys);
        LE32(buff[i]);
    }

    return;
}

void monsterhunter_decrypt_data(uint8_t* buff, uint32_t size, int ver)
{
    uint32_t seed;
    const uint8_t* dec_table = (ver == 3) ? MH3_DEC_TABLE : MH2_DEC_TABLE;

    LOG("[*] Total Decrypted Size: 0x%X (%d bytes)", size, size);

    // Get the XOR cipher seed from the end of the data and apply a
    // substitution cipher
    mh_buffer_translate(&buff[size-4], 4, dec_table);
    mh_buffer_translate(buff, size, dec_table);

    seed = *((uint32_t*) &buff[size-4]);
	LE32(seed);
    LOG("[*] Encryption Seed: %08X", seed);

    // Decrypt the data
    mh_xor_block(buff, size-4, seed);

    // Apply a substitution cipher to the data
    mh_buffer_translate(buff, size-4, dec_table);

    LOG("[*] Decrypted File Successfully!");

    return;
}

void monsterhunter_encrypt_data(uint8_t* buff, uint32_t size, int ver)
{
    uint32_t seed;
    const uint8_t* enc_table = (ver == 3) ? MH3_ENC_TABLE : MH2_ENC_TABLE;

    LOG("[*] Total Encrypted Size: 0x%X (%d bytes)", size, size);

    // Get a new seed for the XOR cipher
    seed = *((uint32_t*) &buff[size-4]);
	LE32(seed);
    LOG("[*] Encryption Seed: %08X", seed);

    // Apply a substitution cipher to the data and encrypt it
    mh_buffer_translate(buff, size-4, enc_table);
    mh_xor_block(buff, size-4, seed);

    // Apply a substitution cipher to the XOR cipher seed and append it to the data
    mh_buffer_translate(buff, size, enc_table);
    mh_buffer_translate(&buff[size-4], 4, enc_table);

    LOG("[*] Encrypted File Successfully!");
    return;
}
