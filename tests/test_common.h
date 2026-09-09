#ifndef APOLLO_TEST_COMMON_H
#define APOLLO_TEST_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include "apollo.h"

/*
 * Endian mode.
 *
 * The library selects little- vs big-endian save-data behavior at RUNTIME:
 * apollo_apply_sw_code() takes it from the code entry's
 * APOLLO_CODE_FLAG_ORDER_* flags, falling back to apollo_set_endianness()
 * (see source/patches.c). Nothing in the library branches on it at compile
 * time any more, so ONE binary covers both modes: run_registered_tests()
 * is called once per mode and the vector builders below stamp the matching
 * flag. This used to be two binaries built with -D__PS3_PC__.
 *
 * Vectors whose expected bytes depend on the mode branch on
 * apollo_test_be(). Opcodes that use BE* (search/bulk, always big-endian) or
 * that read pointers with an explicit endianness are mode-INVARIANT: their
 * expected bytes are written once, which asserts that invariance.
 */
int         apollo_test_be(void);           /* 1 while the BE pass runs   */
const char *apollo_test_endian_name(void);  /* "LE" / "BE", for messages  */
void        apollo_test_set_endian(int be); /* runner + corpus entry point */

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
