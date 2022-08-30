#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "apollo.h"

#ifdef __WIN32__
#define TMP_FOLDER      "C:\\WINDOWS\\TEMP\\"
#else
#define TMP_FOLDER      "/tmp/"
#endif

#ifdef __PS3_PC__
#define CLI_VERSION     "0.1.0 PS3/big-endian"
#else
#define CLI_VERSION     "0.1.0"
#endif

static int log = 0;

void print_usage(const char* argv0)
{
    printf("USAGE: %s <FILE.savepatch> 1,2,7,..,18 target.file\n\n", argv0);
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

    printf("- %s\n", buffer);
}

int is_active_code(const char* a, int id)
{
    int val;
    char* arg = strdup(a);

     for (char* tmp = strtok(arg, ","); tmp; tmp = strtok(NULL, ","))
     {
        sscanf(tmp, "%d", &val);
        if (val == id)
        {
            free(arg);
            return 1;
        }
     }

    free(arg);
    return 0;
}

int main(int argc, char **argv)
{
    size_t len;
    char *data, title[32];
    list_t* list_codes;

    printf("\nApollo cheat patcher v%s - (c) 2022 by Bucanero\n\n", CLI_VERSION);

    if (--argc < 3)
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
    code->name = argv[1];
    code->file = argv[1];

    list_codes = list_alloc();
    list_append(list_codes, code);
    load_patch_code_list(data, list_codes, NULL, NULL);
    free(data);

    list_node_t *node = list_head(list_codes);

    snprintf(title, sizeof(title), "%p", data);
    printf("[i] Applying codes [%s] to %s...\n\n", argv[2], argv[3]);

    for (len=1, node = list_next(node); (code = list_get(node)); node = list_next(node), len++)
    {
        if (is_active_code(argv[2], len))
        {
            log++;
            printf("[+] Applying code #%d...\n", len);
            if (apply_cheat_patch_code(argv[3], title, code, TMP_FOLDER))
                printf("- OK\n");
            else
                printf("- ERROR!\n");
        }
    }

    free_patch_var_list();
    printf("\nPatching completed: %d codes applied\n\n", log);
}
