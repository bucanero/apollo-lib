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
#define CLI_VERSION     APOLLO_LIB_VERSION " PS3/big-endian"
#else
#define CLI_VERSION     APOLLO_LIB_VERSION
#endif

static int log = 0;

void print_usage(const char* argv0)
{
    printf("USAGE: %s file.savepatch 1,2,7-10,18 [data-file.bin]\n\n", argv0);
    printf("  file.savepatch: The cheat patch file to apply\n");
    printf("  1,2,7-10,18:    The list of codes to apply\n");
    printf("  data-file.bin:  The target file to patch\n\n");
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

static void* cli_host_callback(int id, int* size)
{
	switch (id)
	{
	case APOLLO_HOST_TEMP_PATH:
		return TMP_FOLDER;

	case APOLLO_HOST_USERNAME:
	case APOLLO_HOST_SYS_NAME:
	case APOLLO_HOST_LAN_ADDR:
	case APOLLO_HOST_WLAN_ADDR:
		if (size) *size = 6;
		return "APOLLO";
	}

	if (size) *size = 1;
	return "";
}

int main(int argc, char **argv)
{
    size_t len;
    char *data, title[32];
    list_t* list_codes;

    printf("\nApollo cheat patcher v%s - (c) 2022 by Bucanero\n\n", CLI_VERSION);

    if (--argc < 2)
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
    code->file = argv[1];

    list_codes = list_alloc();
    list_append(list_codes, code);
    load_patch_code_list(data, list_codes, NULL, NULL);
    free(data);

    list_node_t *node = list_head(list_codes);

    snprintf(title, sizeof(title), "%p", data);
    printf("[i] Applying codes [%s] to %s...\n", argv[2], (argc == 2) ? "script target file" : argv[3]);

    for (len=1, node = list_next(node); (code = list_get(node)); node = list_next(node), len++)
    {
        if (is_active_code(argv[2], len))
        {
            log++;
            printf("\n===============[ Applying code #%ld ]===============\n", len);
            if (apply_cheat_patch_code((argc == 2) ? code->file : argv[3], title, code, &cli_host_callback))
                printf("- OK\n");
            else
                printf("- ERROR!\n");
        }
    }

    free_patch_var_list();
    printf("\nPatching completed: %d codes applied\n\n", log);
}
