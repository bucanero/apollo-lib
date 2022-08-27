#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "apollo.h"

void print_usage(const char* argv0)
{
    printf("USAGE: %s filename.savepatch\n\n", argv0);
    return;
}

void dbglogger_log(const char* fmt, ...)
{
    if (0)
    {
        char buffer[0x800];

        va_list arg;
        va_start(arg, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, arg);
        va_end(arg);

        printf("[log] %s\n", buffer);
    }
}

int main(int argc, char **argv)
{
    size_t len;
    char *data;
    list_t* list_codes;

    printf("\nApollo .savepatch to markdown parser 0.1.0 - (c) 2021 by Bucanero\n\n");

    if (--argc < 1)
    {
        print_usage(argv[0]);
        return -1;
    }

    if (read_buffer(argv[1], (uint8_t **) &data, &len) != 0)
    {
        printf("[*] Could Not Access The File (%s)\n", argv[1]);
        return -1;
    }

    code_entry_t* code = calloc(1, sizeof(code_entry_t));
    code->name = strrchr(argv[1], '/') + 1;
    code->file = strrchr(argv[1], '/');

    list_codes = list_alloc();
    list_append(list_codes, code);
    load_patch_code_list(data, list_codes, NULL, NULL);
    free(data);

    list_node_t *node = list_head(list_codes);
    FILE *fp = fopen("out.md", "w");

    printf("Parsing file %s...\n\n", argv[1]);
    fprintf(fp, "# %s\n\n", code->name);

    for (len = 1, node = list_next(node); (code = list_get(node)); node = list_next(node), len++)
    {

        if ((code->name[1] & 0xFF) == 0xE2)
            code->name = code->name+4;
        else if ((code->name[0] & 0xFF) == 0xE2)
            code->name = code->name+3;

        if ((code->name[1] & 0xFF) == 0x0F)
            code->name = code->name+2;

        printf("%d. %s%s\n", len, code->name, code->codes[0] ? "" : " (Empty)");
        fprintf(fp, "### %d. %s\n\nTarget File: `%s`\n\n", len, code->name, code->file);
        if (code->codes && code->codes[0])
            fprintf(fp, "```\n%s```\n\n", code->codes);
/*
        if (bv->data)
            free(bv->data);
        if (bv->name)
            free(bv->name);

        free(bv);
*/
    }

    fclose(fp);
    printf("\nParse completed: %d codes\n\n", len-1);
}
