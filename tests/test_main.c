#include <stdio.h>
#include <string.h>
#include "test_common.h"

int main(int argc, char** argv)
{
    if (argc >= 3 && strcmp(argv[1], "--corpus") == 0)
        return corpus_run(argv[2]);

    if (argc >= 2 && strcmp(argv[1], "--corpus") == 0) {
        fprintf(stderr, "usage: %s --corpus <patches-dir>\n", argv[0]);
        return 2;
    }

    return run_registered_tests();
}
