/* Headless parity check for apollo_ctrl: lists codes like `patcher <file>`. */
#include <stdio.h>
#include "apollo_ctrl.h"

static void log_sink(void *ud, const char *line) { (void)ud; printf("- %s\n", line); }

static const char *type_name(int t) {
    switch (t) {
        case APOLLO_CODE_BSD:    return "BSD";
        case APOLLO_CODE_PYTHON: return "Python";
        case APOLLO_CODE_SAVEWIZARD: return "Save Wizard";
        default: return "Unknown";
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s file.savepatch\n", argv[0]); return 2; }

    apctl_set_log_sink(log_sink, NULL);   /* comment out for quiet listing */
    apctl_set_log_sink(NULL, NULL);

    apctl_session_t *s = apctl_open_file(argv[1]);
    if (!s) { fprintf(stderr, "Could not open %s\n", argv[1]); return 1; }

    printf("Game: %s\n\n", apctl_game_name(s));
    int n = apctl_code_count(s);
    for (int i = 0; i < n; i++) {
        apctl_code_t *c = apctl_code_at(s, i);
        const char *grp = c->is_parent ? "# " : (c->is_child ? "+--- " : "");
        char info[8] = "";
        if (c->flags & APOLLO_CODE_FLAG_ALERT)    snprintf(info, sizeof info, "[!] ");
        else if (c->flags & APOLLO_CODE_FLAG_EMPTY)    snprintf(info, sizeof info, "[E] ");
        else if (c->flags & APOLLO_CODE_FLAG_DISABLED) snprintf(info, sizeof info, "[D] ");
        else if (c->flags & APOLLO_CODE_FLAG_REQUIRED) snprintf(info, sizeof info, "[R] ");
        printf("%4d. %s%s%s  (%s%s)\n", c->id, grp, info, c->name,
               type_name(c->type), c->options_count ? ", has options" : "");
    }
    printf("\nParse completed: %d codes\n", n);
    apctl_close(s);
    return 0;
}
