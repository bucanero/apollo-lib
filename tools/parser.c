#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "apollo.h"

#define CLI_VERSION     APOLLO_LIB_VERSION

static int log = 0;

void print_usage(const char* argv0)
{
    printf("USAGE: %s filename.savepatch\n\n", argv0);
    return;
}

void dbglogger_log(const char* fmt, ...)
{
    if (!log)
        return;

    char buffer[0x800];

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, arg);
    va_end(arg);

    printf("%s\n", buffer);
}

const char* group_flags(int flag)
{
    if (flag & APOLLO_CODE_FLAG_PARENT)   return "# ";
    if (flag & APOLLO_CODE_FLAG_CHILD)    return "+--- ";

    return "";
}
const char* info_flags(int flag)
{
    if (flag & APOLLO_CODE_FLAG_ALERT)    return "[!] ";
    if (flag & APOLLO_CODE_FLAG_EMPTY)    return "[E] ";
    if (flag & APOLLO_CODE_FLAG_DISABLED) return "[D] ";

    return "";
}

int main(int argc, char **argv)
{
    size_t len, item = 0;
    char *data;
    list_t* list_codes;

    printf("\nApollo .savepatch parser v%s - (c) 2021-2025 by Bucanero\n\n", CLI_VERSION);

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
    data = realloc(data, len+1);
    data[len] = 0;

    code_entry_t* code = calloc(1, sizeof(code_entry_t));
    code->name = argv[1];
    code->file = strchr(data+1, ';');
    if (code->file)
    {
        code->file++;
        code->name = calloc(1, 128);
        memcpy(code->name, code->file, strchr(code->file, '\n') - code->file);
        for (char* c = code->name; c[0]; c++)
            if (*c < ' ') c[0] = ' ';
    }
    code->file = argv[1];

    list_codes = list_alloc();
    list_append(list_codes, code);
    load_patch_code_list(data, list_codes, NULL, NULL);
    free(data);

    log = (argc == 2 && argv[2][1] == 'd');
    list_node_t *node = list_head(list_codes);
    FILE *fp = NULL;

    printf("Parsing file %s...\n\n", argv[1]);
    if (log)
    {
        data = strdup(argv[1]);
        strcpy(strrchr(data, '.'), ".md");
        fp = fopen(data, "w");
        fprintf(fp, "# %s\n\n`%s`\n\n", code->name, code->file);
        free(data);
    }

    if (argc == 2 && sscanf(argv[2], "%ld", &item) == 1)
        printf("Selected Code #%ld\n", item);

    for (len = 1, node = list_next(node); (code = list_get(node)); node = list_next(node), len++)
    {
        if (item && item != len)
            continue;

        if (item)
        {
            printf("\n:%s\n\n[%s]\n%s\n", code->file, code->name, code->codes);
            return 0;
        }

        printf("%4ld. %s%s%s\n", len, group_flags(code->flags), info_flags(code->flags), code->name);

        if (log) fprintf(fp, "### %ld. %s\n", len, code->name);
        if (log && !(code->flags & APOLLO_CODE_FLAG_EMPTY))
            fprintf(fp, "\nTarget File: `%s`\n\n```\n%s```\n\n", code->file, code->codes);
    }

    if (log) fclose(fp);
    printf("\nParse completed: %ld codes\n\n", len-1);
}
