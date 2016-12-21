#include <argp.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "pav.h"

bool g_verbose;

static void set_defaults(struct pav_opts *args);
static error_t parse_opt (int key, char *arg, struct argp_state *state);
static void set_mode(struct argp_state *state, enum pav_op op);
static bool opts_valid(struct pav_opts *opts);

enum opt_keys {
        OPT_KEY_VERSION = 'V',
        OPT_KEY_VERBOSE = 'v',
        OPT_KEY_IN_FILENAME = 'i',
        OPT_KEY_OUT_FILENAME = 'o',
};

enum opt_groups {
        OPT_GROUP_REQUIRED = 1,
        OPT_GROUP_COMMAND,
        OPT_GROUP_OPTIONAL
};

static struct argp_option options[] =
{
    {"Commands:", 0, 0, OPTION_DOC, 0, OPT_GROUP_COMMAND},
    {"version", OPT_KEY_VERSION, 0, 0, "Report program version"},

    {0, 0, 0, OPTION_DOC, "Options:", OPT_GROUP_OPTIONAL},
    {"file_in", 'i', "[capture_file]", 0, "Capture file input", OPT_GROUP_REQUIRED},
    {0, 0, 0, OPTION_DOC, "Options:", OPT_GROUP_OPTIONAL},
    {"verbose", OPT_KEY_VERBOSE, 0, OPTION_ARG_OPTIONAL, "Write additional information to stdout", OPT_GROUP_OPTIONAL},
    {0}
};

const char *argp_program_version = "alpha";
const char *argp_program_bug_address = "<jack@bradach.net>";
static char doc[] =
    "Protocol Analyzer Viewer";
static struct argp argp = {options, parse_opt, NULL, doc};

void parse_cmdline(int argc, char *argv[], struct pav_opts *args)
{
        set_defaults(args);
        argp_parse(&argp, argc, argv, 0, 0, args);
}

static void set_defaults(struct pav_opts *opts)
{
        opts->fin = NULL;
        opts->fout = NULL;
        opts->op = PAV_OP_INVALID;
        opts->verbose = false;
        *opts->fin_name = '\0';
        *opts->fout_name = '\0';
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct pav_opts *opts = state->input;
    int rc;

    switch (key) {

    case OPT_KEY_VERSION:
        set_mode(state, PAV_OP_VERSION);
        break;

    case OPT_KEY_VERBOSE:
        g_verbose = true;
        break;

    case ARGP_KEY_ARG:
        argp_usage(state);
        break;

    case ARGP_KEY_END:
        if (!opts_valid(opts))
                argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

/* Check whether the provided options are valid. */
static bool opts_valid(struct pav_opts *opts)
{
    /* Default mode if not specified is 'decode' */
    if (PAV_OP_INVALID == opts->op)
        opts->op = PAV_OP_DECODE;

    return true;
}

static void set_mode(struct argp_state *state, enum pav_op mode)
{
    struct pav_opts *opts = state->input;

    if (PAV_OP_INVALID != opts->op) {
        fprintf(stderr, "Multiple commands specified;  Pick one!\n");
        argp_usage(state);
    }

    opts->op = mode;
}
