/*
 * apollo_ctrl - stdio-free controller wrapper around libapollo.
 *
 * This is the shared "engine facade" used by both the GUI and (optionally)
 * a rewritten CLI. It contains NO printf/scanf: all output goes through a
 * log callback, and interactive code "options" are resolved by the caller
 * setting a selection index before applying.
 *
 * The heavy lifting (parsing, patch application, crypto) stays in libapollo;
 * this file only adapts its data model into a UI-friendly, callback-driven API.
 */
#ifndef APOLLO_CTRL_H
#define APOLLO_CTRL_H

#include <stddef.h>
#include "apollo.h"   /* C++-safe: the code_entry_t type and APOLLO_CODE_* /
                         APOLLO_CODE_FLAG_* constants come straight from here. */

#ifdef __cplusplus
extern "C" {
#endif

/* Log sink. `line` is a single, already-formatted message (no trailing newline
 * guaranteed). `ud` is the opaque pointer passed to apollo_set_log_sink(). */
typedef void (*apollo_log_fn)(void *ud, const char *line);

/* Install a process-wide log sink. libapollo emits progress through the global
 * dbglogger_log() symbol (defined in apollo_ctrl.c); this routes it to `fn`.
 * Pass (NULL, NULL) to silence. Not thread-safe; set it before applying. */
void apollo_set_log_sink(apollo_log_fn fn, void *ud);

/* One code row, flattened for display. Pointers are owned by the session and
 * remain valid until apollo_close(). `raw` is the underlying engine entry,
 * needed for option queries and apply. */
typedef struct {
    int            id;             /* 1-based position, matches CLI numbering  */
    int            type;           /* APOLLO_CODE_* (SaveWizard/BSD/Python)    */
    int            flags;          /* APOLLO_CODE_FLAG_* bitmask               */
    int            is_parent;      /* group header                            */
    int            is_child;       /* member of the group above               */
    int            activated;      /* [DEFAULT:] codes start pre-selected     */
    int            options_count;  /* >0 => needs option selection before apply*/
    const char    *name;
    const char    *file;           /* target-file hint from the patch         */
    code_entry_t  *raw;
} apollo_code_t;

typedef struct apollo_session apollo_session_t;

/* Parse a .savepatch. Returns NULL on read/parse failure.
 * `big_endian` is accepted for API symmetry; note the engine's endianness is a
 * COMPILE-TIME switch (__PS3_PC__), so a runtime value here only takes effect
 * if this wrapper was built with the big-endian engine. See README. */
apollo_session_t *apollo_open_file(const char *path);
apollo_session_t *apollo_open_buffer(const char *buf, size_t len, const char *name);

const char       *apollo_game_name(apollo_session_t *s);
int               apollo_code_count(apollo_session_t *s);   /* excludes header */
apollo_code_t    *apollo_code_at(apollo_session_t *s, int index); /* 0-based    */

/* Raw code/script body (Save Wizard lines, BSD commands, or Python source).
 * Owned by the session; may be empty for header/group entries. */
const char *apollo_code_text(const apollo_code_t *c);

/* ---- Interactive options (dropdowns) ---- */
int          apollo_opt_group_count(const apollo_code_t *c);
const char  *apollo_opt_tag(const apollo_code_t *c, int group);
int          apollo_opt_value_count(const apollo_code_t *c, int group);
const char  *apollo_opt_value_name(const apollo_code_t *c, int group, int idx);
int          apollo_opt_get_selected(const apollo_code_t *c, int group);
void         apollo_opt_set_selected(apollo_code_t *c, int group, int idx);

/* Apply one code to `target_file`. Returns 1 on success, 0 on error.
 * Progress is emitted through the installed log sink. If the code has options,
 * set them via apollo_opt_set_selected() first. */
int  apollo_apply(apollo_session_t *s, apollo_code_t *c, const char *target_file);

/* Release engine patch-variable state accumulated across apply() calls.
 * Call once after a batch of applies (mirrors free_patch_var_list()). */
void apollo_reset_vars(void);

void apollo_close(apollo_session_t *s);

#ifdef __cplusplus
}
#endif

#endif /* APOLLO_CTRL_H */
