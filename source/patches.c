#include <polarssl/md.h>
#include <polarssl/md5.h>
#include <polarssl/sha1.h>
#include <polarssl/sha256.h>
#include <polarssl/sha512.h>
#include <zlib.h>
#include <dirent.h>

#include "apollo.h"
#include "types.h"
#include "crc_util.h"

#define SUCCESS                 0
#define skip_spaces(str)        while (*str == ' ') str++;

typedef enum
{
    BSD_VAR_NULL = 0,
    BSD_VAR_INT8 = 1,
    BSD_VAR_INT16 = 2,
    BSD_VAR_INT32 = 4,
    BSD_VAR_INT64 = 8,
    BSD_VAR_MD5 = 16,
    BSD_VAR_SHA1 = 20,
    BSD_VAR_SHA256 = 32,
    BSD_VAR_SHA512 = 64,
} bsd_var_types;

typedef struct
{
    char* name;
    int len;
    uint8_t* data;
} bsd_variable_t;

static list_t* var_list = NULL;
static apollo_host_cb_t host_callback = NULL;


static long search_data(const char* data, size_t size, int start, const char* search, int len, int count)
{
	int k = 1;

	for (size_t i = start; i <= (size-len); i++)
		if ((memcmp(data + i, search, len) == 0) && (k++ == count))
			return i;

    return -1;
}

static long reverse_search_data(const char* data, size_t size, int start, const char* search, int len, int count)
{
	int k = 1;

	for (long i = start; i >= 0; i--)
		if ((i+len <= (long)size) && (memcmp(data + i, search, len) == 0) && (k++ == count))
			return i;

	return -1;
}

static bsd_variable_t* _get_bsd_variable(const char* vname)
{
	list_node_t *node;
	bsd_variable_t *var;

	for (node = list_head(var_list); (var = list_get(node)); node = list_next(node))
		if (strcmp(var->name, vname) == 0)
			return var;

	return NULL;
}

static char* _decode_variable_data(const char* line, int *data_len)
{
    int i, len = 0;
    char* output = NULL;

    skip_spaces(line);
	if (wildcard_match(line, "\"*\"*"))
	{
	    char* c = strchr(line, '"')+1;
	    len = strrchr(line, '"') - c;
		output = malloc(len);
	    
	    for (i = 0; i < len; i++)
	        output[i] = c[i];
	}
	else if (wildcard_match(line, "[*]*"))
	{
	    line++;
	    
	    char* tmp = strchr(line, ']');
	    *tmp = 0;
	    
	    bsd_variable_t* var = _get_bsd_variable(line);
	    
	    line = tmp+1;
	    *tmp = ']';
	    
	    if (var)
	    {
	        len = var->len;
    		output = malloc(len);
	        memcpy(output, var->data, len);

			switch (len)
			{
			case BSD_VAR_INT16:
				BE16(*((uint16_t*)output));
				break;
			case BSD_VAR_INT32:
				BE32(*((uint32_t*)output));
				break;
			case BSD_VAR_INT64:
				BE64(*((uint64_t*)output));
				break;
			default:
				break;
			}
	    }
	}
	else
	{
	    if (line[0] == '0' && line[1] == 'x')
	        line += 2;

	    len = strlen(line) / 2;
	    output = (char*) x_to_u8_buffer(line);
	}

	*data_len = len;
	return output;
}

static int _parse_int_value(const char* line, const int ptrval, const int size)
{
    int ret = 0, neg = 0;

    skip_spaces(line);
	if (line[0] == '+') line++;
	if (line[0] == '-')
	{
		neg = 1;
		line++;
	}

    if (strlen(line) == 0)
    {
        return 0;
    }
	else if (wildcard_match_icase(line, "pointer*"))
	{
	    line += strlen("pointer");
	    skip_spaces(line);

	    ret = ptrval + _parse_int_value(line, ptrval, size);
	}
	else if (wildcard_match_icase(line, "eof*"))
	{
		line += strlen("eof");
	    skip_spaces(line);

//        sscanf(line, "%x", &ret);
        ret += size - 1 + _parse_int_value(line, ptrval, size);
	}
	else if (wildcard_match(line, "[*]*"))
	{
	    line++;
	    
	    char* tmp = strchr(line, ']');
	    *tmp = 0;
	    
	    bsd_variable_t* var = _get_bsd_variable(line);
	    
	    line = tmp+1;
	    *tmp = ']';
	    
	    if (var)
	    {
	        switch (var->len)
	        {
	            case BSD_VAR_INT8:
        	        ret = *((uint8_t*)var->data) + _parse_int_value(line, ptrval, size);
        	        break;
	            case BSD_VAR_INT16:
        	        ret = *((uint16_t*)var->data) + _parse_int_value(line, ptrval, size);
        	        break;
	            case BSD_VAR_INT32:
        	        ret = *((uint32_t*)var->data) + _parse_int_value(line, ptrval, size);
        	        break;
	        }
	    }
	}
	else
	{
	    sscanf(line, "%x", &ret);
	}
	
	return (neg ? -ret : ret);
}

void free_patch_var_list(void)
{
	list_node_t *node;
	bsd_variable_t* bv;

	for (node = list_head(var_list); (bv = list_get(node)); node = list_next(node))
	{
		if (bv->data)
			free(bv->data);
		if (bv->name)
			free(bv->name);

		free(bv);
	}

	list_free(var_list);
	var_list = NULL;
}

static void _parse_start_end(char* line, int pointer, int dsize, int *start_val, int *end_val)
{
	char *tmp;

	tmp = strchr(line, ',');
	*tmp = 0;
	
	*start_val = _parse_int_value(line, pointer, dsize);

	line = tmp+1;
	*tmp = ',';
	tmp = strchr(line, ')');
	*tmp = 0;

	*end_val = _parse_int_value(line, pointer, dsize);
	*tmp = ')';
}

static void _log_dump(const char* name, const uint8_t* buf, int size)
{
	char msgout[64];
	char ascii[32];

	LOG("----- %s %d bytes -----", name, size);
	for (int i = 0; i < size; i++)
	{
		if (i && !(i % 16))
			LOG("%06X: %s | %s", i-0x10, msgout, ascii);

		sprintf(msgout + (i % 16)*3, "%02X ", buf[i]);
		sprintf(ascii  + (i % 16), "%c", (buf[i] > 32 && buf[i] < 128) ? buf[i] : '.');
	}
	LOG("%06X: %-48s | %s", (size-1) & ~15, msgout, ascii);
	LOG("----- %s %d bytes -----", name, size);
}

static void swap_u16_data(uint16_t* data, int count)
{
	for (int i=0; i < count; i++)
		data[i] = ES16(data[i]);
}

static void swap_u32_data(uint32_t* data, int count)
{
	for (int i=0; i < count; i++)
		data[i] = ES32(data[i]);
}

static void swap_u64_data(uint64_t* data, int count)
{
	for (int i=0; i < count; i++)
		data[i] = ES64(data[i]);
}

int apply_bsd_patch_code(const char* filepath, const code_entry_t* code)
{
	char *data, *bsd_code;
	size_t dsize;
	long pointer = 0;
	long range_start = 0, range_end = 0;
	uint8_t carry = 0, eof = 0;
	uint32_t old_val = 0;
	custom_crc_t custom_crc = {0,0,0,0,0,0};

	LOG("Applying [%s] to '%s'...", code->name, filepath);
	if (read_buffer(filepath, (uint8_t**) &data, &dsize) != SUCCESS)
	{
		LOG("Can't load file '%s'", filepath);
		return 0;
	}

	range_end = dsize;
	if (!var_list)
		var_list = list_alloc();

	bsd_code = strdup(code->codes);
	for (char *line = strtok(bsd_code, "\n"); line != NULL; line = strtok(NULL, "\n"))
	{
        // carry(*)
		if (wildcard_match_icase(line, "carry(*)"))
		{
			int tmpi;
			// carry setting for add() / sub()
			line += strlen("carry");
		    sscanf(line, "(%d)", &tmpi);
			carry = tmpi;
		    
		    LOG("Set carry bytes = %d", carry);
		}

        // set *:*
		else if (wildcard_match_icase(line, "set *:*"))
		{
			// set pointer: 43
			// ;Sets the file pointer to a specific address.
			//
			// set [hash]:md5
			// ;[hash] is the variable. it can be any name. you can have many variables.
			// ;md5 is a function. the md5 hash is calculated for the current file, and stored in the variable [hash]
			//
			// set [myvariable2]:"hello"
			// ;sets the text "hello" into the variable [myvariable2]
			//
			// set [anyname1]:read(0x100, (10)) 
			// ;read(offset, length) is a function that reads n bytes from current file
			// ;0x100 is the offset in hex
			// ;(10) is the length in decimal
			// ;the example 4 reads 10 bytes starting from offset 0x100 and store them in the variable [anyname1]

			// set *:param = "lastbyte*"
			// set *:param = "eof*"
			// set *:param = "xor:*", "not *", "md5*", "crc*", "eachecksum", "adler16", "adler32", "sha1*", "sha256", 
			// set *:param = "sha384", "sha512", "hmac*", "md4*", "md2*", "read(*,*)*", "xor(*,*,*)*", "add(*,*)", 
			// set *:param = "wadd(*,*)", "[dq]wadd(*,*)", "sub(*,*)", "wsub(*,*)", "[dq]wsub(*,*)", "repeat(*,*)*", 
			// set *:param = "mid(*,*)", "left(*,*)", "right(*,*)"
			// UNUSED: "userid*", "titleid*", "psid*", "account*", "profile*",
			// UNUSED: crc16, adler16, md4, sha384, sha512, left, not
			//
			// "set range:*,*", "set crc_*:*", "set md5:*", "set md2:*", "set sha1:*", "set crc32:*", "set crc32big:*", 
			// "set crc32little:*", "set crc16:*", "set adler16:*", "set adler32:*",  
			// "set ""*"":*", "set pointer:*"
			// UNUSED: "set psid:*", "set userid:*", "set titleid:*", "set *account*:*", "set *profile*:*",
			// UNUSED: crc16, adler16, md4, sha384, sha512,

			int ptr_off, len;
			char* tmp = NULL;

			line += strlen("set");
		    skip_spaces(line);

		    // set pointer:*
			if (wildcard_match_icase(line, "pointer:*"))
			{
    			line += strlen("pointer:");

    			// set pointer:eof*
    			if (wildcard_match_icase(line, "eof*"))
    			{
        			line += strlen("eof");
        		    skip_spaces(line);

                    eof = 1;
                    sscanf(line, "%x", &ptr_off);
                    pointer = dsize + ptr_off - 1;
    			}
    			// set pointer:lastbyte*
    			else if (wildcard_match_icase(line, "lastbyte*"))
    			{
        			line += strlen("lastbyte");
        		    skip_spaces(line);

                    eof = 1;
                    sscanf(line, "%x", &ptr_off);
                    pointer = dsize + ptr_off - 1;
    			}
    			// set pointer:pointer*
    			else if (wildcard_match_icase(line, "pointer*"))
    			{
        			line += strlen("pointer");
        		    skip_spaces(line);

                    sscanf(line, "%x", &ptr_off);
                    pointer += ptr_off;
    			}
    			// set pointer:read(*,*)*
    			else if (wildcard_match_icase(line, "read(*,*)*"))
    			{
        			line += strlen("read");
        			
        			int raddr, rlen;
        			sscanf(line, "(%x,%x)", &raddr, &rlen);

                    uint32_t rval = *((uint32_t*) &data[raddr]);
					BE32(rval);
            	    LOG("address = %d len %d ", raddr, rlen);
            	    
            	    pointer = rval;
    			}
    			// set pointer:[*]*
    			else if (wildcard_match_icase(line, "[*]*"))
    			{
    			    LOG("Getting value for %s", line);
    			    pointer = _parse_int_value(line, pointer, dsize);
    			}
    			// set pointer:* (e.g. 0x00000000)
    			else
    			{
                    sscanf(line, "%x", &ptr_off);
                    pointer = ptr_off;
    			}

                LOG("POINTER = %ld (0x%lX)", pointer, pointer);
			}

			// set range:*,*
			else if (wildcard_match_icase(line, "range:*,*"))
			{
    			line += strlen("range:");

			    tmp = strchr(line, ',');
			    *tmp = 0;
			    
			    range_start = _parse_int_value(line, pointer, dsize);
				if (range_start < 0)
					range_start = 0;

			    line = tmp+1;
			    *tmp = ',';

				range_end = _parse_int_value(line, pointer - eof, dsize) + 1;
				if (range_end > (long)dsize)
					range_end = dsize;

                LOG("RANGE = %ld (0x%lX) - %ld (0x%lX)", range_start, range_start, range_end, range_end);
			}

			// set crc_*:*
			else if (wildcard_match_icase(line, "crc_*:*"))
			{
				int tmpi;
			    line += strlen("crc_");

			    if (wildcard_match_icase(line, "bandwidth:*"))
			    {
    			    line += strlen("bandwidth:");
    			    sscanf(line, "%d", &tmpi);
					custom_crc.width = tmpi;
			    }

			    else if (wildcard_match_icase(line, "polynomial:*"))
			    {
    			    line += strlen("polynomial:");
    			    sscanf(line, "%" PRIx64, &custom_crc.poly);
			    }

			    else if (wildcard_match_icase(line, "initial_value:[*]*"))
			    {
    			    line += strlen("initial_value:");
    			    custom_crc.init = _parse_int_value(line, pointer, dsize);
			    }

				else if (wildcard_match_icase(line, "initial_value:*"))
				{
					line += strlen("initial_value:");
					sscanf(line, "%" PRIx64, &custom_crc.init);
				}

			    else if (wildcard_match_icase(line, "output_xor:*"))
			    {
    			    line += strlen("output_xor:");
    			    sscanf(line, "%" PRIx64, &custom_crc.xor);
			    }

			    else if (wildcard_match_icase(line, "reflection_input:*"))
			    {
    			    line += strlen("reflection_input:");
    			    sscanf(line, "%d", &tmpi);
					custom_crc.refIn = tmpi;
			    }

			    else if (wildcard_match_icase(line, "reflection_output:*"))
			    {
    			    line += strlen("reflection_output:");
    			    sscanf(line, "%d", &tmpi);
					custom_crc.refOut = tmpi;
			    }
			}

			// set [*]:*
			else if (wildcard_match(line, "[*]:*"))
			{
			    line++;
			    
			    tmp = strchr(line, ']');
			    *tmp = 0;
			    
        	    bsd_variable_t* var = _get_bsd_variable(line);

			    if (!var)
			    {
			        var = malloc(sizeof(bsd_variable_t));
    			    var->name = strdup(line);
    			    var->data = NULL;
    			    var->len = BSD_VAR_NULL;
    			    list_append(var_list, var);
    			}
    			else
    			{
    			    // for now we don't update variable values, we only overwrite
			        switch (var->len)
			        {
			            case BSD_VAR_INT8:
		        	        old_val = *((uint8_t*)var->data);
		        	        break;
			            case BSD_VAR_INT16:
		        	        old_val = *((uint16_t*)var->data);
		        	        break;
			            case BSD_VAR_INT32:
		        	        old_val = *((uint32_t*)var->data);
		        	        break;
		        	    default:
		        	    	old_val = 0;
		        	    	break;
			        }

    			    if (var->data)
    			    {
    			        free(var->data);
    			        var->data = (uint8_t*) &old_val + PADDING(4 - var->len);
    			    }

    			    LOG("Old value 0x%X", old_val);
    			}

        	    LOG("Var name = %s", var->name);

			    line = tmp+2;
			    *tmp = ']';

    		    // set [*]:xor:*
    			if (wildcard_match_icase(line, "xor:*"))
    			{
    			    line += strlen("xor:");
        		    skip_spaces(line);
    
    			    int wlen;
    			    char* xor_val = _decode_variable_data(line, &wlen);

    			    if (var->len != wlen)
    			    {
						// variable has different length
						LOG("[%s]:XOR: error! var length doesn't match", var->name);
						dsize = 0;
						goto bsd_end;
    			    }
#ifndef __PPU__
					// workaround: _decode_variable_data() returns data as big endian
					// if not PPU, we need to convert it to little endian to match the data
					char* le_val = malloc(wlen);
    			    
					for (int i=0; i < wlen; i++)
						le_val[i] = xor_val[wlen - i - 1];

					memcpy(xor_val, le_val, wlen);
					free(le_val);
#endif
    			    for (int i=0; i < wlen; i++)
    			        xor_val[i] ^= var->data[i];

                    var->data = (uint8_t*) xor_val;

    			    LOG("Var [%s]:XOR = %s ^ %X", var->name, line, old_val);
    			}

				// set [*]:endian_swap*
				else if (wildcard_match_icase(line, "endian_swap*"))
				{
					if (var->len != BSD_VAR_INT16 && var->len != BSD_VAR_INT32 && var->len != BSD_VAR_INT64)
					{
						// variable has different length
						LOG("[%s]:endian_swap error! unsupported var length (%d)", var->name, var->len);
						dsize = 0;
						goto bsd_end;
					}

					uint8_t* le_val = malloc(var->len);
					
					for (int i=0; i < var->len; i++)
						le_val[i] = var->data[var->len - i - 1];

					var->data = le_val;

					LOG("Var [%s]:endian_swap( %X )", var->name, old_val);
				}

				// set [*]:[*]*
				else if (wildcard_match_icase(line, "[*]*"))
				{
					uint32_t val = _parse_int_value(line, pointer, dsize);

					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &val, var->len);

					LOG("Var [%s]:%s = %08X", var->name, line, val);
				}

			    // set [*]:crc32*
			    else if (wildcard_match_icase(line, "crc32*"))
			    {
			        uint32_t hash;
			        custom_crc.init = CRC_32_INIT_VALUE;
					custom_crc.poly = CRC_32_POLYNOMIAL;
					custom_crc.xor = CRC_32_XOR_VALUE;

    			    tmp = strchr(line, ':');
    			    if (tmp)
    			    {
    			        sscanf(tmp+1, "%" PRIx64, &custom_crc.init);
    			    }
    			    
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

    			    if (wildcard_match_icase(line, "crc32big*"))
    			    {
    			        // CRC-32/BZIP
						custom_crc.refIn = 0;
						custom_crc.refOut = 0;

						hash = crc32_hash(start, len, &custom_crc);
						LOG("len %d CRC32Big HASH = %08X", len, hash);
    			    }
    			    else
    			    {
						// CRC-32 (crc32, crc32little)
						custom_crc.refIn = 1;
						custom_crc.refOut = 1;

						hash = crc32_hash(start, len, &custom_crc);
						LOG("len %d CRC32 HASH = %08X", len, hash);    			    
    			    }

                    var->len = BSD_VAR_INT32;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) &hash, var->len);
			    }

			    // set [*]:crc16*
			    // CRC-16/XMODEM
			    else if (wildcard_match_icase(line, "crc16*"))
			    {
			        uint16_t hash;
			        custom_crc.init = CRC_16_INIT_VALUE;
					custom_crc.poly = CRC_16_POLYNOMIAL;
					custom_crc.xor = CRC_16_XOR_VALUE;
					custom_crc.refIn = 0;
					custom_crc.refOut = 0;

    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

    			    hash = crc16_hash(start, len, &custom_crc);

                    var->len = BSD_VAR_INT16;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) &hash, var->len);

    			    LOG("len %d CRC16 HASH = %04X", len, hash);
			    }

			    // set [*]:crc64*
			    else if (wildcard_match_icase(line, "crc64*"))
			    {
					//crc64_ecma / crc64_iso
					uint64_t hash;

					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					if (wildcard_match_icase(line, "crc64_ecma*"))
					{
						// CRC-64 ECMA 182
						custom_crc.poly = CRC_64_ECMA182_POLY;
						custom_crc.init = CRC_64_ECMA182_INIT_VALUE;
						custom_crc.xor = CRC_64_ECMA182_XOR_VALUE;
						custom_crc.refIn = 0;
						custom_crc.refOut = 0;

						hash = crc64_hash(start, len, &custom_crc);
						LOG("len %d CRC64 ECMA HASH = %016" PRIX64, len, hash);
					}
					else
					{
						// CRC-64 ISO
						custom_crc.poly = CRC_64_ISO_POLY;
						custom_crc.init = CRC_64_ISO_INIT_VALUE;
						custom_crc.xor = CRC_64_ISO_XOR_VALUE;
						custom_crc.refIn = 1;
						custom_crc.refOut = 1;

						hash = crc64_hash(start, len, &custom_crc);
						LOG("len %d CRC64 ISO HASH = %016" PRIX64, len, hash);
					}

					var->len = BSD_VAR_INT64;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);
			    }

			    // set [*]:crc*
			    // Custom CRC
			    else if (wildcard_match_icase(line, "crc*"))
			    {
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

                    LOG("CRC %d Poly %lX Init %lX", custom_crc.width, custom_crc.poly, custom_crc.init);
                    LOG("Xor %lX RefIn %d RefOut %d", custom_crc.xor, custom_crc.refIn, custom_crc.refOut);

			        if (custom_crc.width == CRC_16_RESULT_WIDTH)
			        {
    			        // Custom CRC-16
    			        uint16_t hash = crc16_hash(start, len, &custom_crc);

                        var->len = BSD_VAR_INT16;
                        var->data = malloc(var->len);
                        memcpy(var->data, (uint8_t*) &hash, var->len);

        			    LOG("len %d Custom CRC16 HASH = %04X", len, hash);
			        }
					else if (custom_crc.width == CRC_64_RESULT_WIDTH)
					{
						// Custom CRC-64
						uint64_t hash = crc64_hash(start, len, &custom_crc);

						var->len = BSD_VAR_INT64;
						var->data = malloc(var->len);
						memcpy(var->data, (uint8_t*) &hash, var->len);

						LOG("len %d Custom CRC64 HASH = %016" PRIX64, len, hash);
					}
			        else
			        {
    			        // Custom CRC-32
    			        uint32_t hash = crc32_hash(start, len, &custom_crc);

                        var->len = BSD_VAR_INT32;
                        var->data = malloc(var->len);
                        memcpy(var->data, (uint8_t*) &hash, var->len);

        			    LOG("len %d Custom CRC32 HASH = %08X", len, hash);
			        }
			    }

			    // set [*]:md5_xor*
			    else if (wildcard_match_icase(line, "md5_xor*"))
			    {
    			    uint32_t hash[4];
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

					md5(start, len, (uint8_t*) hash);
					hash[0] ^= (hash[1] ^ hash[2] ^ hash[3]);
					LE32(hash[0]);

                    var->len = BSD_VAR_INT32;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) hash, var->len);

    			    LOG("len %d MD5_XOR HASH = %08X", len, *(uint32_t*)hash);
			    }

			    // set [*]:md5*
			    else if (wildcard_match_icase(line, "md5*"))
			    {
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

                    var->len = BSD_VAR_MD5;
                    var->data = malloc(var->len);
			        md5(start, len, var->data);

					LOG("len %d MD5", len);
					_log_dump("MD5 HASH", var->data, var->len);
			    }

			    // set [*]:md2*
			    else if (wildcard_match_icase(line, "md2*"))
			    {
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

                    var->len = BSD_VAR_MD5;
                    var->data = malloc(var->len);
                    md(md_info_from_type(POLARSSL_MD_MD2), start, len, var->data);

					LOG("len %d MD2", len);
					_log_dump("MD2 HASH", var->data, var->len);
			    }

			    // set [*]:md4*
			    else if (wildcard_match_icase(line, "md4*"))
			    {
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

                    var->len = BSD_VAR_MD5;
                    var->data = malloc(var->len);
                    md(md_info_from_type(POLARSSL_MD_MD4), start, len, var->data);

					LOG("len %d MD4", len);
					_log_dump("MD4 HASH", var->data, var->len);
				}

				// set [*]:sha1_xor64*
				else if (wildcard_match_icase(line, "sha1_xor64*"))
				{
					uint64_t sha[3] = {0, 0, 0};
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					sha1(start, len, (uint8_t*) sha);
					sha[0] ^= (sha[1] ^ sha[2]);
					BE64(sha[0]);

					var->len = BSD_VAR_INT64;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) sha, var->len);

					LOG("len %d SHA1_XOR64 HASH = %016" PRIX64, len, ((uint64_t*)var->data)[0]);
				}

			    // set [*]:sha1*
			    else if (wildcard_match_icase(line, "sha1*"))
			    {
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

                    var->len = BSD_VAR_SHA1;
                    var->data = malloc(var->len);
			        sha1(start, len, var->data);

					LOG("len %d SHA1", len);
					_log_dump("SHA1 HASH", var->data, var->len);
			    }

			    // set [*]:sha256*
			    else if (wildcard_match_icase(line, "sha256*"))
			    {
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

                    var->len = BSD_VAR_SHA256;
                    var->data = malloc(var->len);
					sha256(start, len, var->data, 0);

					LOG("len %d SHA256", len);
					_log_dump("SHA256 HASH", var->data, var->len);
			    }

			    // set [*]:sha384*
			    else if (wildcard_match_icase(line, "sha384*"))
			    {
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

                    var->len = BSD_VAR_SHA512;
                    var->data = malloc(var->len);
					sha512(start, len, var->data, 1);

					LOG("len %d SHA384", len);
					_log_dump("SHA384 HASH", var->data, var->len);
			    }

			    // set [*]:sha512*
			    else if (wildcard_match_icase(line, "sha512*"))
			    {
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

                    var->len = BSD_VAR_SHA512;
                    var->data = malloc(var->len);
					sha512(start, len, var->data, 0);

					LOG("len %d SHA512", len);
					_log_dump("SHA512 HASH", var->data, var->len);
			    }

			    // set [*]:adler32*
			    else if (wildcard_match_icase(line, "adler32*"))
			    {
    			    uint32_t hash = adler32(0L, Z_NULL, 0);
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

					tmp = strchr(line, ':');
					if (tmp)
					{
						hash = _parse_int_value(tmp+1, pointer, dsize);
					}

    			    hash = adler32(hash, start, len);

                    var->len = BSD_VAR_INT32;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) &hash, var->len);

    			    LOG("len %d Adler32 HASH = %08X", len, hash);
			    }

			    // set [*]:adler16*
			    else if (wildcard_match_icase(line, "adler16*"))
			    {
					uint16_t hash;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					hash = adler16(start, len);

					var->len = BSD_VAR_INT16;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d Adler16 HASH = %04X", len, hash);
			    }

				// set [*]:murmur3_32*
				else if (wildcard_match_icase(line, "murmur3_32*"))
				{
					uint32_t hash;
					len = range_end - range_start;

					tmp = strchr(line, ':');
					hash = tmp ? _parse_int_value(tmp+1, pointer, dsize) : 0;

					hash = murmur3_32((uint8_t*)data + range_start, len, hash);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d Murmur3-32 HASH = %08X", len, hash);
				}

				// set [*]:jhash*
				else if (wildcard_match_icase(line, "jhash*"))
				{
					uint32_t hash;
					len = range_end - range_start;

					tmp = strchr(line, ':');
					hash = tmp ? _parse_int_value(tmp+1, pointer, dsize) : 0;

					hash = jhash((uint8_t*)data + range_start, len, hash);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d JHASH = %08X", len, hash);
				}

				// set [*]:jenkins_oaat*
				else if (wildcard_match_icase(line, "jenkins_oaat*"))
				{
					uint32_t hash;
					len = range_end - range_start;

					tmp = strchr(line, ':');
					hash = tmp ? _parse_int_value(tmp+1, pointer, dsize) : 0;

					hash = jenkins_oaat_hash((uint8_t*)data + range_start, len, hash);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d Jenkins OAAT HASH = %08X", len, hash);
				}

			    // set [*]:hmac_sha1*
			    else if (wildcard_match_icase(line, "hmac_sha1(*)*"))
			    {
					char *key;
					int key_len;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					line += strlen("hmac_sha1(");
					tmp = strrchr(line, ')');
					*tmp = 0;

					LOG("HMAC Key=%s", line);

					key = _decode_variable_data(line, &key_len);
					*tmp = ')';

					var->len = BSD_VAR_SHA1;
					var->data = malloc(var->len);
					sha1_hmac((uint8_t*) key, key_len, start, len, var->data);
					free(key);

					LOG("len %d SHA1/HMAC", len);
					_log_dump("SHA1/HMAC HASH", var->data, var->len);
			    }

			    // set [*]:force_crc32:*
			    else if (wildcard_match_icase(line, "force_crc32:*"))
			    {
					uint32_t hash, newcrc;
					len = range_end - range_start;

					line = strchr(line, ':')+1;
					newcrc = _parse_int_value(line, pointer, dsize);

					hash = force_crc32((uint8_t*)data + range_start, len, pointer, newcrc);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d Force-CRC32 (%08X) HASH = %08X", len, newcrc, hash);
			    }

			    // set [*]:eachecksum*
			    else if (wildcard_match_icase(line, "eachecksum*"))
			    {
			        uint32_t hash;
    			    uint8_t* start = (uint8_t*)data + range_start;
    			    len = range_end - range_start;

    			    hash = MC02_hash(start, len);

                    var->len = BSD_VAR_INT32;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) &hash, var->len);

    			    LOG("len %d EA/MC02 HASH = %08X", len, hash);
			    }

				// set [*]:ffx_checksum*
				else if (wildcard_match_icase(line, "ffx_checksum*"))
				{
					uint16_t hash;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					// FFX hash is stored in little-endian
					hash = ffx_hash(start, len);
					hash = ES16(hash);

					var->len = BSD_VAR_INT16;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d FFX HASH = %04X", len, hash);
				}

				// set [*]:ff13_checksum*
				else if (wildcard_match_icase(line, "ff13_checksum*"))
				{
					uint32_t hash;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					// FFXIII hash is stored in little-endian
					hash = ff13_checksum(start, len);
					hash = ES32(hash);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d FFXIII HASH = %08X", len, hash);
				}

				// set [*]:castlevania_checksum*
				else if (wildcard_match_icase(line, "castlevania_checksum*"))
				{
					uint32_t hash;
					len = range_end - range_start;

					// Castlevania LOS hash is stored in little-endian
					hash = castlevania_hash((uint8_t*)data + range_start, len);
					hash = ES32(hash);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d Castlevania HASH = %08X", len, hash);
				}

				// set [*]:deadrising_checksum*
				else if (wildcard_match_icase(line, "deadrising_checksum*"))
				{
					int blocks;
					len = range_end - range_start;

					blocks = deadrising_checksum((uint8_t*)data + range_start, len);
					LOG("len %d Dead Rising checksum: %d blocks updated", len, blocks);
				}

				// set [*]:rockstar_checksum*
				else if (wildcard_match_icase(line, "rockstar_checksum*"))
				{
					int chks_off;
					uint32_t chks = 0, chks_len = 0;
					len = range_end - range_start;

					// Updates all CHKS values
					chks_off = search_data(data, len, range_start, "CHKS", 5, 1);
					while (chks_off > 0)
					{
						chks     = (*(uint32_t*)(data + chks_off + 4));
						chks_len = (*(uint32_t*)(data + chks_off + 8));
						BE32(chks);
						BE32(chks_len);

						memset(data + chks_off + 8, 0, 8);
						chks = jenkins_oaat_hash((uint8_t*) (data + chks_off - chks_len + chks), chks_len, 0x3FAC7125);
						LOG(" + CHKS Size: 0x%X Offset: 0x%X - Wrote Checksum: %08X", chks_len, chks_off, chks);

						BE32(chks);
						BE32(chks_len);
						memcpy(data + chks_off + 0xC, &chks, sizeof(uint32_t));
						memcpy(data + chks_off + 0x8, &chks_len, sizeof(uint32_t));

						chks_off = search_data(data, len, chks_off+1, "CHKS", 5, 1);
					}

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &chks, var->len);

					LOG("len %d Rockstar CHKS %s", len, chks_len ? "OK" : "ERROR");
				}

				// set [*]:lookup3_little2(*,*)*
				else if (wildcard_match_icase(line, "lookup3_little2(*,*)*"))
				{
					uint64_t hash;
					uint32_t iv1, iv2;
					len = range_end - range_start;

					line += strlen("lookup3_little2(");
					_parse_start_end(line, pointer, dsize, (int*) &iv1, (int*) &iv2);
					LOG("lookup3 init values %X %X", iv1, iv2);

					lookup3_hashlittle2((uint8_t*)data + range_start, len, &iv1, &iv2);
					hash = iv2 + (((uint64_t) iv1) << 32);

					var->len = BSD_VAR_INT64;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d lookup3_little2 Hashes = %08X %08X", len, iv1, iv2);
				}

				// set [*]:kh25_checksum*
				else if (wildcard_match_icase(line, "kh25_checksum*"))
				{
					uint32_t hash;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					// Kingdom Hearts 2.5 hash is stored in little-endian
					hash = kh25_hash(start, len);
					hash = ES32(hash);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d KH 2.5 HASH = %08X", len, hash);
				}

				// set [*]:khcom_checksum*
				else if (wildcard_match_icase(line, "khcom_checksum*"))
				{
					uint32_t hash;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					hash = kh_com_hash(start, len);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d KH CoM HASH = %08X", len, hash);
				}

				// set [*]:mgs2_checksum*
				else if (wildcard_match_icase(line, "mgs2_checksum*"))
				{
					uint32_t hash;
					len = range_end - range_start;

					hash = mgs2_hash((uint8_t*)data + range_start, len);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d MGS2 HASH = %08X", len, hash);
				}

				// set [*]:mgspw_checksum*
				else if (wildcard_match_icase(line, "mgspw_checksum*"))
				{
					uint32_t hash;
					len = range_end - range_start;

					hash = mgspw_Checksum((uint8_t*)data + range_start, len);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d MGS PW HASH = %08X", len, hash);
				}

				// set [*]:sw4_checksum*
				else if (wildcard_match_icase(line, "sw4_checksum*"))
				{
					uint32_t hash[4];
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					sw4_hash(start, len, hash);

					var->len = BSD_VAR_MD5;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d SW4 HASH = %08X %08X %08X %08X", len, hash[0], hash[1], hash[2], hash[3]);
				}

				// set [*]:toz_checksum*
				else if (wildcard_match_icase(line, "toz_checksum*"))
				{
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					var->len = BSD_VAR_SHA1;
					var->data = malloc(var->len);
					toz_hash(start, len, var->data);

					LOG("len %d TOZ SHA1", len);
					_log_dump("TOZ SHA1 HASH", var->data, var->len);
				}

				// set [*]:tiara2_checksum*
				else if (wildcard_match_icase(line, "tiara2_checksum*"))
				{
					uint32_t hash;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					hash = tiara2_hash(start, len);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d Tiara2 HASH = %08X", len, hash);
				}

				// set [*]:checksum32*
				else if (wildcard_match_icase(line, "checksum32*"))
				{
					uint32_t hash;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					hash = Checksum32_hash(start, len);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d Checksum32 = %08X", len, hash);
				}

				// set [*]:sdbm*
				else if (wildcard_match_icase(line, "sdbm*"))
				{
					uint32_t hash, init_val = 0;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					tmp = strchr(line, ':');
					if (tmp)
					{
						init_val = _parse_int_value(tmp+1, pointer, dsize);
					}

					hash = sdbm_hash(start, len, init_val);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d SDBM HASH = %08X", len, hash);
				}

				// set [*]:fnv1*
				else if (wildcard_match_icase(line, "fnv1*"))
				{
					uint32_t hash, init_val = 0x811c9dc5;
					uint8_t* start = (uint8_t*)data + range_start;
					len = range_end - range_start;

					tmp = strchr(line, ':');
					if (tmp)
					{
						init_val = _parse_int_value(tmp+1, pointer, dsize);
					}

					hash = fnv1_hash(start, len, init_val);

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &hash, var->len);

					LOG("len %d FNV-1 HASH = %08X", len, hash);
				}

			    // set [*]:qwadd(*,*)*
			    else if (wildcard_match_icase(line, "qwadd(*,*)*"))
			    {
					// qwadd(<start>,<endrange>)
					// 64-bit	0xFFFFFFFFFFFFFFFF
					int add_s, add_e;
					uint32_t add = old_val;

					line += strlen("qwadd(");
					_parse_start_end(line, pointer, dsize, &add_s, &add_e);
					char* read = data + add_s + BSD_VAR_INT32;
					
					while (read < data + add_e)
					{
						uint32_t radd = (*(uint32_t*)read);
						BE32(radd);
						add += radd;
						read += BSD_VAR_INT64;
					}

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &add, var->len);
					
					LOG("[%s]:qwadd(0x%X , 0x%X) = %X", var->name, add_s, add_e, add);
			    }

			    // set [*]:dwadd(*,*)*
			    else if (wildcard_match_icase(line, "dwadd(*,*)*"))
			    {
			        // dwadd(<start>,<endrange>)
			        // 32-bit	0xFFFFFFFF
			        int add_s, add_e;
			        uint32_t add = old_val;

			        line += strlen("dwadd(");
					_parse_start_end(line, pointer, dsize, &add_s, &add_e);
    			    char* read = data + add_s;
    			    
    			    while (read < data + add_e)
    			    {
						uint32_t radd = (*(uint32_t*)read);
						BE32(radd);
    			    	add += radd;
    			    	read += BSD_VAR_INT32;
    			    }

                    var->len = BSD_VAR_INT32;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) &add, var->len);
    			    
    			    LOG("[%s]:dwadd(0x%X , 0x%X) = %X", var->name, add_s, add_e, add);
			    }

				// set [*]:wadd_le(*,*)*
				else if (wildcard_match_icase(line, "wadd_le(*,*)*"))
				{
					// little-endian
					// wadd_le(<start>,<endrange>)
					// 32-bit	0xFFFFFFFF
					int add_s, add_e;
					uint32_t add = old_val;

					line += strlen("wadd_le(");
					_parse_start_end(line, pointer, dsize, &add_s, &add_e);
					char* read = data + add_s;
					
					while (read < data + add_e)
					{
						uint16_t radd = (*(uint16_t*)read);
						LE16(radd);
						add += radd;
						read += BSD_VAR_INT16;
					}

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &add, var->len);

					LOG("[%s]:wadd_le(0x%X , 0x%X) = %X", var->name, add_s, add_e, add);
				}

				// set [*]:dwadd_le(*,*)*
				else if (wildcard_match_icase(line, "dwadd_le(*,*)*"))
				{
					// little-endian
					// dwadd_le(<start>,<endrange>)
					// 32-bit	0xFFFFFFFF
					int add_s, add_e;
					uint32_t add = old_val;

					line += strlen("dwadd_le(");
					_parse_start_end(line, pointer, dsize, &add_s, &add_e);
					char* read = data + add_s;
					
					while (read < data + add_e)
					{
						uint32_t radd = (*(uint32_t*)read);
						LE32(radd);
						add += radd;
						read += BSD_VAR_INT32;
					}

					var->len = BSD_VAR_INT32;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &add, var->len);
					
					LOG("[%s]:dwadd_le(0x%X , 0x%X) = %X", var->name, add_s, add_e, add);
				}

			    // set [*]:wadd(*,*)*
			    else if (wildcard_match_icase(line, "wadd(*,*)*"))
			    {
			        // wadd(<start>,<endrange>)
			        // 16-bit	0xFFFF
			        int add_s, add_e;
			        uint32_t add = old_val;

			        line += strlen("wadd(");
			        _parse_start_end(line, pointer, dsize, &add_s, &add_e);
    			    char* read = data + add_s;
    			    
    			    while (read < data + add_e)
    			    {
						uint16_t radd = (*(uint16_t*)read);
						BE16(radd);
    			    	add += radd;
    			    	read += BSD_VAR_INT16;
    			    }
    			    
    			    while ((carry > 0) && (add > 0xFFFF))
    			    {
    			    	add = (add & 0x0000FFFF) + ((add & 0xFFFF0000) >> 8*carry);
    			    }

                    var->len = BSD_VAR_INT32 - carry;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) &add + PADDING(carry), var->len);
    			    
    			    LOG("[%s]:wadd(0x%X , 0x%X) = %X", var->name, add_s, add_e, add);
			    }

			    // set [*]:add(*,*)*
			    else if (wildcard_match_icase(line, "add(*,*)*"))
			    {
			        // add(<start>,<endrange>)
			        // 8-bit	0xFF
			        int add_s, add_e;
			        uint32_t add = old_val;

			        line += strlen("add(");
					_parse_start_end(line, pointer, dsize, &add_s, &add_e);
    			    char* read = data + add_s;
    			    
    			    while (read <= data + add_e)
    			    {
    			    	add += (*(uint8_t*)read);
    			    	read += BSD_VAR_INT8;
    			    }

    			    while ((carry > 0) && (add > 0xFFFF))
    			    {
    			    	add = (add & 0x0000FFFF) + ((add & 0xFFFF0000) >> 8*carry);
    			    }

                    var->len = BSD_VAR_INT32 - carry;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) &add + PADDING(carry), var->len);
    			    
    			    LOG("[%s]:add(0x%X , 0x%X) = %X", var->name, add_s, add_e, add);
			    }

			    // set [*]:wsub(*,*)*
			    else if (wildcard_match_icase(line, "wsub(*,*)*"))
			    {
			        // wsub(<start>,<endrange>)
			        // 16-bit	0xFFFF
					// sub()			byte		8-bit	0xFF
					// dwsub()			dbl word	32-bit	0xFFFFFFFF
					// qwsub()			quad word	64-bit	0xFFFFFFFFFFFFFFFF
			        int sub_s, sub_e;
			        uint32_t sub = old_val;

			        line += strlen("wsub(");
			        _parse_start_end(line, pointer, dsize, &sub_s, &sub_e);
    			    char* read = data + sub_s;
    			    
    			    while (read < data + sub_e)
    			    {
						uint16_t rsub = (*(uint16_t*)read);
						BE16(rsub);
    			    	sub -= rsub;
    			    	read += BSD_VAR_INT16;
    			    }

                    var->len = BSD_VAR_INT32;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) &sub, var->len);
    			    
    			    LOG("[%s]:wsub(0x%X , 0x%X) = %X", var->name, sub_s, sub_e, sub);
			    }

			    // set [*]:xor(*,*,*)*
			    else if (wildcard_match_icase(line, "xor(*,*,*)*"))
			    {
			        // xor(<start>,<endrange>,<incr>)
			        int xor_s, xor_e, xor_i;
			        uint8_t j, xor[4] = {0,0,0,0};

			        line += strlen("xor(");
    			    tmp = strchr(line, ',');
    			    *tmp = 0;
    			    
    			    xor_s = _parse_int_value(line, pointer, dsize);

			        line = tmp+1;
    			    *tmp = ',';
    			    tmp = strchr(line, ',');
    			    *tmp = 0;

    			    xor_e = _parse_int_value(line, pointer, dsize);

			        line = tmp+1;
    			    *tmp = ',';
    			    tmp = strchr(line, ')');
    			    *tmp = 0;

    			    xor_i = _parse_int_value(line, pointer, dsize);

    			    *tmp = ')';
    			    char* read = data + xor_s;
    			    
    			    while (read < data + xor_e)
    			    {
    			        for (j = 0; j < xor_i; j++)
    			            xor[j] ^= read[j];

    			        read += xor_i;
    			    }

                    var->len = BSD_VAR_INT32;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) xor, var->len);

    			    LOG("[%s]:XOR(0x%X , 0x%X, %d) = %X", var->name, xor_s, xor_e, xor_i, ((uint32_t*)var->data)[0]);
			    }

			    // set [*]:read(*,*)*
			    else if (wildcard_match_icase(line, "read(*,*)*"))
			    {
					// ;read(offset, length) is a function that reads n bytes from current file
					// set [anyname1]:read(0x100, (10)) 
					// ;0x100 is the offset in hex
					// ;(10) is the length in decimal
					// ;the example 4 reads 10 bytes starting from offset 0x100 and store them in the variable [anyname1]
					int read_s, read_l;

					line += strlen("read(");
					_parse_start_end(line, pointer, dsize, &read_s, &read_l);
					char* read = data + read_s;

					switch (read_l)
					{
					case BSD_VAR_INT8:
					case BSD_VAR_INT16:
					case BSD_VAR_INT32:
					case BSD_VAR_INT64:
					case BSD_VAR_MD5:
					case BSD_VAR_SHA256:
						var->len = read_l;
						break;

					default:
						LOG("custom read() length = %d", read_l);
						var->len = read_l;
					}

					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) read, var->len);

					switch (var->len)
					{
					case BSD_VAR_INT16:
						BE16(*((uint16_t*) var->data));
						break;
					case BSD_VAR_INT32:
						BE32(*((uint32_t*) var->data));
						break;
					case BSD_VAR_INT64:
						BE64(*((uint64_t*) var->data));
						break;
					default:
						break;
					}

					LOG("[%s]:read(0x%X , 0x%X)", var->name, read_s, read_l);
					_log_dump("read()", (uint8_t*) read, var->len);
			    }

			    // set [*]:right(*,*)*
			    else if (wildcard_match_icase(line, "right(*,*)*"))
			    {
					// right(<value>,<len>)
					int rvalue, rlen;

					line += strlen("right(");
					_parse_start_end(line, pointer, dsize, &rvalue, &rlen);

					var->len = rlen;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &rvalue + PADDING(4 - rlen), var->len);

					LOG("[%s]:right(0x%X , %d)", var->name, rvalue, rlen);
			    }

			    // set [*]:left(*)*
			    else if (wildcard_match_icase(line, "left(*,*)*"))
			    {
					// left(<value>,<len>)
					int rvalue, rlen;

					line += strlen("left(");
					_parse_start_end(line, pointer, dsize, &rvalue, &rlen);

					var->len = rlen;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*) &rvalue, var->len);

					LOG("[%s]:left(0x%X , %d)", var->name, rvalue, rlen);
			    }

				// set [*]:mid(*,*,*)*
				else if (wildcard_match_icase(line, "mid(*,*,*)*"))
				{
					// mid(<value>,<start>,<len>)
					int mid_s, mid_c, mlen;

					line += strlen("mid(");
					tmp = strchr(line, ',');
					*tmp = 0;

					char* mid_val = _decode_variable_data(line, &mlen);

					line = tmp+1;
					*tmp = ',';
					tmp = strchr(line, ',');
					*tmp = 0;

					mid_s = _parse_int_value(line, pointer, dsize);

					line = tmp+1;
					*tmp = ',';
					tmp = strchr(line, ')');
					*tmp = 0;

					mid_c = _parse_int_value(line, pointer, dsize);

					*tmp = ')';

					var->len = mid_c;
					var->data = malloc(var->len);
					memcpy(var->data, (uint8_t*)mid_val + mid_s, var->len);
					free(mid_val);

					LOG("[%s]:mid(..%d.., %X, %d)", var->name, mlen, mid_s, mid_c);
			    }

				// set [*]:host_lan_addr*
				else if (wildcard_match_icase(line, "host_lan_addr*"))
				{
					char* rval = host_callback(APOLLO_HOST_LAN_ADDR, &var->len);

					var->data = malloc(var->len);
					memcpy(var->data, rval, var->len);

					_log_dump("host_lan_addr", var->data, var->len);
				}

				// set [*]:host_wlan_addr*
				else if (wildcard_match_icase(line, "host_wlan_addr*"))
				{
					char* rval = host_callback(APOLLO_HOST_WLAN_ADDR, &var->len);

					var->data = malloc(var->len);
					memcpy(var->data, rval, var->len);

					_log_dump("host_wlan_addr", var->data, var->len);
				}

				// set [*]:host_account_id*
				else if (wildcard_match_icase(line, "host_account_id*"))
				{
					char* rval = host_callback(APOLLO_HOST_ACCOUNT_ID, &var->len);

					var->data = malloc(var->len);
					memcpy(var->data, rval, var->len);

					_log_dump("host_account_id", var->data, var->len);
				}

				// set [*]:host_psid*
				else if (wildcard_match_icase(line, "host_psid*"))
				{
					char* rval = host_callback(APOLLO_HOST_PSID, &var->len);

					var->data = malloc(var->len);
					memcpy(var->data, rval, var->len);

					_log_dump("host_psid", var->data, var->len);
				}

				// set [*]:host_username*
				else if (wildcard_match_icase(line, "host_username*"))
				{
					char* rval = host_callback(APOLLO_HOST_USERNAME, NULL);

					var->len = strlen(rval);
					var->data = malloc(var->len);
					memcpy(var->data, rval, var->len);

					_log_dump("host_username", var->data, var->len);
				}

				// set [*]:host_sysname*
				else if (wildcard_match_icase(line, "host_sysname*"))
				{
					char* rval = host_callback(APOLLO_HOST_SYS_NAME, NULL);

					var->len = strlen(rval);
					var->data = malloc(var->len);
					memcpy(var->data, rval, var->len);

					_log_dump("host_sysname", var->data, var->len);
				}

			    // set [*]:* (e.g. 0x00000000)
			    else
			    {
			        uint32_t tval;
                    sscanf(line, "%" PRIx32, &tval);

                    var->len = BSD_VAR_INT32;
                    var->data = malloc(var->len);
                    memcpy(var->data, (uint8_t*) &tval, var->len);

    			    LOG("[%s]:%s = %X", var->name, line, tval);
			    }
			        
			}

		}

        // write *:*
		else if (wildcard_match_icase(line, "write *:*"))
		{
			// write next / write at
			//
			// write at (67): 446966666963756C7479
			//
			// ; (67) = 0x43
			// ;Addresses enclosed in parenthesis are treated as decimal.
			//
			// The following 3 lines are equivalent:
			// write at (67): 446966666963756C7479
			// write at 43: "Difficulty"
			// write at 0x43: "Difficulty"
			//
			// write next 0: 446966666963756C7479
			// ;Overwrites 0 bytes after the file pointer address.
			//
			// write next 0: "Difficulty"
			// ;Overwrites with the text at 0 bytes after the found offset in this case "next 0" is at the found offset.
			// ;"next (10)" or "next A" would write 10 bytes after the found offset.
			//
			// write at 0x100:[anyname1]
			// ;Overwrites the content of the variable [anyname1] starting at offset 0x100.
   			int off, wlen;
			uint8_t from_pointer = 0;
			char* tmp = NULL;
			char* write_val = NULL;
			
			line += strlen("write");
		    skip_spaces(line);

			if (wildcard_match_icase(line, "at*"))
			{
			    from_pointer = 0;
			    line += strlen("at");
			}
			else if (wildcard_match_icase(line, "next*"))
			{
			    from_pointer = 1;
			    line += strlen("next");
			}
			else
			{
				// invalid command
				LOG("ERROR: Invalid write command");
				dsize = 0;
				goto bsd_end;
			}

		    skip_spaces(line);

		    tmp = strchr(line, ':');
		    *tmp = 0;

//		    sscanf(line, wildcard_match(line, "(*)") ? "(%d)" : "%x", &off);
			if (wildcard_match(line, "(*)"))
			    sscanf(line, "(%d)", &off);
			else
			    sscanf(line, "%x", &off);

			off += (from_pointer ? pointer : 0);

			line = tmp+1;
			*tmp = ':';

		    skip_spaces(line);

		    // write at/next *:xor:*
			if (wildcard_match_icase(line, "xor:*"))
			{
			    line += strlen("xor:");
    		    skip_spaces(line);

			    write_val = _decode_variable_data(line, &wlen);
			    
			    for (int i=0; i < wlen; i++)
			        write_val[i] ^= data[off + i];

			    LOG(":xor:%s", line);
			}

			// write at/next *:repeat(*,*)*
			else if (wildcard_match_icase(line, "repeat(*,*)*"))
			{
				// repeat(<count>,<value>)
				int r_cnt, j;
				char* r_val;

				line += strlen("repeat(");
				tmp = strchr(line, ',');
				*tmp = 0;
				
				r_cnt = _parse_int_value(line, pointer, dsize);

				line = tmp+1;
				*tmp = ',';
				tmp = strchr(line, ')');
				*tmp = 0;

				r_val = _decode_variable_data(line, &wlen);

				*tmp = ')';
				
				write_val = malloc(r_cnt * wlen);
				
				for(j = 0; j < r_cnt; j++)
					memcpy(write_val + (j * wlen), r_val, wlen);

				free(r_val);
				wlen = r_cnt * wlen;

				LOG(":repeat(0x%X , %s)", r_cnt, line);
			}

		    // write at/next *:[*]
			else if (wildcard_match(line, "[*]"))
			{
			    LOG("Getting value for %s", line);
			    write_val = _decode_variable_data(line, &wlen);
			}

		    // write at/next *:* (e.g. 0x00000000)
			else
			{
			    write_val = _decode_variable_data(line, &wlen);
			}

			if (!write_val)
			{
				// no data to write
				LOG("ERROR: No data to write");
				dsize = 0;
				goto bsd_end;
			}

//			for (int i=0; i < wlen; i++)
//				LOG("%x", write_val[i]);

			char* write = data + off;
			memcpy(write, write_val, wlen);
			free(write_val);

            LOG("Wrote %d bytes (%s) to 0x%X", wlen, line, off);
		}

		// insert *:*
		else if (wildcard_match_icase(line, "insert *:*"))
		{
			// insert data
			// insert next / insert at
   			int off, ilen;
			uint8_t from_pointer = 0;
			char* tmp = NULL;
			
			line += strlen("insert");
		    skip_spaces(line);

			if (wildcard_match_icase(line, "at*"))
			{
			    from_pointer = 0;
			    line += strlen("at");
			}
			else if (wildcard_match_icase(line, "next*"))
			{
			    from_pointer = 1;
			    line += strlen("next");
			}
			else
			{
				// invalid command
				LOG("ERROR: Invalid insert command");
				dsize = 0;
				goto bsd_end;
			}

			skip_spaces(line);

			tmp = strchr(line, ':');
			*tmp = 0;

			if (wildcard_match(line, "(*)"))
				sscanf(line, "(%d)", &off);
			else
				sscanf(line, "%x", &off);

			off += (from_pointer ? pointer : 0);

			line = tmp+1;
			*tmp = ':';

			skip_spaces(line);

			char* idata = _decode_variable_data(line, &ilen);
			
			if (!idata)
			{
				LOG("Error: no data to insert");
				dsize = 0;
				goto bsd_end;
			}

			char* write = malloc(dsize + ilen);

			memcpy(write, data, off);
			memcpy(write + off, idata, ilen);
			memcpy(write + off + ilen, data + off, dsize - off);

			free(idata);
			free(data);

			data = write;
			dsize += ilen;

            LOG("Inserted %d bytes (%s) from 0x%X to 0x%X", ilen, line, off, off + ilen);
		}

		// delete *:*
		else if (wildcard_match_icase(line, "delete *:*"))
		{
			// delete data
			// delete next / delete at
   			int off, dlen;
			uint8_t from_pointer = 0;
			char* tmp = NULL;
			
			line += strlen("delete");
		    skip_spaces(line);

			if (wildcard_match_icase(line, "at*"))
			{
			    from_pointer = 0;
			    line += strlen("at");
			}
			else if (wildcard_match_icase(line, "next*"))
			{
			    from_pointer = 1;
			    line += strlen("next");
			}
			else
			{
				// invalid command
				LOG("ERROR: Invalid delete command");
				dsize = 0;
				goto bsd_end;
			}

		    skip_spaces(line);

		    tmp = strchr(line, ':');
		    *tmp = 0;

			if (wildcard_match(line, "(*)"))
			    sscanf(line, "(%d)", &off);
			else
			    sscanf(line, "%x", &off);

			off += (from_pointer ? pointer : 0);

			line = tmp+1;
			*tmp = ':';

		    skip_spaces(line);

			// delete at/next *:until*
			if (wildcard_match_icase(line, "until*"))
			{
			    line += strlen("until");
    		    skip_spaces(line);

				int flen;
			    char* find = _decode_variable_data(line, &flen);
			    
			    if (!find)
			    {
					LOG("Error: no data to search {%s}", line);
					dsize = 0;
					goto bsd_end;
			    }
			    
			    dlen = search_data(data, dsize, off, find, flen, 1) - off;
			    free(find);
			}
			else
			{
			    dlen = _parse_int_value(line, pointer, dsize);
				if (dlen + off > (long)dsize)
					dlen = dsize - off;
			}

			dsize -= dlen;
			memmove(data + off, data + off + dlen, dsize - off);

            LOG("Deleted %d bytes (%s) from 0x%X to 0x%X", dlen, line, off, off + dlen);
		}

        // search *
		else if (wildcard_match_icase(line, "search *"))
		{
			// search "difficulty"
			// ;Searches for the word "difficulty" as text.
			//
			// search 646966666963756C7479:1
			// ;":1" means repeat search 1 time (default). ":3" at the end would set the pointer to the 3rd occurrence of the searched text
			//
			// search next 0x010e
			// ; Start search from current pointer

			int cnt = 1, len, off = 0;
			char* find;
			char* tmp = NULL;

			line += strlen("search");
			skip_spaces(line);

			if (wildcard_match(line, "next*"))
			{
				line += strlen("next");
				skip_spaces(line);
				off = pointer;
			}

			if (wildcard_match(line, "*:*"))
			{
			    tmp = strrchr(line, ':');
			    sscanf(tmp+1, "%d", &cnt);
			    *tmp = 0;
			}

			find = _decode_variable_data(line, &len);

			if (tmp)
				*tmp = ':';

		    if (!find)
		    {
		        // error decoding
				LOG("Error parsing search pattern! {%s}", line);
				dsize = 0;
				goto bsd_end;
		    }

			LOG("Searching {%s} ...", line);
			pointer = search_data(data, dsize, off, find, len, cnt);
			
			if (pointer < 0)
			{
				LOG("ERROR: SEARCH PATTERN NOT FOUND");
				free(find);
				dsize = 0;
				goto bsd_end;
			}
			
			LOG("POINTER = %ld (0x%lX)", pointer, pointer);
			free(find);
		}

		else if (wildcard_match_icase(line, "copy *:*:*"))
		{
			// copy <from address>:<to address>:<size>
			// copy data

			int from = 0, len, off = 0;
			char* tmp = NULL;

			line += strlen("copy");
			skip_spaces(line);

			tmp = strchr(line, ':');
			*tmp = 0;
			from = _parse_int_value(line, pointer, dsize);
			*tmp++ = ':';
			line = tmp;

			tmp = strchr(line, ':');
			*tmp = 0;
			off = _parse_int_value(line, pointer, dsize);
			*tmp++ = ':';
			line = tmp;

			len = _parse_int_value(line, pointer, dsize);
			memmove(data + off, data + from, len);

			LOG("Copied %d bytes from 0x%X to 0x%X", len, from, off);
		}

		else if (wildcard_match_icase(line, "msgbox [*]*"))
		{
			int len;
			char* buf;

			line += strlen("msgbox");
			skip_spaces(line);

			buf = _decode_variable_data(line, &len);
			_log_dump(line, (uint8_t*) buf, len);
			free(buf);
		}

		else if (wildcard_match_icase(line, "endian_swap(*)*"))
		{
			int mode;
			char *tmp;
			uint8_t* start = (uint8_t*)data + range_start;

			line += strlen("endian_swap(");
			tmp = strrchr(line, ')');
			*tmp = 0;

			mode = _parse_int_value(line, pointer, dsize);
			*tmp = ')';

			switch (mode)
			{
			case 2:
				swap_u16_data((uint16_t*) start, (range_end - range_start)/2);
				break;

			case 4:
				swap_u32_data((uint32_t*) start, (range_end - range_start)/4);
				break;

			case 8:
				swap_u64_data((uint64_t*) start, (range_end - range_start)/8);
				break;

			default:
				LOG("ERROR: unsupported swap length (%d)", mode);
				dsize = 0;
				goto bsd_end;
			}
			LOG("Endian Swap (%d bits) data 0x%X..0x%X", mode*8, range_start, range_end);
		}

		else if (wildcard_match_icase(line, "decrypt *"))
		{
			line += strlen("decrypt");
			skip_spaces(line);

			if (wildcard_match_icase(line, "diablo3*"))
			{
				LOG("Decrypt Diablo 3 data");
				diablo_decrypt_data((uint8_t*) data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "dw8xl*"))
			{
				LOG("Decrypt Dynasty Warriors 8 XL data");
				dw8xl_encode_data((uint8_t*)data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "silent_hill3*"))
			{
				LOG("Decrypt Silent Hill 3 data");
				sh3_decrypt_data((uint8_t*)data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "nfs_undercover*"))
			{
				LOG("Decrypt NFS Undercover data");
				nfsu_decrypt_data((uint8_t*)data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "camellia_ecb(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("camellia_ecb(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				camellia_ecb_decrypt(start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "ffxiii(*,*)*"))
			{
				int key_len, mode;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("ffxiii(");
				tmp = strrchr(line, ',');
				*tmp = 0;

				mode = _parse_int_value(line, pointer, dsize);
				*tmp = ',';

				line = tmp + 1;
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("FFXIII Type=%d Encryption Key=%s", mode, line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				ff13_decrypt_data(mode, start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "rgg_studio(*)*"))
			{
				int klen;
				char *tmp, *rggkey;

				line += strlen("rgg_studio(");
				tmp = strrchr(line, ')');
				*tmp = 0;
				LOG("RGG Key=%s", line);

				rggkey = _decode_variable_data(line, &klen);
				*tmp = ')';

				rgg_xor_data((uint8_t*)data + range_start, (range_end - range_start), rggkey, klen);
				free(rggkey);
			}
			else if (wildcard_match_icase(line, "borderlands3(*)*"))
			{
				int s_type;
				char *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("borderlands3(");
				tmp = strrchr(line, ')');
				*tmp = 0;
				LOG("Borderlands 3 Save Type=%s", line);

				s_type = _parse_int_value(line, pointer, dsize);
				*tmp = ')';

				borderlands3_Decrypt(start, (range_end - range_start), s_type);
			}
			else if (wildcard_match_icase(line, "monster_hunter(*)*"))
			{
				int type;
				char *tmp;

				line += strlen("monster_hunter(");
				tmp = strrchr(line, ')');
				*tmp = 0;
				LOG("Monster Hunter PSP Save Type=%s", line);

				type = _parse_int_value(line, pointer, dsize);
				*tmp = ')';

				monsterhunter_decrypt_data((uint8_t*)data + range_start, (range_end - range_start), type);
			}
			else if (wildcard_match_icase(line, "mgs5_tpp(*)*"))
			{
				int xor_key;
				char *tmp;

				line += strlen("mgs5_tpp(");
				tmp = strrchr(line, ')');
				*tmp = 0;
				LOG("MGS 5 Key=%s", line);

				xor_key = _parse_int_value(line, pointer, dsize);
				*tmp = ')';

				mgs5tpp_encode_data((uint32_t*)(data + range_start), (range_end - range_start), xor_key);
			}
			else if (wildcard_match_icase(line, "mgs_pw*"))
			{
				LOG("Decrypt MGS Peace Walker data");
				mgspw_Decrypt((uint32_t*)(data + range_start), (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "mgs_base64*"))
			{
				LOG("Decode MGS Base64 data");
				mgs_DecodeBase64((uint8_t*)data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "mgs(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("mgs(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("MGS HD Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				mgs_Decrypt(start, (range_end - range_start), key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "aes_ecb(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("aes_ecb(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				aes_ecb_decrypt(start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "des_ecb(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("des_ecb(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				des_ecb_decrypt(start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "des3_cbc(*,*)*"))
			{
				int key_len, iv_len;
				char *key, *iv, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("des3_cbc(");
				tmp = strrchr(line, ',');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ',';

				line = tmp + 1;
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption IV=%s", line);

				iv = _decode_variable_data(line, &iv_len);
				*tmp = ')';

				des3_cbc_decrypt(start, (range_end - range_start), (uint8_t*) key, key_len, (uint8_t*) iv, iv_len);
				free(key);
				free(iv);
			}
			else if (wildcard_match_icase(line, "blowfish_ecb(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("blowfish_ecb(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				blowfish_ecb_decrypt(start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}

		}

		else if (wildcard_match_icase(line, "encrypt *"))
		{
			line += strlen("encrypt");
			skip_spaces(line);

			if (wildcard_match_icase(line, "diablo3*"))
			{
				LOG("Encrypt Diablo 3 data");
				diablo_encrypt_data((uint8_t*) data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "dw8xl*"))
			{
				LOG("Encrypt Dynasty Warriors 8 XL data");
				dw8xl_encode_data((uint8_t*)data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "silent_hill3*"))
			{
				LOG("Encrypt Silent Hill 3 data");
				sh3_encrypt_data((uint8_t*)data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "nfs_undercover*"))
			{
				LOG("Encrypt NFS Undercover data");
				nfsu_encrypt_data((uint8_t*)data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "camellia_ecb(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("camellia_ecb(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				camellia_ecb_encrypt(start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "ffxiii(*,*)*"))
			{
				int key_len, mode;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("ffxiii(");
				tmp = strrchr(line, ',');
				*tmp = 0;

				mode = _parse_int_value(line, pointer, dsize);
				*tmp = ',';

				line = tmp + 1;
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("FFXIII Type=%d Encryption Key=%s", mode, line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				ff13_encrypt_data(mode, start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "rgg_studio(*)*"))
			{
				int klen;
				char *tmp, *rggkey;

				line += strlen("rgg_studio(");
				tmp = strrchr(line, ')');
				*tmp = 0;
				LOG("RGG Key=%s", line);

				rggkey = _decode_variable_data(line, &klen);
				*tmp = ')';

				rgg_xor_data((uint8_t*)data + range_start, (range_end - range_start), rggkey, klen);
				free(rggkey);
			}
			else if (wildcard_match_icase(line, "borderlands3(*)*"))
			{
				int s_type;
				char *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("borderlands3(");
				tmp = strrchr(line, ')');
				*tmp = 0;
				LOG("Borderlands 3 Save Type=%s", line);

				s_type = _parse_int_value(line, pointer, dsize);
				*tmp = ')';

				borderlands3_Encrypt(start, (range_end - range_start), s_type);
			}
			else if (wildcard_match_icase(line, "monster_hunter(*)*"))
			{
				int type;
				char *tmp;

				line += strlen("monster_hunter(");
				tmp = strrchr(line, ')');
				*tmp = 0;
				LOG("Monster Hunter PSP Save Type=%s", line);

				type = _parse_int_value(line, pointer, dsize);
				*tmp = ')';

				monsterhunter_encrypt_data((uint8_t*)data + range_start, (range_end - range_start), type);
			}
			else if (wildcard_match_icase(line, "mgs5_tpp(*)*"))
			{
				int xor_key;
				char *tmp;

				line += strlen("mgs5_tpp(");
				tmp = strrchr(line, ')');
				*tmp = 0;
				LOG("MGS 5 Key=%s", line);

				xor_key = _parse_int_value(line, pointer, dsize);
				*tmp = ')';

				mgs5tpp_encode_data((uint32_t*)(data + range_start), (range_end - range_start), xor_key);
			}
			else if (wildcard_match_icase(line, "mgs_pw*"))
			{
				LOG("Encrypt MGS Peace Walker data");
				mgspw_Encrypt((uint32_t*)(data + range_start), (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "mgs_base64*"))
			{
				LOG("Encode MGS Base64 data");
				mgs_EncodeBase64((uint8_t*)data + range_start, (range_end - range_start));
			}
			else if (wildcard_match_icase(line, "mgs(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("mgs(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("MGS HD Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				mgs_Encrypt(start, (range_end - range_start), key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "aes_ecb(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("aes_ecb(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				aes_ecb_encrypt(start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "des_ecb(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("des_ecb(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				des_ecb_encrypt(start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}
			else if (wildcard_match_icase(line, "des3_cbc(*,*)*"))
			{
				int key_len, iv_len;
				char *key, *iv, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("des3_cbc(");
				tmp = strrchr(line, ',');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ',';

				line = tmp + 1;
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption IV=%s", line);

				iv = _decode_variable_data(line, &iv_len);
				*tmp = ')';

				des3_cbc_encrypt(start, (range_end - range_start), (uint8_t*) key, key_len, (uint8_t*) iv, iv_len);
				free(key);
				free(iv);
			}
			else if (wildcard_match_icase(line, "blowfish_ecb(*)*"))
			{
				int key_len;
				char *key, *tmp;
				uint8_t* start = (uint8_t*)data + range_start;

				line += strlen("blowfish_ecb(");
				tmp = strrchr(line, ')');
				*tmp = 0;

				LOG("Encryption Key=%s", line);

				key = _decode_variable_data(line, &key_len);
				*tmp = ')';

				blowfish_ecb_encrypt(start, (range_end - range_start), (uint8_t*) key, key_len);
				free(key);
			}

		}
    }

	write_buffer(filepath, (uint8_t*) data, dsize);

bsd_end:
	free(data);
	free(bsd_code);

	return (dsize);
}

int apply_ggenie_patch_code(const char* filepath, const code_entry_t* code)
{
	char *data, *gg_code;
	size_t dsize;
	long pointer = 0, end_pointer = 0;
	uint32_t ptr_value = 0;
	char tmp3[4], tmp4[5], tmp6[7], tmp8[9];

	LOG("Applying [%s] to '%s'...", code->name, filepath);
	if (read_buffer(filepath, (uint8_t**) &data, &dsize) != SUCCESS)
	{
		LOG("Can't load file '%s'", filepath);
		return 0;
	}

	gg_code = strdup(code->codes);
	for (char *line = strtok(gg_code, "\n"); line != NULL; line = strtok(NULL, "\n"))
	{
    	switch (line[0])
    	{
    		case '0':
    			//	8-bit write
    			//	0TXXXXXX 000000YY
    		case '1':
    			//	16-bit write
    			//	1TXXXXXX 0000YYYY
    		case '2':
    			//	32-bit write
    			//	2TXXXXXX YYYYYYYY
    			//	X= Address/Offset
    			//	Y= Value to write
    			//	T=Address/Offset type (0 = Normal / 8 = Offset From Pointer)
    		{
    			int off;
    			uint32_t val;
    			uint8_t bytes = 1 << (line[0] - 0x30);

    			sprintf(tmp6, "%.6s", line+2);
    			sscanf(tmp6, "%x", &off);
    			off += (line[1] == '8' ? pointer : 0);

    			sprintf(tmp8, "%.8s", line+9);
    			sscanf(tmp8, "%" PRIx32, &val);
				MEM32(val);

    			memcpy(data + off, (char*) &val + PADDING(4 - bytes), bytes);

    			LOG("Wrote %d bytes (%s) to 0x%X", bytes, tmp8 + (8 - bytes*2), off);
    		}
    			break;

    		case '3':
				//	Increase / Decrease Write
				//	Increases or Decreases a specified amount of data from a specific Address
				//	This does not add/remove Bytes into the save, it just adjusts the value of the Bytes already in it
				//	For the 8 Byte Value Type, it will write 4 Bytes of data but will continue to write the bytes afterwards if it cannot write any more.
				//	3BYYYYYY XXXXXXXX
				//	B = Byte Value & Offset Type
				//	0 = Add 1 Byte  (000000XX)
				//	1 = Add 2 Bytes (0000XXXX)
				//	2 = Add 4 Bytes
				//	3 = Add 8 Bytes
				//	4 = Sub 1 Byte  (000000XX)
				//	5 = Sub 2 Bytes (0000XXXX)
				//	6 = Sub 4 Bytes
				//	7 = Sub 8 Bytes
				//	8 = Offset from Pointer; Add 1 Byte  (000000XX)
				//	9 = Offset from Pointer; Add 2 Bytes (0000XXXX)
				//	A = Offset from Pointer; Add 4 Bytes
				//	B = Offset from Pointer; Add 8 Bytes
				//	C = Offset from Pointer; Sub 1 Byte  (000000XX)
				//	D = Offset from Pointer; Sub 2 Bytes (0000XXXX)
				//	E = Offset from Pointer; Sub 4 Bytes
				//	F = Offset from Pointer; Sub 8 Bytes
				//	Y = Address
				//	X = Bytes to Add/Sub
			{
				int off;
				uint32_t val, wv32;
				uint16_t wv16;
				uint8_t wv8;
				char t = line[1];

				sprintf(tmp6, "%.6s", line+2);
				sscanf(tmp6, "%x", &off);
				off += ((t == '8' || t == '9' || t == 'A' || t == 'C' || t == 'D' || t == 'E') ? pointer : 0);

				sprintf(tmp8, "%.8s", line+9);
				sscanf(tmp8, "%" PRIx32, &val);

				char* write = data + off;

				switch (t)
				{
					case '0':
					case '8':
						wv8 = (uint8_t) write[0];
						wv8 += (val & 0x000000FF);
						memcpy(write, &wv8, 1);
						LOG("Add-Write 1 byte (%02X) to 0x%X", val, off);
						break;

					case '1':
					case '9':
						wv16 = ((uint16_t*) write)[0];
						MEM16(wv16);
						wv16 += (val & 0x0000FFFF);
						MEM16(wv16);
						memcpy(write, &wv16, 2);
						LOG("Add-Write 2 bytes (%04X) to 0x%X", val, off);
						break;

					case '2':
					case 'A':
						wv32 = ((uint32_t*) write)[0];
						MEM32(wv32);
						wv32 += val;
						MEM32(wv32);
						memcpy(write, &wv32, 4);
						LOG("Add-Write 4 bytes (%08X) to 0x%X", val, off);
						break;

					case '3':
					case 'B':
						LOG("Not Implemented! Add-Wrote 8 bytes (%08X) to 0x%X", val, off);
						break;

					case '4':
					case 'C':
						wv8 = (uint8_t) write[0];
						wv8 -= (val & 0x000000FF);
						memcpy(write, &wv8, 1);
						LOG("Sub-Write 1 byte (%02X) to 0x%X", val, off);
						break;

					case '5':
					case 'D':
						wv16 = ((uint16_t*) write)[0];
						MEM16(wv16);
						wv16 -= (val & 0x0000FFFF);
						MEM16(wv16);
						memcpy(write, &wv16, 2);
						LOG("Sub-Write 2 bytes (%04X) to 0x%X", val, off);
						break;

					case '6':
					case 'E':
						wv32 = ((uint32_t*) write)[0];
						MEM32(wv32);
						wv32 -= val;
						MEM32(wv32);
						memcpy(write, &wv32, 4);
						LOG("Sub-Write 4 bytes (%08X) to 0x%X", val, off);
						break;

					case '7':
					case 'F':
						LOG("Not Implemented! Sub-Write 8 bytes (%08X) to 0x%X", val, off);
						break;
				}
    		}
    			break;

    		case '4':
    			//	multi write
    			//	4TXXXXXX YYYYYYYY
    			//	4NNNWWWW VVVVVVVV
    			//	X= Address/Offset
    			//	Y= Value to write (Starting)
    			//	N=Times to Write
    			//	W=Increase Address By
    			//	V=Increase Value By
    			//	T=Address/Offset type
    			//	Normal/Pointer
    			//	0 / 8 = 8bit
    			//	1 / 9 = 16bit
    			//	2 / A = 32bit
    		{
    			int i, off, n, incoff;
    			uint32_t val, incval, wv32;
    			char t = line[1];
    			char* write;
				uint8_t wv8;
				uint16_t wv16;

    			sprintf(tmp6, "%.6s", line+2);
    			sscanf(tmp6, "%x", &off);
    			off += ((t == '8' || t == '9' || t == 'A') ? pointer : 0);

    			sprintf(tmp8, "%.8s", line+9);
    			sscanf(tmp8, "%" PRIx32, &val);

			    line = strtok(NULL, "\n");

    			sprintf(tmp3, "%.3s", line+1);
    			sscanf(tmp3, "%x", &n);

    			sprintf(tmp4, "%.4s", line+4);
    			sscanf(tmp4, "%x", &incoff);

    			sprintf(tmp8, "%.8s", line+9);
    			sscanf(tmp8, "%" PRIx32, &incval);
			
				LOG("Multi-write at (0x%X) %d times, inc-addr (%d) inc-val (%X)", off, n, incoff, incval);

				for (i = 0; i < n; i++)
				{
	    			write = data + off + (incoff * i);

					switch (t)
					{
						case '0':
						case '8':
							wv8 = val;
							memcpy(write, &wv8, 1);
							LOG("M-Wrote 1 byte (%02X) to 0x%lX", val, write - data);
							break;

						case '1':
						case '9':
							wv16 = val;
							MEM16(wv16);
							memcpy(write, &wv16, 2);
							LOG("M-Wrote 2 bytes (%04X) to 0x%lX", val, write - data);
							break;

						case '2':
						case 'A':
							wv32 = val;
							MEM32(wv32);
							memcpy(write, &wv32, 4);
							LOG("M-Wrote 4 bytes (%08X) to 0x%lX", val, write - data);
							break;
					}

	    			val += incval;
				}
    		}
    			break;

    		case '5':
    			//	copy bytes
    			//	5TXXXXXX ZZZZZZZZ
    			//	5TYYYYYY 00000000
    			//	  XXXXXX = Offset to copy from
    			//	  YYYYYY = Offset to copy to
    			//	         ZZZZZZZZ = Number of bytes to copy
    			//	 T = Bit Size
    			//	 0 = From start of the data
    			//	 8 = From found from a search
    		{
    			int off_src, off_dst;
    			uint32_t val;

    			sprintf(tmp6, "%.6s", line+2);
    			sscanf(tmp6, "%x", &off_src);

    			sprintf(tmp8, "%.8s", line+9);
    			sscanf(tmp8, "%" PRIx32, &val);

    			char* src = data + off_src + (line[1] == '8' ? pointer : 0);

			    line = strtok(NULL, "\n");

    			sprintf(tmp6, "%.6s", line+2);
    			sscanf(tmp6, "%x", &off_dst);
    			
    			char* dst = data + off_dst + (line[1] == '8' ? pointer : 0);

    			memcpy(dst, src, val);
				LOG("Copied %d bytes from 0x%lX to 0x%lX", val, src - data, dst - data);
    		}
    			break;

    		case '6':
    			//	special mega code
    			//	6TWX0Y0Z VVVVVVVV <- Code Type 6
    			//	6 = Type 6: Pointer codes
    			//	T = Data size of VVVVVVVV: 0:8bit, 1:16bit, 2:32bit, search-> 8:8bit, 9:16bit, A:32bit
    			//	W = operator:
    			//	      0X = Read "address" from file (X = 0:none, 1:add, 2:multiply)
    			//	      1X = Move pointer from obtained address ?? (X = 0:add, 1:substract, 2:multiply)
    			//	      2X = Move pointer ?? (X = 0:add, 1:substract, 2:multiply)
    			//	      4X = Write value: X=0 at read address, X=1 at pointer address
    			//	Y = flag relative to read add (very tricky to understand; 0=absolute, 1=pointer)
    			//	Z = flag relative to current pointer (very tricky to understand)
    			//	V = Data
			{
				char t = line[1];
				char w = line[2];
				char x = line[3];
				char y = line[5];
				char z = line[7];
				uint32_t val, wv32;
				uint16_t wv16;
				uint8_t wv8;

				sprintf(tmp8, "%.8s", line+9);
				sscanf(tmp8, "%" PRIx32, &val);

				char* write = data;
				int off = ((t == '8' || t == '9' || t == 'A') ? pointer : 0);

				switch (w)
				{
				case '0':
					// 0X = Read "address" from file (X = 0:none, 1:add, 2:multiply)
					val += (x == '1' ? ptr_value : 0);
					write += (val + off);

					if (y == '1')
						pointer = val;

					switch (t)
					{
					case '0':
					case '8':
						// Data size = 8 bits
						// 000000VV
						ptr_value = (uint8_t) write[0];
						break;

					case '1':
					case '9':
						// Data size = 16 bits
						// 0000VVVV
						wv16 = ((uint16_t*) write)[0];
						MEM16(wv16);
						ptr_value = wv16;
						break;

					case '2':
					case 'A':
						// Data size = 32 bits
						// VVVVVVVV
						wv32 = ((uint32_t*) write)[0];
						MEM32(wv32);
						ptr_value = wv32;
						break;
					}

					LOG("Read address (%X) = %X", val, ptr_value);
					break;

				case '1':
					// 1X = Move pointer from obtained address ?? (X = 0:add, 1:substract, 2:multiply)
					switch (x)
					{
					case '0':
						// 0:add
						ptr_value += val;
						break;

					case '1':
						// 1:substract
						ptr_value -= val;
						break;

					case '2':
						// 2:multiply
						ptr_value *= val;
						break;
					}

					ptr_value += (z == '1' ? pointer : 0);
					pointer = ptr_value;

					LOG("Pointer %ld (0x%lX) [val=%x]", pointer, pointer, val);
					break;

				case '2':
					// 2X = Move pointer ?? (X = 0:add, 1:substract, 2:multiply)
					switch (x)
					{
					case '0':
						// 0:add
						pointer += val;
						break;

					case '1':
						// 1:substract
						pointer -= val;
						break;

					case '2':
						// 2:multiply
						pointer *= val;
						break;
					}

					if (y == '1')
						ptr_value = pointer;

					LOG("Pointer %ld (0x%lX) [val=%x]", pointer, pointer, val);
					break;

				case '4':
					// 4X = Write value: X=0 at read address, X=1 at pointer address
					write += pointer;

					switch (t)
					{
						case '0':
						case '8':
							wv8 = val;
							memcpy(write, &wv8, 1);
							LOG("6-Wrote 1 byte (%02X) to 0x%lX", val, pointer);
							break;

						case '1':
						case '9':
							wv16 = val;
							MEM16(wv16);
							memcpy(write, &wv16, 2);
							LOG("6-Wrote 2 bytes (%04X) to 0x%lX", val, pointer);
							break;

						case '2':
						case 'A':
							wv32 = val;
							MEM32(wv32);
							memcpy(write, &wv32, 4);
							LOG("6-Wrote 4 bytes (%08X) to 0x%lX", val, pointer);
							break;
					}
					break;
				}

			}
				break;

    		case '7':
				//	Writes Bytes up to a specified Maximum/Minimum to a specific Address
				//	This code is the same as a standard write code however it will only write the bytes if the current value at the address is no more or no less than X.
				//	For example, you can use a no less than value to make sure the address has more than X but will take no effect if it already has more than the value on the save.
				//	7BYYYYYY XXXXXXXX
				//	B = Byte Value & Offset Type
				//	0 = No Less Than: 1 Byte  (000000XX)
				//	1 = No Less Than: 2 Bytes (0000XXXX)
				//	2 = No Less Than: 4 Bytes
				//	4 = No More Than: 1 Byte  (000000XX)
				//	5 = No More Than: 2 Bytes (0000XXXX)
				//	6 = No More Than: 4 Bytes
				//	8 = Offset from Pointer; No Less Than: 1 Byte  (000000XX)
				//	9 = Offset from Pointer; No Less Than: 2 Bytes (0000XXXX)
				//	A = Offset from Pointer; No Less Than: 4 Bytes
				//	C = Offset from Pointer; No More Than: 1 Byte  (000000XX)
				//	D = Offset from Pointer; No More Than: 2 Bytes (0000XXXX)
				//	E = Offset from Pointer; No More Than: 4 Bytes
				//	Y = Address
				//	X = Bytes to Write
    		{
    			int off;
				uint32_t val, wv32;
				uint16_t wv16;
				uint8_t wv8;
    			char t = line[1];

    			sprintf(tmp6, "%.6s", line+2);
    			sscanf(tmp6, "%x", &off);
				off += ((t == '8' || t == '9' || t == 'A' || t == 'C' || t == 'D' || t == 'E') ? pointer : 0);

    			sprintf(tmp8, "%.8s", line+9);
    			sscanf(tmp8, "%" PRIx32, &val);

    			char* write = data + off;

				switch (t)
				{
					case '0':
					case '8':
						val &= 0x000000FF;
						wv8 = (uint8_t) write[0];
						if (val > wv8) wv8 = val;
						memcpy(write, &wv8, 1);
						LOG("nlt-Wrote 1 byte (%02X) to 0x%X", val, off);
						break;

					case '1':
					case '9':
						val &= 0x0000FFFF;
						wv16 = ((uint16_t*) write)[0];
						MEM16(wv16);
						if (val > wv16) wv16 = val;
						MEM16(wv16);
						memcpy(write, &wv16, 2);
						LOG("nlt-Wrote 2 bytes (%04X) to 0x%X", val, off);
						break;

					case '2':
					case 'A':
						wv32 = ((uint32_t*) write)[0];
						MEM32(wv32);
						if (val > wv32) wv32 = val;
						MEM32(wv32);
						memcpy(write, &wv32, 4);
						LOG("nlt-Wrote 4 bytes (%08X) to 0x%X", val, off);
						break;

					case '4':
					case 'C':
						val &= 0x000000FF;
						wv8 = (uint8_t) write[0];
						if (val < wv8) wv8 = val;
						memcpy(write, &wv8, 1);
						LOG("nmt-Wrote 1 byte (%02X) to 0x%X", val, off);
						break;

					case '5':
					case 'D':
						val &= 0x0000FFFF;
						wv16 = ((uint16_t*) write)[0];
						MEM16(wv16);
						if (val < wv16) wv16 = val;
						MEM16(wv16);
						memcpy(write, &wv16, 2);
						LOG("nmt-Wrote 2 bytes (%04X) to 0x%X", val, off);
						break;

					case '6':
					case 'E':
						wv32 = ((uint32_t*) write)[0];
						MEM32(wv32);
						if (val < wv32) wv32 = val;
						MEM32(wv32);
						memcpy(write, &wv32, 4);
						LOG("nmt-Wrote 4 bytes (%08X) to 0x%X", val, off);
						break;
				}
    		}
    			break;

    		case '8':
    			//	Search Type
    			//	8TZZXXXX YYYYYYYY
    			//	T= Address/Offset type (0 = Normal / 8 = Offset From Pointer)
    			//	Z= Amount of times to find before Write
    			//	X= Amount of data to Match
    			//	Y= Seach For (note can be extended for more just continue it like YYYYYYYY YYYYYYYY under it)
    			//	Once u have your Search type done then place one of the standard code types under it with setting T to the Pointer type
    		{
    			int i, cnt, len;
    			uint32_t val;
    			char* find;
    			char t = line[1];

    			sprintf(tmp3, "%.2s", line+2);
    			sscanf(tmp3, "%x", &cnt);

    			sprintf(tmp4, "%.4s", line+4);
    			sscanf(tmp4, "%x", &len);

    			sprintf(tmp8, "%.8s", line+9);
    			sscanf(tmp8, "%" PRIx32, &val);
				BE32(val);

    			find = malloc((len+3) & ~3);
    			if (!cnt) cnt = 1;

				memcpy(find, (char*) &val, 4);
    			
    			for (i=4; i < len; i += 8)
    			{
				    line = strtok(NULL, "\n");

					sprintf(tmp8, "%.8s", line);
	    			sscanf(tmp8, "%" PRIx32, &val);
					BE32(val);

					memcpy(find + i, (char*) &val, 4);

					sprintf(tmp8, "%.8s", line+9);
	    			sscanf(tmp8, "%" PRIx32, &val);
					BE32(val);

					if (i+4 < len)
						memcpy(find + i+4, (char*) &val, 4);
    			}

				LOG("Searching (len=%d count=%d) ...", len, cnt);
				_log_dump("Search", (uint8_t*) find, len);

				pointer = search_data(data, dsize, (t == '8') ? pointer : 0, find, len, cnt);
				free(find);

				if (pointer < 0)
				{
					do
					{
						line = strtok(NULL, "\n");
					} while (line && ((line[0] != '8' && line[0] != 'B' && line[0] != 'C') || line[1] == '8'));

					pointer = 0;
					LOG(line ? "NOT FOUND - SKIP TO NEXT SEARCH" : "SEARCH PATTERN NOT FOUND");
					continue;
				}

				LOG("Search pointer = %ld (0x%lX)", pointer, pointer);
    		}
    			break;

    		case '9':
    			//	Pointer Manipulator (Set/Move Pointer)
    			//	Adjusts the Pointer Offset using numerous Operators
    			//	9Y000000 XXXXXXXX
    			//	Y = Operator
    			//	0 = Set Pointer to Big Endian value at XXXXXXXX
    			//	1 = Set Pointer to Little Endian value at XXXXXXXX
    			//	2 = Add X to Pointer
    			//	3 = Sub X to Pointer
    			//	4 = Set Pointer to the end of file and subtract X
    			//	5 = Set Pointer to X
    			//	D = Set End Address = to X
    			//	E = Set End Address From Pointer + X
    			//	X = Value to set / change
    			//	---
    			//	Move pointer to offset in address XXXXXXXXX (CONFIRMED CODE)
    			//	90000000 XXXXXXXX
    			//	---
    			//	Step Forward Code (CONFIRMED CODE)
    			//	92000000 XXXXXXXX
    			//	---
    			//	Step Back Code (CONFIRMED CODE)
    			//	93000000 XXXXXXXX
    			//	---
    			//	Step Back From End of File Code (CONFIRMED CODE)
    			//	94000000 XXXXXXXX
    		{
    			uint32_t off, val;

    			sprintf(tmp8, "%.8s", line+9);
    			sscanf(tmp8, "%" PRIx32, &off);

				switch (line[1])
				{
					case '0':
						val = *(uint32_t*)(data + off);
						BE32(val);
						pointer = val;
						break;
					case '1':
						val = *(uint32_t*)(data + off);
						LE32(val);
						pointer = val;
						break;
					case '2':
						pointer += off;
						break;
					case '3':
						pointer -= off;
						break;
					case '4':
						pointer = dsize - off;
						break;
					case '5':
						pointer = off;
						break;
					case 'D':
						end_pointer = off;
						LOG("End Pointer set to 0x%X (%d)", end_pointer, end_pointer);
						break;
					case 'E':
						end_pointer = pointer + off;
						LOG("End Pointer set to 0x%X (%d)", end_pointer, end_pointer);
						break;
				}
				LOG("Pointer set to offset 0x%X (%d)", pointer, pointer);
    		}
    			break;

    		case 'A':
    			//	Multi-write
    			//	ATxxxxxx yyyyyyyy  (xxxxxx = address, yyyyyyyy = size)
    			//	zzzzzzzz zzzzzzzz  <-data to write at address
    			//	T= Address/Offset type (0 = Normal / 8 = Offset From Pointer)
    		{
    			int off;
    			uint32_t val, size;
    			char* write;
    			char t = line[1];

    			sprintf(tmp6, "%.6s", line+2);
    			sscanf(tmp6, "%x", &off);
    			off += ((t == '8') ? pointer : 0);

    			sprintf(tmp8, "%.8s", line+9);
    			sscanf(tmp8, "%" PRIx32, &size);
				write = malloc((size+3) & ~3);

				for (uint32_t i = 0; i < size; i += 8)
				{
				    line = strtok(NULL, "\n");

    				sprintf(tmp8, "%.8s", line);
    				sscanf(tmp8, "%" PRIx32, &val);
					BE32(val);

					memcpy(write + i, (char*) &val, 4);

    				sprintf(tmp8, "%.8s", line+9);
    				sscanf(tmp8, "%" PRIx32, &val);
					BE32(val);

					if (i + 4 < size)
						memcpy(write + i+4, (char*) &val, 4);
				}

				_log_dump("m-Write", (uint8_t*) write, size);
				memcpy(data + off, write, size);
				free(write);

				LOG("m-Wrote %d bytes to 0x%X", size, off);
    		}
    			break;

			case 'B':
				//	Backward Byte Search (Set Pointer)
				//	Searches Backwards for a specified Value and saves the Value's Address as the Pointer Offset
				//	Will start from the end of the save file, but can be changed using a previous Pointer Offset
				//	BTCCYYYY XXXXXXXX
				//	*Other Code Here, Use Specific Offset Type*
				//	T = Offset Type
				//	0 = Default
				//	8 = Offset from Pointer
				//	C = Amount of Times to Find until Pointer Set
				//	Y = Amount of Bytes to Search
				//	1 = 1 Byte
				//	2 = 2 Bytes
				//	and so on...
				//	X = Bytes to Search, use Multiple Lines if Needed
			{
				int i, cnt, len;
				uint32_t val;
				char* find;
				char t = line[1];

				sprintf(tmp3, "%.2s", line+2);
				sscanf(tmp3, "%x", &cnt);

				sprintf(tmp4, "%.4s", line+4);
				sscanf(tmp4, "%x", &len);

				sprintf(tmp8, "%.8s", line+9);
				sscanf(tmp8, "%" PRIx32, &val);
				BE32(val);

				find = malloc((len+3) & ~3);
				if (!cnt) cnt = 1;
				if (!end_pointer) end_pointer = dsize - 1;

				memcpy(find, (char*) &val, 4);
				
				for (i=4; i < len; i += 8)
				{
					line = strtok(NULL, "\n");

					sprintf(tmp8, "%.8s", line);
					sscanf(tmp8, "%" PRIx32, &val);
					BE32(val);

					memcpy(find + i, (char*) &val, 4);

					sprintf(tmp8, "%.8s", line+9);
					sscanf(tmp8, "%" PRIx32, &val);
					BE32(val);

					if (i+4 < len)
						memcpy(find + i+4, (char*) &val, 4);
				}

				LOG("Backward Searching (len=%d count=%d) ...", len, cnt);
				_log_dump("Search", (uint8_t*) find, len);

				pointer = reverse_search_data(data, dsize, (t == '8') ? pointer : end_pointer, find, len, cnt);
				free(find);

				if (pointer < 0)
				{
					do
					{
						line = strtok(NULL, "\n");
					} while (line && ((line[0] != '8' && line[0] != 'B' && line[0] != 'C') || line[1] == '8'));

					pointer = 0;
					LOG(line ? "NOT FOUND - SKIP TO NEXT SEARCH" : "SEARCH PATTERN NOT FOUND");
					continue;
				}

				LOG("Search pointer = %ld (0x%lX)", pointer, pointer);
			}
				break;

			case 'C':
				//	Address Byte Search (Set Pointer)
				//	Searches for a Value from a specified Address and saves the new Value's Address as the Pointer Offset
				//	Rather than searching for Bytes already given such as code types 8 and B, this code will instead search using the bytes at a specific Address
				//	CBFFYYYY XXXXXXXX
				//	*Other Code Here, Use Specific Offset Type*
				//	B = Offset Type
				//	0 = Search Forwards from Address Given
				//	4 = Search from 0x0 to Address Given
				//	8 = Offset from Pointer; Search Forwards from Address Given
				//	C = Offset from Pointer; Search from 0x0 to Address Given
				//	F = Amount of Times to Find until Pointer Set
				//	Y = Amount of Bytes to Search from Address
				//	1 = 1 Byte
				//	2 = 2 Bytes
				//	and so on...
				//	X = Address of Bytes to Search with
			{
				int cnt, len;
				uint32_t addr;
				char* find;
				char t = line[1];

				sprintf(tmp3, "%.2s", line+2);
				sscanf(tmp3, "%x", &cnt);

				sprintf(tmp4, "%.4s", line+4);
				sscanf(tmp4, "%x", &len);

				sprintf(tmp8, "%.8s", line+9);
				sscanf(tmp8, "%" PRIx32, &addr);
				addr += ((t == '8' || t == 'C') ? pointer : 0);

				find = data + addr;
				if (!cnt) cnt = 1;

				LOG("Address Searching (len=%d count=%d) ...", len, cnt);
				_log_dump("Search", (uint8_t*) find, len);

				if (t == '4' || t == 'C')
					pointer = search_data(data, addr + len, 0, find, len, cnt);
				else
					pointer = search_data(data, dsize, addr + len, find, len, cnt);

				if (pointer < 0)
				{
					do
					{
						line = strtok(NULL, "\n");
					} while (line && ((line[0] != '8' && line[0] != 'B' && line[0] != 'C') || line[1] == '8'));

					pointer = 0;
					LOG(line ? "NOT FOUND - SKIP TO NEXT SEARCH" : "SEARCH PATTERN NOT FOUND");
					continue;
				}

				LOG("Search pointer = %ld (0x%lX)", pointer, pointer);
			}
				break;

			case 'D':
				//	2 Byte Test Commands (Code Skipper)
				//	Test a specific Address using an Operation; skips the following code lines if Operation fails
				//	DBYYYYYY CCZDXXXX
				//	B = Offset Type
				//	0 = Normal
				//	8 = Offset from Pointer
				//	Y = Address to test
				//	C = Lines of code to skip if test fails
				//	Z = Value data type
				//	0 = 16-bit
				//	1 = 8-bit
				//	D = Test Operation
				//	0 = Equal
				//	1 = Not Equal
				//	2 = Greater Than (Value at the Address is greater than the tested value)
				//	3 = Less Than (Value at the Address is less than the tested value)
				//	X = Value to test
			{
				int l, val, off;
				uint16_t src;
				char t = line[1];
				char op = line[12];
				char bit = line[11];

				sprintf(tmp6, "%.6s", line+2);
				sscanf(tmp6, "%x", &off);
				off += ((t == '8') ? pointer : 0);

				sprintf(tmp3, "%.2s", line+9);
				sscanf(tmp3, "%x", &l);

				sprintf(tmp4, "%.4s", line+13);
				sscanf(tmp4, "%x", &val);

				src = *(uint16_t*)(data + off);
				MEM16(src);

				if (bit == '1')
				{
					val &= 0xFF;
					src = (uint8_t)data[off];
				}

				switch (op)
				{
				case '0':
					/* Equal */
					LOG("Byte Test (%08X) %X = %X", off, src, val);
					off = (src == val);
					break;

				case '1':
					/* Not equal */
					LOG("Byte Test (%08X) %X != %X", off, src, val);
					off = (src != val);
					break;

				case '2':
					/* Greater than */
					LOG("Byte Test (%08X) %X > %X", off, src, val);
					off = (src > val);
					break;

				case '3':
					/* Less than */
					LOG("Byte Test (%08X) %X < %X", off, src, val);
					off = (src < val);
					break;

				default:
					off = 1;
					LOG("Byte Test Unexpected operation (%c)!", op);
					break;
				}

				if (!off)
				{
					LOG("Skipping %d lines...", l);
					while (l--)
						line = strtok(NULL, "\n");
				}
			}
				break;

    		default:
    			break;
    	}
    }

	write_buffer(filepath, (uint8_t*) data, dsize);

	free(data);
	free(gg_code);

	return (dsize);
}

static void* dummy_host_callback(int id, int* size)
{
	switch (id)
	{
	case APOLLO_HOST_TEMP_PATH:
		break;

	case APOLLO_HOST_USERNAME:
	case APOLLO_HOST_SYS_NAME:
	case APOLLO_HOST_LAN_ADDR:
	case APOLLO_HOST_WLAN_ADDR:
	case APOLLO_HOST_ACCOUNT_ID:
		if (size) *size = 6;
		return "APOLLO";
	}

	if (size) *size = 1;
	return "";
}

int apply_cheat_patch_code(const char* fpath, const char* title_id, const code_entry_t* code, apollo_host_cb_t host_cb)
{
	host_callback = host_cb ? host_cb : dummy_host_callback;

	if (code->type == APOLLO_CODE_GAMEGENIE)
	{
		LOG("Game Genie Code");
		return apply_ggenie_patch_code(fpath, code);
	}

	if (code->type == APOLLO_CODE_BSD)
	{
		LOG("Bruteforce Save Data Code");

		if (wildcard_match_icase(code->codes, "decompress *"))
		{
			const char* tmp_dir = host_callback(APOLLO_HOST_TEMP_PATH, NULL);

			// decompress FILENAME
			LOG("Decompressing '%s' (w=%d)...", fpath, OFFZIP_WBITS_ZLIB);

			// try zlib data (default) zlib is header+deflate+crc
			int ret = offzip_util(fpath, tmp_dir, title_id, OFFZIP_WBITS_ZLIB);

			// if zlib didn't work, try deflate (many false positives, used in Zip archives)
			if (!ret)
				ret = offzip_util(fpath, tmp_dir, title_id, OFFZIP_WBITS_DEFLATE);

			return ret;
		}
		else if (wildcard_match_icase(code->codes, "compress *,-w*"))
		{
			// compress FILENAME,-w bits
			DIR *d;
			struct dirent *dir;
			int wbits;
			uint32_t offset;
			char infile[2048];
			const char* tmp_dir = host_callback(APOLLO_HOST_TEMP_PATH, NULL);

			char* tmp = strchr(code->codes, ',') + 3;
			sscanf(tmp, "%d", &wbits);

			if ((wbits < OFFZIP_WBITS_DEFLATE) || (wbits > OFFZIP_WBITS_ZLIB))
				wbits = OFFZIP_WBITS_ZLIB;

			LOG("Compressing '%s' (w=%d)...", fpath, wbits);

			d = opendir(tmp_dir);
			if (!d)
				return 0;

			while ((dir = readdir(d)) != NULL)
			{
				//[BLUS12345]00000000.dat
				if (wildcard_match(dir->d_name, "[*]*.dat") && strncmp(dir->d_name + 1, title_id, 9) == 0)
				{
					tmp = strchr(dir->d_name, ']') + 1;
					sscanf(tmp, "%" PRIx32 ".dat", &offset);
					snprintf(infile, sizeof(infile), "%s%s", tmp_dir, dir->d_name);

					LOG("Adding '%s' to '%s' at %d (0x%X)...", dir->d_name, code->file, offset, offset);
					if (!packzip_util(infile, fpath, offset, wbits))
					{
						LOG("Error: unable to compress file (%s)", dir->d_name);
						return 0;
					}

					if (access(infile, F_OK) == SUCCESS)
					{
						chmod(infile, 0777);
						remove(infile);
					}					
				}
			}
			closedir(d);

			return 1;
		}

		return apply_bsd_patch_code(fpath, code);
	}

	return 0;
}
