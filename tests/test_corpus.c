/*
 * Golden corpus regression.
 *
 * Applies every code from a tree of real .savepatch files to a fixed,
 * deterministic synthetic buffer and prints a stable manifest line per code:
 *
 *     <relpath>#<index>\t<type>\t<status>\t<outsize>\t<hash>
 *
 * The manifest is captured now (from the current LE and BE builds) as the
 * frozen reference. After the endian refactor, re-running must reproduce the
 * SAME manifest byte-for-byte — that is the regression guarantee.
 *
 * Robustness:
 *   - Each CODE is applied in a forked child, so an out-of-range code that
 *     corrupts the heap or segfaults yields exactly one CRASH line and never
 *     affects any other code (the same code crashes identically before/after,
 *     so CRASH is itself a stable, comparable outcome). Every code therefore
 *     produces exactly one manifest line.
 *   - Codes are driven through the public apply_cheat_patch_code() entry point
 *     (via a temp file), so the host callback and the real file path are
 *     exercised exactly as production does.
 *   - Python codes and offzip-extracted targets are skipped (they need an
 *     interpreter / external state and are out of scope for endian testing).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "test_common.h"

/* deterministic baseline: 2 MiB covers realistic save offsets; larger,
 * pointer-derived offsets on synthetic data either miss (search fails) or
 * crash (isolated per-file). */
#define CORPUS_BUF_SIZE   0x200000u
#define CORPUS_SEED       0x1234abcdu

extern void* apollo_test_host_cb(int info, uint32_t* size);

static const char* g_tmp_path;   /* per-process temp target file */

/* ---- file list collection ------------------------------------------------ */

typedef struct { char** items; size_t count, cap; } strlist_t;

static void sl_push(strlist_t* sl, const char* s)
{
    if (sl->count == sl->cap) {
        sl->cap = sl->cap ? sl->cap * 2 : 128;
        sl->items = realloc(sl->items, sl->cap * sizeof(char*));
    }
    sl->items[sl->count++] = strdup(s);
}

static int cmp_str(const void* a, const void* b)
{
    return strcmp(*(const char* const*)a, *(const char* const*)b);
}

static void collect(const char* dir, strlist_t* out)
{
    DIR* d = opendir(dir);
    if (!d) return;

    struct dirent* e;
    char path[4096];
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);

        struct stat st;
        if (stat(path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            collect(path, out);
        } else {
            size_t n = strlen(e->d_name);
            if (n > 10 && strcmp(e->d_name + n - 10, ".savepatch") == 0)
                sl_push(out, path);
        }
    }
    closedir(d);
}

/* ---- per-code application ------------------------------------------------ */

static int write_baseline(const uint8_t* baseline)
{
    FILE* f = fopen(g_tmp_path, "wb");
    if (!f) return -1;
    size_t w = fwrite(baseline, 1, CORPUS_BUF_SIZE, f);
    fclose(f);
    return (w == CORPUS_BUF_SIZE) ? 0 : -1;
}

/* apply one code in the current (child) process and print its manifest line */
static void apply_one(const char* rel, int idx, code_entry_t* code,
                      const uint8_t* baseline)
{
    if (write_baseline(baseline) != 0) {
        printf("%s#%d\t%d\tsetup-err\t-\t-\n", rel, idx, code->type);
        return;
    }

#if APOLLO_TEST_ENDIAN_BE
    code->flags = APOLLO_CODE_FLAG_ORDER_BE;
#endif
    size_t out = apply_cheat_patch_code(g_tmp_path, code, apollo_test_host_cb);
    if (out == 0) {
        printf("%s#%d\t%d\tnoop\t0\t-\n", rel, idx, code->type);
        return;
    }

    uint8_t* result;
    size_t rlen;
    if (read_buffer(g_tmp_path, &result, &rlen) != 0) {
        printf("%s#%d\t%d\tread-err\t-\t-\n", rel, idx, code->type);
        return;
    }
    printf("%s#%d\t%d\tok\t%zu\t%016llx\n",
           rel, idx, code->type, rlen,
           (unsigned long long)fnv1a(result, rlen));
    free(result);
}

/* rel = path with the root prefix stripped, for root-independent manifests */
static void process_file(const char* path, const char* rel, const uint8_t* baseline)
{
    uint8_t* data;
    size_t len;
    if (read_buffer(path, &data, &len) != 0) {
        printf("%s#*\t-\tparse-read-err\t-\t-\n", rel);
        return;
    }

    data = realloc(data, len + 1);
    data[len] = 0;

    list_t* codes = list_alloc();
    code_entry_t* header = calloc(1, sizeof(code_entry_t));
    header->name = (char*)rel;
    header->file = (char*)g_tmp_path;   /* target every code at our temp file */
    list_append(codes, header);

    load_patch_code_list((char*)data, codes, NULL, NULL);

    int idx = 0;
    list_node_t* node = list_head(codes);
    for (node = list_next(node); ; node = list_next(node)) {
        code_entry_t* code = list_get(node);
        if (!code) break;
        idx++;

        code->file = (char*)g_tmp_path;

        if (code->type == APOLLO_CODE_PYTHON) {
            printf("%s#%d\t%d\tskip-python\t-\t-\n", rel, idx, code->type);
            continue;
        }
        if (code->flags & APOLLO_CODE_FLAG_EMPTY) {
            printf("%s#%d\t%d\tempty\t-\t-\n", rel, idx, code->type);
            continue;
        }

        /* isolate every code: a crash costs only this one line */
        fflush(stdout);
        pid_t pid = fork();
        if (pid == 0) {
            apply_one(rel, idx, code, baseline);
            fflush(stdout);
            _exit(0);
        } else if (pid > 0) {
            int st;
            waitpid(pid, &st, 0);
            if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) {
                int sig = WIFSIGNALED(st) ? WTERMSIG(st) : -1;
                printf("%s#%d\t%d\tCRASH(sig=%d)\t-\t-\n", rel, idx, code->type, sig);
            }
        } else {
            printf("%s#%d\t%d\tfork-err\t-\t-\n", rel, idx, code->type);
        }
    }

    list_free(codes);
    free(data);
    fflush(stdout);
}

/* ---- driver -------------------------------------------------------------- */

int corpus_run(const char* root)
{
    static char tmpl[4096];
    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir || !*tmpdir) tmpdir = "/tmp";
    snprintf(tmpl, sizeof(tmpl), "%s/apollo_corpus_%d.bin", tmpdir, (int)getpid());
    g_tmp_path = tmpl;

    uint8_t* baseline = malloc(CORPUS_BUF_SIZE);
    fill_lcg(baseline, CORPUS_BUF_SIZE, CORPUS_SEED);

    strlist_t files = {0};
    collect(root, &files);
    qsort(files.items, files.count, sizeof(char*), cmp_str);

    size_t rootlen = strlen(root);
    fprintf(stderr, "[corpus %s] %zu files under %s\n",
            APOLLO_TEST_ENDIAN_NAME, files.count, root);

    for (size_t i = 0; i < files.count; i++) {
        const char* path = files.items[i];
        const char* rel = path;
        if (strncmp(path, root, rootlen) == 0) {
            rel = path + rootlen;
            while (*rel == '/') rel++;
        }

        /* per-code forking inside process_file provides crash isolation;
         * parsing is trusted (valid savepatch input, per project scope). */
        process_file(path, rel, baseline);
    }

    unlink(g_tmp_path);
    free(baseline);
    return 0;
}
