/*
 * Savepatch parsing vectors (apollo_load_code_list / get_patch_code).
 *
 * Parsing is endian-agnostic, so these are single-expectation tests that run
 * identically in the LE and BE builds. Input is assumed well-formed (per the
 * project's staging: valid savepatch text). They pin the parser contract that
 * the rest of the suite — and the CLI/GUI — rely on:
 *
 *   - `:file` headers set the target file for following codes
 *   - `[Name]` starts a code; the trailing `]` and ` ---` are stripped
 *   - code TYPE defaults to Save Wizard and flips to BSD as soon as any
 *     non-comment body line is not `XXXXXXXX YYYYYYYY`
 *   - `[DEFAULT:*]` -> activated, `[INFO:*]` -> ALERT, `[PYTHON:*]` -> Python
 *   - `[GROUP:*]` -> PARENT, following codes -> CHILD
 *   - a name containing `(REQUIRED)` -> REQUIRED
 *   - a body with no code lines (comments only / empty) -> EMPTY
 *   - `;` comment lines are excluded from code->codes
 */
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

/* Build a code list the way the CLI does: one caller-owned header node at the
 * head, then parse. Parsed codes are items 1..N. */
static list_t* parse(const char* text)
{
    char* buf = strdup(text);
    list_t* list = list_alloc();

    code_entry_t* header = calloc(1, sizeof(code_entry_t));
    header->name = strdup("header");
    header->file = strdup("header");
    list_append(list, header);

    apollo_load_code_list(buf, list, NULL, NULL);
    free(buf);   /* names/codes are strdup'd inside the parser */
    return list;
}

static const char* SAMPLE =
    ";CUSA00000\n"
    ";Game Title\n"
    ":SAVE.DAT\n"
    "[Simple SW code]\n"
    "20000004 12345678\n"
    "00000000 000000AB\n"
    "\n"
    "[BSD code]\n"
    "set [x]:0\n"
    "write at 0:AABB\n"
    "\n"
    "[DEFAULT:Active by default]\n"
    "10000000 00001234\n"
    "\n"
    "[INFO:This is an alert]\n"
    "\n"
    "[Weapon (REQUIRED)]\n"
    "20000000 00000001\n"
    "\n"
    "[PYTHON:Run script]\n"
    "print('hi')\n"
    "\n"
    "[GROUP:My Group]\n"
    "20000000 00000002\n"
    "\n"
    "[Child in group]\n"
    "20000000 00000003\n"
    "\n"
    ":OTHER.BIN\n"
    "[Uses other file]\n"
    "; a comment inside the body\n"
    "20000000 00000004\n"
    "\n"
    "[Empty code]\n"
    "; only a comment, no code\n";

/* item indices (0 = caller header) */
enum {
    I_SW = 1, I_BSD, I_DEFAULT, I_INFO, I_REQUIRED,
    I_PYTHON, I_GROUP, I_CHILD, I_OTHERFILE, I_EMPTY
};

TEST(parse_count_and_names)
{
    list_t* l = parse(SAMPLE);
    CHECK_U64("parsed 10 codes (+1 header)", list_count(l), 11);

    code_entry_t* c = list_get_item(l, I_SW);
    CHECK_STR("name strips trailing ]", c->name, "Simple SW code");
}

TEST(parse_type_savewizard_vs_bsd)
{
    list_t* l = parse(SAMPLE);

    code_entry_t* sw  = list_get_item(l, I_SW);
    code_entry_t* bsd = list_get_item(l, I_BSD);

    CHECK_U64("hex-only body -> Save Wizard", sw->type, APOLLO_CODE_SAVEWIZARD);
    CHECK_U64("non-hex body -> BSD", bsd->type, APOLLO_CODE_BSD);
}

/*
 * The Save Wizard type is only assigned when EVERY body line is exactly
 * "XXXXXXXX YYYYYYYY" (17 chars) — the mask in loader.c has no trailing '*', so
 * wildcard_match requires a whole-string match. apollo_apply_sw_code relies on
 * this: it indexes each line at fixed offsets up to line[16].
 */
TEST(parse_sw_type_requires_exact_line_width)
{
    list_t* l = parse(":F.BIN\n"
                      "[Short line]\n"
                      "20000004 123456\n"        /* 15 chars */
                      "\n"
                      "[Long line]\n"
                      "20000004 123456789\n"     /* 18 chars */
                      "\n"
                      "[Exact line]\n"
                      "20000004 12345678\n");

    CHECK_U64("15-char line -> BSD", ((code_entry_t*)list_get_item(l, 1))->type, APOLLO_CODE_BSD);
    CHECK_U64("18-char line -> BSD", ((code_entry_t*)list_get_item(l, 2))->type, APOLLO_CODE_BSD);
    CHECK_U64("17-char line -> Save Wizard", ((code_entry_t*)list_get_item(l, 3))->type, APOLLO_CODE_SAVEWIZARD);
}

TEST(parse_code_body_excludes_comments)
{
    list_t* l = parse(SAMPLE);

    code_entry_t* sw = list_get_item(l, I_SW);
    CHECK_STR("SW body concatenated with newlines",
              sw->codes, "20000004 12345678\n00000000 000000AB\n");

    code_entry_t* other = list_get_item(l, I_OTHERFILE);
    CHECK_STR("comment line dropped from body",
              other->codes, "20000000 00000004\n");
}

TEST(parse_file_association)
{
    list_t* l = parse(SAMPLE);

    code_entry_t* sw    = list_get_item(l, I_SW);
    code_entry_t* other = list_get_item(l, I_OTHERFILE);

    CHECK_STR("code inherits :SAVE.DAT", sw->file, "SAVE.DAT");
    CHECK_STR("later :OTHER.BIN applies", other->file, "OTHER.BIN");
}

TEST(parse_flag_default_activated)
{
    list_t* l = parse(SAMPLE);
    code_entry_t* c = list_get_item(l, I_DEFAULT);

    CHECK_U64("[DEFAULT:*] sets activated", c->activated, 1);
    CHECK_STR("[DEFAULT:*] name", c->name, "Active by default");
}

TEST(parse_flag_info_alert_and_empty)
{
    list_t* l = parse(SAMPLE);
    code_entry_t* c = list_get_item(l, I_INFO);

    CHECK_U64("[INFO:*] sets ALERT", (c->flags & APOLLO_CODE_FLAG_ALERT) != 0, 1);
    CHECK_U64("[INFO:*] empty body -> EMPTY", (c->flags & APOLLO_CODE_FLAG_EMPTY) != 0, 1);
    CHECK_STR("[INFO:*] name", c->name, "This is an alert");
}

TEST(parse_flag_required)
{
    list_t* l = parse(SAMPLE);
    code_entry_t* c = list_get_item(l, I_REQUIRED);

    CHECK_U64("(REQUIRED) in name -> REQUIRED flag",
              (c->flags & APOLLO_CODE_FLAG_REQUIRED) != 0, 1);
}

TEST(parse_type_python)
{
    list_t* l = parse(SAMPLE);
    code_entry_t* c = list_get_item(l, I_PYTHON);

    CHECK_U64("[PYTHON:*] -> Python type", c->type, APOLLO_CODE_PYTHON);
    CHECK_STR("[PYTHON:*] name", c->name, "Run script");
}

TEST(parse_group_parent_child)
{
    list_t* l = parse(SAMPLE);

    code_entry_t* parent = list_get_item(l, I_GROUP);
    code_entry_t* child  = list_get_item(l, I_CHILD);

    CHECK_U64("[GROUP:*] -> PARENT flag",
              (parent->flags & APOLLO_CODE_FLAG_PARENT) != 0, 1);
    CHECK_U64("following code -> CHILD flag",
              (child->flags & APOLLO_CODE_FLAG_CHILD) != 0, 1);
}

TEST(parse_empty_body_flag)
{
    list_t* l = parse(SAMPLE);
    code_entry_t* c = list_get_item(l, I_EMPTY);

    CHECK_U64("comment-only body -> EMPTY", (c->flags & APOLLO_CODE_FLAG_EMPTY) != 0, 1);
}
