#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "apollo.h"        /* full engine types (code_entry_t, list_t, ...) */
#include "apollo_ctrl.h"

/* ---------------------------------------------------------------------------
 * Log plumbing.
 * libapollo calls dbglogger_log() for progress (see source/loader.c: #define
 * LOG dbglogger_log). The CLI defined it to printf; we route it to a sink.
 * ------------------------------------------------------------------------- */
static apctl_log_fn g_log_fn = NULL;
static void         *g_log_ud = NULL;

void apctl_set_log_sink(apctl_log_fn fn, void *ud)
{
    g_log_fn = fn;
    g_log_ud = ud;
}

/* Symbol required by libapollo. Kept variadic to match its call sites. */
void dbglogger_log(const char *fmt, ...)
{
    if (!g_log_fn)
        return;

    char buffer[0x800];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, arg);
    va_end(arg);

    g_log_fn(g_log_ud, buffer);
}

/* MicroPython print() sink (used when the engine is built WITHOUT -DAPOLLO_CLI,
 * which is what we want for the GUI: Python output lands in the log panel,
 * not on stdout). Routes to the same sink as dbglogger_log. */
void dbglogger_printf(const char *fmt, ...)
{
    if (!g_log_fn)
        return;

    char buffer[0x800];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, arg);
    va_end(arg);

    g_log_fn(g_log_ud, buffer);
}

/* ---------------------------------------------------------------------------
 * Session
 * ------------------------------------------------------------------------- */
struct apctl_session {
    list_t         *codes;      /* head node is the synthetic header entry */
    char           *game_name;
    code_entry_t   *header;
    apctl_code_t  *rows;       /* flattened view, count == n_rows          */
    int             n_rows;
};

/* Build the synthetic header entry + flattened rows, mirroring patcher.c. */
static apctl_session_t *build_session(char *data, size_t len, const char *name)
{
    apctl_session_t *s = calloc(1, sizeof(*s));
    if (!s) return NULL;

    /* NUL-terminate the working buffer (apollo_load_code_list mutates it). */
    data = realloc(data, len + 1);
    data[len] = 0;

    code_entry_t *header = calloc(1, sizeof(code_entry_t));
    header->name = (char *)name;
    header->file = (char *)name;

    /* Game name lives after the first ';' up to newline (same as CLI). */
    char *tmp = strchr(data + 1, ';');
    if (tmp) {
        tmp++;
        size_t nlen = strcspn(tmp, "\n");
        s->game_name = malloc(nlen + 1);
        memcpy(s->game_name, tmp, nlen);
        s->game_name[nlen] = '\0';
        for (char *p = s->game_name; p[0]; p++)
            if (*p < ' ') p[0] = ' ';
        header->name = s->game_name;
    }

    s->header = header;
    s->codes  = list_alloc();
    list_append(s->codes, header);
    apollo_load_code_list(data, s->codes, NULL, NULL);
    free(data);

    /* Flatten: skip the header node, number from 1 (CLI-compatible). */
    int total = (int)list_count(s->codes) - 1;
    if (total < 0) total = 0;
    s->rows   = calloc(total ? total : 1, sizeof(apctl_code_t));
    s->n_rows = 0;

    int pos = 1;
    for (list_node_t *node = list_next(list_head(s->codes)); node; node = list_next(node), pos++) {
        code_entry_t *code = list_get(node);
        apctl_code_t *r = &s->rows[s->n_rows++];
        r->id            = pos;
        r->type          = code->type;
        r->flags         = code->flags;
        r->is_parent     = (code->flags & APOLLO_CODE_FLAG_PARENT) ? 1 : 0;
        r->is_child      = (code->flags & APOLLO_CODE_FLAG_CHILD) ? 1 : 0;
        r->activated     = code->activated;
        r->options_count = code->options_count;
        r->name          = code->name;
        r->file          = code->file;
        r->raw           = code;
    }
    return s;
}

apctl_session_t *apctl_open_buffer(const char *buf, size_t len, const char *name)
{
    if (!buf || !len) return NULL;
    char *data = malloc(len);
    if (!data) return NULL;
    memcpy(data, buf, len);
    return build_session(data, len, name ? name : "buffer");
}

apctl_session_t *apctl_open_file(const char *path)
{
    uint8_t *data = NULL;
    size_t   len  = 0;
    if (read_buffer(path, &data, &len) != 0)
        return NULL;
    return build_session((char *)data, len, path);
}

const char *apctl_game_name(apctl_session_t *s)
{
    return s ? (s->game_name ? s->game_name : s->header->name) : "";
}

int apctl_code_count(apctl_session_t *s) { return s ? s->n_rows : 0; }

apctl_code_t *apctl_code_at(apctl_session_t *s, int index)
{
    if (!s || index < 0 || index >= s->n_rows) return NULL;
    return &s->rows[index];
}

const char *apctl_code_text(const apctl_code_t *c)
{
    if (!c || !c->raw || !c->raw->codes) return "";
    return c->raw->codes;
}

/* ---- options ---- */
int apctl_opt_group_count(const apctl_code_t *c)
{
    return c ? c->raw->options_count : 0;
}

const char *apctl_opt_tag(const apctl_code_t *c, int group)
{
    if (!c || group < 0 || group >= c->raw->options_count) return "";
    const char *line = c->raw->options[group].line;
    return line ? line : "";
}

int apctl_opt_value_count(const apctl_code_t *c, int group)
{
    if (!c || group < 0 || group >= c->raw->options_count) return 0;
    return (int)list_count(c->raw->options[group].opts);
}

const char *apctl_opt_value_name(const apctl_code_t *c, int group, int idx)
{
    if (!c || group < 0 || group >= c->raw->options_count) return "";
    option_value_t *v = list_get_item(c->raw->options[group].opts, idx);
    return (v && v->name) ? v->name : "";
}

int apctl_opt_get_selected(const apctl_code_t *c, int group)
{
    if (!c || group < 0 || group >= c->raw->options_count) return -1;
    return c->raw->options[group].sel;
}

void apctl_opt_set_selected(apctl_code_t *c, int group, int idx)
{
    if (!c || group < 0 || group >= c->raw->options_count) return;
    c->raw->options[group].sel = idx;
}

/* ---- data endianness ---- */
static int g_big_endian = 0;

void apctl_set_big_endian(int enabled)
{
    g_big_endian = enabled ? 1 : 0;
    apollo_set_endianness(g_big_endian ? APOLLO_DATA_MODE_BIG : APOLLO_DATA_MODE_DEFAULT);
}

int apctl_get_big_endian(void) { return g_big_endian; }

/* ---- apply ---- */
int apctl_apply(apctl_session_t *s, apctl_code_t *c, const char *target_file)
{
    (void)s;
    if (!c) return 0;
    const char *target = target_file ? target_file : c->raw->file;
    /* apollo_free_var_list() resets the engine to the host's byte order, and it
     * can run between codes, so re-assert the mode on every apply. */
    apollo_set_endianness(g_big_endian ? APOLLO_DATA_MODE_BIG : APOLLO_DATA_MODE_DEFAULT);
    return apollo_apply_code(target, c->raw, NULL) ? 1 : 0;
}

void apctl_reset_vars(void) { apollo_free_var_list(); }

void apctl_close(apctl_session_t *s)
{
    if (!s) return;
    /* list_free walks the engine's code_entry_t entries; the header is ours. */
    if (s->codes) list_free(s->codes);
    free(s->rows);
    free(s->game_name);
    free(s);
}
