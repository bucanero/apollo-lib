#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "apollo.h"

#include <dbglogger.h>
#define LOG dbglogger_log


uint64_t x_to_u64(const char *hex)
{
	uint64_t result, t;
	uint32_t len;
	int c;

	result = 0;
	t = 0;
	len = strlen(hex);

	while (len--) {
		c = *hex++;
		if (c >= '0' && c <= '9')
			t = c - '0';
		else if (c >= 'a' && c <= 'f')
			t = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			t = c - 'A' + 10;
		else
			t = 0;
		result |= t << (len * 4);
	}

	return result;
}

uint8_t * x_to_u8_buffer(const char *hex)
{
	char tmp[3] = { 0, 0, 0 };
	uint8_t *result;
	uint8_t *ptr;
	uint32_t len;

	len = strlen(hex);
	if (len % 2 != 0)
		return NULL;

	result = (uint8_t *)malloc(len);
	memset(result, 0, len);
	ptr = result;

	while (len--) {
		tmp[0] = *hex++;
		tmp[1] = *hex++;
		*ptr++ = (uint8_t)x_to_u64(tmp);
	}

	return result;
}

int read_buffer(const char *file_path, uint8_t **buf, size_t *size) {
        FILE *fp;
        uint8_t *file_buf;
        size_t file_size;

        if ((fp = fopen(file_path, "rb")) == NULL)
                return -1;
        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        file_buf = (uint8_t *)malloc(file_size);
        fread(file_buf, 1, file_size, fp);
        fclose(fp);

        if (buf)
                *buf = file_buf;
        else
                free(file_buf);
        if (size)
                *size = file_size;

        return 0;
}

int write_buffer(const char *file_path, const uint8_t *buf, size_t size)
{
        FILE *fp;

        if ((fp = fopen(file_path, "wb")) == NULL)
                return -1;
        fwrite(buf, 1, size, fp);
        fclose(fp);

        return 0;
}

static void clean_eol(char * str)
{
	for (int x = 0, len = strlen(str); x < len; x++)
		if (strncmp(&str[x], "\r\n", 2) == 0)
			str[x] = ' ';
		else if (str[x] == '\r')
			str[x] = '\n';
}

static void remove_char(char * str, int len, char seek)
{
	for (int x = 0; x < len; x++)
		if (str[x] == seek)
			str[x] = '\n';
}

/*
 * Function:		str_ends_with()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		Checks to see if a ends with b
 * Arguments:
 *	a:				String
 *	b:				Potential end
 * Return:			pointer if true, NULL if false
 */
static char* str_ends_with(const char * a, const char * b)
{
	int al = strlen(a), bl = strlen(b);
    
	if (al < bl)
		return NULL;

	a += (al - bl);
	while (*a)
		if (toupper(*a++) != toupper(*b++)) return NULL;

	return (char*) (a - bl);
}

/*
 * Function:		str_rtrim()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		Trims ending white spaces (' ') from a string
 * Arguments:
 *	buffer:			String
 * Return:			Amount of characters removed
 */
static int str_rtrim(char * buffer)
{
	int i, max = strlen(buffer) - 1;
	for (i = max; (buffer[i] == ' ') && (i >= 0); i--)
		buffer[i] = 0;

	return (max - i);
}

// Expects buffer without CR's (\r)
static void get_patch_code(char* buffer, int code_id, code_entry_t* entry)
{
	int i=0;
    char *tmp = NULL;
    char *res = calloc(1, 1);
    char *line = strtok(buffer, "\n");

    while (line)
    {
    	if ((wildcard_match(line, "[*]") ||
			wildcard_match(line, "; --- * ---") ||
			wildcard_match_icase(line, "GROUP:*")) && (i++ == code_id))
    	{
			LOG("Reading patch code for '%s'...", line);

		    for (line = strtok(NULL, "\n"); line != NULL; line = strtok(NULL, "\n"))
		    {
		    	if ((wildcard_match(line, "; --- * ---")) 	||
		    		(wildcard_match(line, ":*"))			||
		    		(wildcard_match(line, "[*]"))			||
		    		(wildcard_match_icase(line, "PATH:*"))	||
		    		(wildcard_match_icase(line, "GROUP:*")))
		    	{
						break;
			    }

		    	if (!wildcard_match(line, ";*"))
		    	{
					asprintf(&tmp, "%s%s\n", res, line);
					free(res);
					res = tmp;

//			    	LOG("%s", line);
					if (!wildcard_match(line, "\?\?\?\?\?\?\?\? \?\?\?\?\?\?\?\?") || (
						(line[0] < '0') && (line[0] > '9') && (line[0] < 'A') && (line[0] > 'F')))
						entry->type = APOLLO_CODE_BSD;
			    }
		    }
    	}
		if (i > code_id || !line)
			break;

    	line = strtok(NULL, "\n");
    }

//	LOG("Result (%s)", res);
	entry->codes = res;
}

int load_patch_code_list(char* buffer, list_t* list_codes, apollo_get_files_cb_t get_files_opt, const char* save_path)
{
	int code_count = 0;
	code_entry_t * code;
	char * file_opt = NULL;
	char filePath[256] = "";
	char group = 0;
	size_t bufferLen = strlen(buffer);
	list_node_t* node = list_tail(list_codes);

	clean_eol(buffer);
	for (char *line = strtok(buffer, "\n"); line != NULL; line = strtok(NULL, "\n"))
	{
		str_rtrim(line);
		if (wildcard_match(line, ":*"))
		{
			char* tmp_mask;

			strncpy(filePath, line+1, sizeof(filePath)-1);
			LOG("FILE: %s\n", filePath);

			if (strrchr(filePath, '\\'))
				tmp_mask = strrchr(filePath, '\\')+1;
			else
				tmp_mask = filePath;

			if (strchr(tmp_mask, '*') && get_files_opt)
				file_opt = tmp_mask;
			else
				file_opt = NULL;

		}
		else if (wildcard_match(line, "?=*") ||
					wildcard_match(line, "\?\?=*") ||
					wildcard_match(line, "\?\?\?\?=*"))
		{
			// options
		}
		else if (wildcard_match_icase(line, "PATH:*"))
		{
			//
		}
		else if (wildcard_match(line, "[*]") ||
				wildcard_match(line, "; --- * ---") ||
				wildcard_match_icase(line, "GROUP:*"))
		{
			if (wildcard_match_icase(line, "[DEFAULT:*"))
			{
				line += 8;
				line[0] = APOLLO_CODE_FLAG_ALERT;
			}
			else if (wildcard_match_icase(line, "[INFO:*"))
			{
				line += 5;
				line[0] = APOLLO_CODE_FLAG_ALERT;
			}
			else if (wildcard_match_icase(line, "*GROUP:\\*"))
			{
				group = 0;
				line = strrchr(line, '\\');
				line[0] = ' ';
			}
			else if (wildcard_match_icase(line, "[GROUP:*"))
			{
				line += 6;
				group = APOLLO_CODE_FLAG_PARENT;
				LOG("GROUP: %s\n", line);
			}
			else if (wildcard_match(line, "; --- * ---") || wildcard_match_icase(line, "GROUP:*"))
			{
				line += 5;
				group = APOLLO_CODE_FLAG_PARENT;
				LOG("GROUP: %s\n", line);
			}
			line++;

			code = calloc(1, sizeof(code_entry_t));
			code->type = APOLLO_CODE_GAMEGENIE;
			code->options = (file_opt ? get_files_opt(save_path, file_opt) : NULL);
			code->options_count = (file_opt ? 1 : 0);
			code->file = strdup(filePath);
			code->name = strdup(line);
			list_append(list_codes, code);

			if(line[-1] == APOLLO_CODE_FLAG_ALERT)
				code->flags |= APOLLO_CODE_FLAG_ALERT;

			if (wildcard_match_icase(code->name, "*(REQUIRED)*"))
				code->flags |= APOLLO_CODE_FLAG_REQUIRED;

			switch (group)
			{
				case APOLLO_CODE_FLAG_PARENT:
					code->flags |= APOLLO_CODE_FLAG_PARENT;
					group = APOLLO_CODE_FLAG_CHILD;
					break;

				case APOLLO_CODE_FLAG_CHILD:
					code->flags |= APOLLO_CODE_FLAG_CHILD;
					break;
			}

			char* end = strrchr(code->name, ']');
			if (end) *end = 0;

			end = str_ends_with(code->name, " ---");
			if (end) *end = 0;
		}
	}

	while ((node = list_next(node)) != NULL)
	{
		code = list_get(node);
		// remove 0x00 from previous strtok(...)
		remove_char(buffer, bufferLen, '\0');
		get_patch_code(buffer, code_count++, code);

		if(!code->codes[0])
			code->flags |= APOLLO_CODE_FLAG_EMPTY;

		LOG("[%d] Name: %s\nFile: %s\nCode (%d): %s\n", code_count, code->name, code->file, code->type, code->codes);
	}

	return code_count;
}
