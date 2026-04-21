#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "apollo.h"

#define CLI_VERSION     APOLLO_LIB_VERSION

void print_usage(const char* argv0)
{
    printf("USAGE: %s filename.ext\n\n", argv0);
    return;
}

void dbglogger_log(const char* fmt, ...)
{
    return (void)fmt;
}

int main(int argc, char **argv)
{
    size_t len;
    unsigned char *data;
    char patch[256];
    FILE* fp;

    printf("\nApollo binary file SW dumper v%s - (c) 2023-2026 by Bucanero\n\n", CLI_VERSION);

    if (--argc < 1)
    {
        print_usage(argv[0]);
        return -1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp)
    {
        printf("[*] Could Not Access The File (%s)\n", argv[1]);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = malloc(len);
    fread(data, 1, len, fp);
    fclose(fp);

    snprintf(patch, sizeof(patch), "%s%s", argv[1], ".savepatch");
    fp = fopen(patch, "w");
    if (!fp)
    {
        printf("[*] Could Not Create The File (%s)\n", patch);
        return -1;
    }

    fprintf(fp, "; set XXXXXXXX to the offset where you want to write the data.\n");
    fprintf(fp, "[%s]\n95000000 XXXXXXXX\nA8000000 %08X\n", argv[1], (uint32_t)len);
    for (size_t i=1; i <= len; i++)
    {
        fprintf(fp, "%02X", data[i-1]);
        if (i % 8 == 0)
            fprintf(fp, "\n");
        else if (i % 4 == 0)
            fprintf(fp, " ");
    }

    switch(8-(len%8))
    {
        case 7:
            fprintf(fp, "00");
        case 6:
            fprintf(fp, "00");
        case 5:
            fprintf(fp, "00 ");
        case 4:
            fprintf(fp, "00");
        case 3:
            fprintf(fp, "00");
        case 2:
            fprintf(fp, "00");
        case 1:
            fprintf(fp, "00");
        default:
            fprintf(fp, "\n");
    }

    free(data);
    fclose(fp);

    printf("Parse completed: %ld bytes dumped\n\n", len);
}
