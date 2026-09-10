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
 *   - code TYPE is inferred from the body -- Save Wizard only while every
 *     non-comment line is `XXXXXXXX YYYYYYYY`, BSD from the first that is not
 *     -- unless a header states it, and then the header wins
 *   - `[DEFAULT:*]` -> activated, `[INFO:*]` -> ALERT, `[PYTHON:*]` -> Python,
 *     `[SW:*]` -> Save Wizard, `[BSD:*]` -> BSD
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

/* Teardown, the split apollo_free_code_list() documents: the loader's entries
 * go back through the library, the caller-owned header by hand. */
static void free_parsed(list_t* list)
{
    code_entry_t* header = list_get(list_head(list));

    apollo_free_code_list(list, list_next(list_head(list)));
    free(header->name);
    free(header->file);
    free(header);
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

/*
 * Interactive {TAG} options: the option block declares `value=Display` pairs,
 * a code body referencing the tag gets a deep copy of them, and the selection
 * starts at -1 — "not chosen", which is what makes the front-ends block Apply
 * until the user picks rather than silently taking the first value.
 *
 * This is also the one shape that exercises every branch of
 * apollo_free_code_list() (option array, value list, per-value strings), so
 * the teardown runs here and an -fsanitize=address build covers it.
 */
TEST(parse_option_tag_values)
{
    list_t* l = parse(";CUSA00000\n"
                      ";Option Sample\n"
                      ":SAVE.DAT\n"
                      "{Z}001=First;002=Second{/Z}\n"
                      "[Pick a slot]\n"
                      "20000004 000000{Z}\n");

    code_entry_t* c = list_get_item(l, 1);
    CHECK_U64("one {tag} in the body -> one option group", c->options_count, 1);
    CHECK_STR("group keeps the tag verbatim, braces included", c->options[0].line, "{Z}");
    CHECK_U64("both values parsed", list_count(c->options[0].opts), 2);
    CHECK_U64("nothing selected yet", (int64_t)c->options[0].sel, (int64_t)-1);

    option_value_t* first = list_get_item(c->options[0].opts, 0);
    CHECK_STR("left of '=' is the substituted value", first->value, "001");
    CHECK_STR("right of '=' is the display name", first->name, "First");

    free_parsed(l);
}

/*
 * A declared type beats the body's shape.
 *
 * Inference alone cannot be overridden, and it is not always right: a Save
 * Wizard code with a single mistyped line reads as BSD and then fails, and a
 * BSD script whose every line happens to be eight hex digits, a space and
 * eight more reads as Save Wizard. No patch in the database declares a type
 * yet, so these two cases exist nowhere else to test against.
 */
TEST(parse_declared_type_beats_body)
{
    list_t* l = parse(":F.BIN\n"
                      "[SW:Mistyped but still Save Wizard]\n"
                      "20000004 12345678\n"
                      "20000008 1234567\n"        /* 16 chars: would infer BSD */
                      "\n"
                      "[BSD:Hex-shaped but still BSD]\n"
                      "20000004 12345678\n"       /* would infer Save Wizard   */
                      "\n"
                      "[sw:lower case works too]\n"
                      "set [x]:0\n");

    CHECK_U64("[SW:*] survives a non-conforming line",
              ((code_entry_t*)list_get_item(l, 1))->type, APOLLO_CODE_SAVEWIZARD);
    CHECK_U64("[BSD:*] survives a conforming body",
              ((code_entry_t*)list_get_item(l, 2))->type, APOLLO_CODE_BSD);
    CHECK_U64("the prefix is case-insensitive",
              ((code_entry_t*)list_get_item(l, 3))->type, APOLLO_CODE_SAVEWIZARD);

    CHECK_STR("[SW:*] is stripped from the name",
              ((code_entry_t*)list_get_item(l, 1))->name, "Mistyped but still Save Wizard");
    CHECK_STR("[BSD:*] is stripped from the name",
              ((code_entry_t*)list_get_item(l, 2))->name, "Hex-shaped but still BSD");

    free_parsed(l);
}

/* A code the parser can infer nothing from still has to come back with a
 * usable type: front-ends switch on it, and 0 is not one of the three. */
TEST(parse_type_never_zero)
{
    list_t* l = parse(SAMPLE);
    int zeros = 0;

    for (size_t i = 1; i < list_count(l); i++)
        if (((code_entry_t*)list_get_item(l, i))->type == 0) zeros++;

    CHECK_U64("no parsed code has type 0", zeros, 0);
    CHECK_U64("an empty body still reads as Save Wizard",
              ((code_entry_t*)list_get_item(l, I_EMPTY))->type, APOLLO_CODE_SAVEWIZARD);

    free_parsed(l);
}
