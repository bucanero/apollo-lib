#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "apollo.h"

#define CLI_VERSION     APOLLO_LIB_VERSION

static int log = 0;

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

    printf("\nApollo binary file SW dumper v%s - (c) 2023 by Bucanero\n\n", CLI_VERSION);

    if (--argc < 1)
    {
        print_usage(argv[0]);
        return -1;
    }

    if (read_buffer(argv[1], &data, &len) != 0)
    {
        printf("[*] Could Not Access The File (%s)\n", argv[1]);
        return -1;
    }

    snprintf(patch, sizeof(patch), "%s%s", argv[1], ".savepatch");
    fp = fopen(patch, "w");
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
