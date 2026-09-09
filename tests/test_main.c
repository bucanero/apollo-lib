#include <stdio.h>
#include <string.h>
#include "test_common.h"

/*
 * One binary covers both endian modes.
 *
 * The library picks save-data endianness at runtime (per code entry, see
 * test_common.h), so the whole vector suite runs twice in one process — once
 * little-endian, once big-endian — instead of being compiled twice. Each pass
 * prints its own header and totals, so the output is what the two former
 * binaries produced back to back.
 *
 * Corpus mode runs a single pass, because its callers compare one mode against
 * the other (see the bsd-invariance target):
 *
 *   test_apollo                          both vector passes
 *   test_apollo --corpus <dir>           corpus manifest, little-endian
 *   test_apollo --corpus <dir> --be      corpus manifest, big-endian
 */
int main(int argc, char** argv)
{
    if (argc >= 2 && strcmp(argv[1], "--corpus") == 0) {
        if (argc < 3) {
            fprintf(stderr, "usage: %s --corpus <patches-dir> [--be]\n", argv[0]);
            return 2;
        }
        apollo_test_set_endian(argc >= 4 && strcmp(argv[3], "--be") == 0);
        return corpus_run(argv[2]);
    }

    int rc = 0;
    for (int be = 0; be <= 1; be++) {
        apollo_test_set_endian(be);
        rc |= run_registered_tests();

        /* Patch variables accumulate across applies and are process-global;
         * drop them so the second pass starts from the same state as the
         * first. (It also resets the engine's global byte order, which the
         * vectors do not rely on — they carry the flag per code.) */
        apollo_free_var_list();
    }
    return rc;
}
