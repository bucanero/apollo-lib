#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "apollo.h"

#ifdef __PS3_PC__
#define CLI_VERSION     APOLLO_LIB_VERSION " PS3/big-endian"
#else
#define CLI_VERSION     APOLLO_LIB_VERSION
#endif

static int log = 0;
static const char* CODE_TYPES[] = {
    "Unknown",
    "Save Wizard",
    "BSD",
    "Python"
};

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
    if (flag & APOLLO_CODE_FLAG_REQUIRED) return "[R] ";

    return "";
}

void print_usage(const char* argv0)
{
    printf("Patching:\n");
    printf(" USAGE: %s file.savepatch 1,2,7-10,18 [data-file.bin]\n\n", argv0);
    printf("  file.savepatch: The cheat patch file to apply\n");
    printf("  1,2,7-10,18:    The list of codes to apply\n");
    printf("  data-file.bin:  The target file to patch\n\n");
    printf("Listing:\n");
    printf(" USAGE: %s file.savepatch [-c 1,2,7-10,18]\n\n", argv0);
    printf("  file.savepatch: The cheat patch file to list\n");
    printf("  -c:             Display code details (Optional)\n");
    printf("  1,2,7-10,18:    The list of codes to display (Optional)\n\n");
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
    int val, end;
    char* arg = strdup(a);

    for (char* tmp = strtok(arg, ","); tmp; tmp = strtok(NULL, ","))
    {
        if (((sscanf(tmp, "%d-%d", &val, &end) == 2) && (id >= val && id <= end)) ||
            ((sscanf(tmp, "%d", &val) == 1) && (val == id)))
        {
            free(arg);
            return 1;
        }
    }

    free(arg);
    return 0;
}

static void get_user_options(code_entry_t* entry)
{
    option_value_t* val;

    printf("\n[%s] Options:\n", entry->name);
    for (int j, i=0; i<entry->options_count; i++)
    {
        j = 0;
        printf("  Tag: %s\n", entry->options[i].line);
        for (list_node_t* node = list_head(entry->options[i].opts); (val = list_get(node)); node = list_next(node))
        {
            printf("%4d.  %s\n", j++, val->name);
        }
        printf("\n Select option: ");
        scanf("%d", &entry->options[i].sel);

        val = list_get_item(entry->options[i].opts, entry->options[i].sel);
        if (val)
        {
            printf("  Selected: %s\n", val->name);
            printf("  Value   : %s\n\n", val->value);
        }
    }
}

void print_codes(code_entry_t *code, list_node_t *node, const char* code_opt, int out)
{
    FILE *fp = NULL;
    uint32_t pos;

    if (out)
    {
        char* data = strdup(code->file);
        strcpy(strrchr(data, '.'), ".md");
        fp = fopen(data, "w");
        if (!fp)
        {
            printf("[*] Could Not Create The File (%s)\n", data);
            exit(-1);
        }

        fprintf(fp, "# %s\n\n`%s`\n\n", code->name, code->file);
        printf("Output file: %s\n", data);
        free(data);
    }

    for (pos = 1, node = list_next(node); (code = list_get(node)); node = list_next(node), pos++)
    {
        if (code_opt)
        {
            if (is_active_code(code_opt, pos))
                printf("===============[ Code #%d ]===============\n"
                    "Type: %s\nFile: %s\n\n[%s]\n%s\n", pos, CODE_TYPES[code->type], code->file, code->name, code->codes);

            continue;
        }

        if (out)
        {
            fprintf(fp, "### %d. %s\n", pos, code->name);
            if (!(code->flags & APOLLO_CODE_FLAG_EMPTY))
                fprintf(fp, "\nTarget File: `%s`\n\n```\n%s```\n\n", code->file, code->codes);
 
            continue;
        }

        printf("%4d. %s%s%s\n", pos, group_flags(code->flags), info_flags(code->flags), code->name);
    }

    if (out) fclose(fp);
    printf("\nParse completed: %d codes\n\n", pos-1);
}

int main(int argc, char **argv)
{
    size_t len;
    char *data;
    list_t* list_codes;

    printf("\nApollo Cheat Patcher v%s - (c) 2022-2026 by Bucanero\n\n", CLI_VERSION);

    if (--argc < 1)
    {
        print_usage(argv[0]);
        return -1;
    }

    if (strchr(argv[1], '\n') && strchr(argv[1], '[') && strchr(argv[1], ']'))
    {
        // Direct string codes as input
        len = strlen(argv[1]);
        data = malloc(len);
        memcpy(data, argv[1], len);
    }
    else if (read_buffer(argv[1], (uint8_t **) &data, &len) != 0)
    {
        printf("[*] Could Not Access The File (%s)\n", argv[1]);
        return -1;
    }
    data = realloc(data, len+1);
    data[len] = 0;

    code_entry_t* code = calloc(1, sizeof(code_entry_t));
    code->name = argv[1];
    code->file = argv[1];
    char *tmp = strchr(data+1, ';');
    if (tmp)
    {
        tmp++;
        len = strcspn(tmp, "\n");

        code->name = malloc(len + 1);
        memcpy(code->name, tmp, len);
        code->name[len] = '\0';
        for (tmp = code->name; tmp[0]; tmp++)
            if (*tmp < ' ') tmp[0] = ' ';
    }

    list_codes = list_alloc();
    list_append(list_codes, code);
    load_patch_code_list(data, list_codes, NULL, NULL);
    free(data);

    list_node_t *node = list_head(list_codes);

    printf("Game: %s\n\n", code->name);

    if (argc == 1 || (argc == 2 && argv[2][1] == 'd') || (argc == 3 && argv[2][1] == 'c'))
    {
        print_codes(code, node, (argc == 3) ? argv[3] : NULL, (argc == 2 && argv[2][1] == 'd'));
        return 0;
    }

    printf("[i] Applying codes [%s] to %s...\n", argv[2], (argc == 2) ? "script target file" : argv[3]);

    for (len=1, node = list_next(node); (code = list_get(node)); node = list_next(node), len++)
    {
        if (code->activated || is_active_code(argv[2], len))
        {
            log++;
            if (code->options_count)
                get_user_options(code);

            printf("\n===============[ Applying code #%ld ]===============\n", len);
            if (apply_cheat_patch_code((argc == 2) ? code->file : argv[3], code, NULL))
                printf("- OK\n");
            else
                printf("- ERROR!\n");
        }
    }

    free_patch_var_list();
    printf("\nPatching completed: %d codes applied\n\n", log);
}
