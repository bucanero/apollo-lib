/*
 * Save Wizard lines inside a BSD script, with no keyword around them:
 *
 *   write at 0x10:00
 *   XXXXXXXX YYYYYYYY     <- a run of these is one Save Wizard code
 *   set range:0,eof
 *
 * The run is handed to apollo_apply_sw_code(), which tokenizes it with plain
 * strtok() -- one hidden position for the whole program. The BSD loop walks its
 * own script with strtok_r() so that cannot move it, and runs the line that
 * ended the run itself. The vectors that put a BSD line AFTER a run are the
 * ones that matter most: if either of those breaks, that line silently never
 * runs, and nothing fails.
 */
#include <stdlib.h>
#include <string.h>
#include "test_common.h"

static size_t apply_bsd(uint8_t** buf, size_t len, const char* codes)
{
    apollo_free_var_list();
    code_entry_t c = make_bsd_code(codes);
    return apollo_apply_bsd_code(buf, len, &c);
}

static uint8_t* zeroed(size_t len)
{
    return calloc(1, len);
}

/* the BSD line that ends the run still runs */
TEST(bsd_sw_lines_script_continues)
{
    uint8_t* buf = zeroed(16);
    size_t n = apply_bsd(&buf, 16,
        "00000004 000000AA\n"
        "write at 8:BB");

    uint8_t exp[16] = {0};
    exp[4] = 0xAA;
    exp[8] = 0xBB;
    CHECK_U64("sw run + bsd: applied", n, 16);
    CHECK_MEM("sw run then write at 8", buf, exp, sizeof(exp));
    free(buf);
}

/* a multi-line SW type, which itself calls strtok(NULL) to read its second
   line, between two BSD lines -- and the code's byte-order flag carries over */
TEST(bsd_sw_lines_multiline_type_and_endianness)
{
    uint8_t* buf = zeroed(16);
    size_t n = apply_bsd(&buf, 16,
        "write at 0xC:CC\n"
        "41000000 00001234\n"
        "40030004 00000001\n"
        "write at 0xF:DD");

    uint8_t exp[16] = {0};
    if (apollo_test_be()) {
        exp[0] = 0x12; exp[1] = 0x34;
        exp[4] = 0x12; exp[5] = 0x35;
        exp[8] = 0x12; exp[9] = 0x36;
    } else {
        exp[0] = 0x34; exp[1] = 0x12;
        exp[4] = 0x35; exp[5] = 0x12;
        exp[8] = 0x36; exp[9] = 0x12;
    }
    exp[0xC] = 0xCC;
    exp[0xF] = 0xDD;
    CHECK_U64("multi-line sw run: applied", n, 16);
    CHECK_MEM("multi-write x3 inside bsd", buf, exp, sizeof(exp));
    free(buf);
}

/* two runs split by a BSD line, the second one ending the script */
TEST(bsd_sw_lines_two_runs_and_last)
{
    uint8_t* buf = zeroed(8);
    size_t n = apply_bsd(&buf, 8,
        "00000000 00000011\n"
        "write at 1:22\n"
        "00000002 00000033\n"
        "00000003 00000044");

    uint8_t exp[8] = {0x11, 0x22, 0x33, 0x44, 0, 0, 0, 0};
    CHECK_U64("two runs: applied", n, 8);
    CHECK_MEM("run, write, run", buf, exp, sizeof(exp));
    free(buf);
}

/* a whole script of SW lines, as a caller that builds the code_entry_t itself
   could pass to the BSD engine */
TEST(bsd_sw_lines_only)
{
    uint8_t* buf = zeroed(4);
    size_t n = apply_bsd(&buf, 4, "00000001 00000055\n00000002 00000066");

    uint8_t exp[4] = {0, 0x55, 0x66, 0};
    CHECK_U64("sw only: applied", n, 4);
    CHECK_MEM("sw only", buf, exp, sizeof(exp));
    free(buf);
}

/* the run sees the buffer as the script left it: here, after an insert grew
   it, so the SW write lands in the new tail */
TEST(bsd_sw_lines_see_current_buffer)
{
    uint8_t* buf = zeroed(4);
    size_t n = apply_bsd(&buf, 4,
        "insert at 4:00000000\n"
        "00000006 000000EE");

    uint8_t exp[8] = {0, 0, 0, 0, 0, 0, 0xEE, 0};
    CHECK_U64("after insert: new size", n, 8);
    CHECK_MEM("sw write past the original end", buf, exp, sizeof(exp));
    free(buf);
}

/* SW starts from its own pointer, not BSD's: a pointer-relative SW write
   (type 0 with the 8 bit set) goes to offset 0 + 2, not 8 + 2 */
TEST(bsd_sw_lines_own_pointer)
{
    uint8_t* buf = zeroed(16);
    apply_bsd(&buf, 16,
        "set pointer:8\n"
        "08000002 00000077");

    uint8_t exp[16] = {0};
    exp[2] = 0x77;
    CHECK_MEM("sw pointer starts at 0", buf, exp, sizeof(exp));
    free(buf);
}

/* indentation is fine, and a comment line does not split the run -- the
   loader strips comments, but a caller passing its own code may not */
TEST(bsd_sw_lines_comments_and_indent)
{
    uint8_t* buf = zeroed(8);
    size_t n = apply_bsd(&buf, 8,
        "  00000003 00000044\n"
        "; max money\n"
        "00000004 00000055");

    uint8_t exp[8] = {0, 0, 0, 0x44, 0x55, 0, 0, 0};
    CHECK_U64("comments: applied", n, 8);
    CHECK_MEM("comment skipped, indented line applied", buf, exp, sizeof(exp));
    free(buf);
}

/* a BSD command with the SW shape stays a BSD command: the SW test is the
   last branch, so only a line no command claimed reaches it */
TEST(bsd_sw_lines_bsd_command_with_sw_shape)
{
    uint8_t* buf = zeroed(0x200);
    size_t n = apply_bsd(&buf, 0x200, "write at 0x100:FF");

    uint8_t exp[0x200] = {0};
    exp[0x100] = 0xFF;
    CHECK_U64("17-char bsd command: applied", n, 0x200);
    CHECK_MEM("written by bsd, not parsed as sw", buf, exp, sizeof(exp));
    free(buf);
}

/* ... including right after a run, where it is the line that ends it: the run
   must hand it back rather than judge it by its shape */
TEST(bsd_sw_lines_sw_shaped_command_ends_run)
{
    uint8_t* buf = zeroed(0x200);
    size_t n = apply_bsd(&buf, 0x200,
        "20000104 0098967F\n"
        "write at 0x100:FF\n"
        "set range:0x10,0x1FF\n"
        "set [csum]:crc32\n"
        "write at 0x0C:[csum]");

    /* the SW 32-bit write follows the byte order; BSD writes bytes as given */
    uint8_t* ref = zeroed(0x200);
    apply_bsd(&ref, 0x200, apollo_test_be() ?
        "write at 0x104:0098967F\n"
        "write at 0x100:FF\n"
        "set range:0x10,0x1FF\n"
        "set [csum]:crc32\n"
        "write at 0x0C:[csum]" :
        "write at 0x104:7F969800\n"
        "write at 0x100:FF\n"
        "set range:0x10,0x1FF\n"
        "set [csum]:crc32\n"
        "write at 0x0C:[csum]");

    CHECK_U64("run + sw-shaped write: applied", n, 0x200);
    CHECK_MEM("same as the bsd-only script", buf, ref, 0x200);
    free(buf);
    free(ref);
}

/* a line BSD does not know is skipped, as it always was -- even with the SW
   shape: here a comment missing its ';' that is exactly 17 characters with a
   space at 8. Rejecting the shape would break scripts that work today. */
TEST(bsd_sw_lines_unknown_sw_shaped_line_skipped)
{
    uint8_t* buf = zeroed(0x200);
    size_t n = apply_bsd(&buf, 0x200,
        "set range:0x10,0x1FF\n"
        "set [csum]:crc32\n"
        "write at 0x100:FF\n"
        "drop the comments");

    uint8_t* ref = zeroed(0x200);
    apply_bsd(&ref, 0x200,
        "set range:0x10,0x1FF\n"
        "set [csum]:crc32\n"
        "write at 0x100:FF");

    CHECK_U64("stray comment: still applied", n, 0x200);
    CHECK_MEM("stray comment: ignored", buf, ref, 0x200);
    free(buf);
    free(ref);
}

/* a placeholder line is not hex, so it ends the run and is then skipped like
   any line BSD does not know: the lines around it still apply, as two runs */
TEST(bsd_sw_lines_placeholder_skipped)
{
    uint8_t* buf = zeroed(8);
    size_t n = apply_bsd(&buf, 8,
        "00000004 000000AA\n"
        "00000005 000000xx\n"
        "00000006 000000BB");

    uint8_t exp[8] = {0, 0, 0, 0, 0xAA, 0, 0xBB, 0};
    CHECK_U64("placeholder: applied", n, 8);
    CHECK_MEM("placeholder line writes nothing", buf, exp, sizeof(exp));
    free(buf);
}

/* a multi-line SW type cut short by a BSD line is rejected by the SW engine's
   own truncation check, and the code with it */
TEST(bsd_sw_lines_truncated_multiline)
{
    uint8_t* buf = zeroed(16);
    size_t n = apply_bsd(&buf, 16,
        "41000000 00001234\n"
        "write at 0xF:DD");

    CHECK_U64("truncated multi-line run: rejected", n, 0);
    free(buf);
}

/* end to end through the loader: the body is not SW-shaped throughout, so the
   code is typed BSD, and the run survives the loader's line handling */
TEST(bsd_sw_lines_through_loader)
{
    char* text = strdup(
        ":SAVE.DAT\n"
        "[Max money + fix checksum]\n"
        "00000004 000000AA\n"
        "set range:0x0,0x7\n"
        "set [h]:crc32big\n"
        "write at 0x8:[h]\n");
    list_t* list = list_alloc();

    apollo_load_code_list(text, list, NULL, NULL);
    free(text);

    code_entry_t* code = list_get(list_head(list));
    CHECK_U64("loader: one code", list_count(list), 1);
    if (code)
    {
        CHECK_U64("loader: typed BSD", code->type, APOLLO_CODE_BSD);

        uint8_t* buf = zeroed(16);
        apollo_free_var_list();
        size_t n = apollo_apply_bsd_code(&buf, 16, code);

        /* reference: the SW write alone, then the checksum over 0..7 */
        uint8_t* ref = zeroed(16);
        apply_bsd(&ref, 16,
            "write at 4:AA\n"
            "set range:0x0,0x7\n"
            "set [h]:crc32big\n"
            "write at 0x8:[h]");

        CHECK_U64("loader code: applied", n, 16);
        CHECK_MEM("sw run + checksum == bsd-only equivalent", buf, ref, 16);
        free(buf);
        free(ref);
    }

    apollo_free_code_list(list, list_head(list));
}
