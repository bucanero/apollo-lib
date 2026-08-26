#ifndef APOLLO_TEST_COMMON_H
#define APOLLO_TEST_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include "apollo.h"

/*
 * Endian build mode.
 *
 * Today the library selects little- vs big-endian save-wizard behavior at
 * COMPILE TIME: the BIGENDIAN make flag defines __PS3_PC__, which makes the
 * MEM* macros byte-swap (see include/types.h). The two binaries produced,
 * test_apollo_le and test_apollo_be, are the reference for the upcoming
 * refactor that turns this into a RUNTIME choice.
 *
 * When endianness becomes a runtime parameter, this is the single place that
 * needs to change: replace the compile-time detection below (and the helpers
 * in test_common.c that build code_entry_t) with the new runtime selector.
 * The test vectors and golden files must NOT change.
 */
#ifdef __PS3_PC__
#define APOLLO_TEST_ENDIAN_BE   1
#define APOLLO_TEST_ENDIAN_NAME "BE"
#else
#define APOLLO_TEST_ENDIAN_BE   0
#define APOLLO_TEST_ENDIAN_NAME "LE"
#endif

/* ---- assertion counters (defined in test_common.c) ---- */
extern int g_checks_run;
extern int g_checks_failed;
extern int g_cur_test_failed;

/* ---- assertions ---- */
int check_mem(const char* file, int line, const char* desc,
              const void* got, const void* exp, size_t len);
int check_u64(const char* file, int line, const char* desc,
              uint64_t got, uint64_t exp);
int check_str(const char* file, int line, const char* desc,
              const char* got, const char* exp);

#define CHECK_MEM(desc, got, exp, len)  check_mem(__FILE__, __LINE__, desc, got, exp, len)
#define CHECK_U64(desc, got, exp)       check_u64(__FILE__, __LINE__, desc, (uint64_t)(got), (uint64_t)(exp))
#define CHECK_STR(desc, got, exp)       check_str(__FILE__, __LINE__, desc, got, exp)

/* ---- code builders ----
 * apollo_apply_sw_code / apollo_apply_bsd_code only read ->codes and
 * ->options_count, so a stack code_entry_t is enough for a vector.
 */
code_entry_t make_sw_code(const char* codes);
code_entry_t make_bsd_code(const char* codes);

/* ---- deterministic data helpers ---- */
void     fill_lcg(uint8_t* buf, size_t len, uint32_t seed);  /* reproducible pseudo-random fill */
uint64_t fnv1a(const uint8_t* buf, size_t len);              /* stable digest for golden manifests */

/* ---- test registry (constructor-based auto-registration) ---- */
typedef void (*apollo_test_fn)(void);
void register_test(const char* name, apollo_test_fn fn);
int  run_registered_tests(void);

#define TEST(name)                                                        \
    static void name(void);                                               \
    __attribute__((constructor)) static void reg_##name(void) {           \
        register_test(#name, name);                                       \
    }                                                                     \
    static void name(void)

/* corpus entry point (test_corpus.c) */
int corpus_run(const char* root);

#endif /* APOLLO_TEST_COMMON_H */
