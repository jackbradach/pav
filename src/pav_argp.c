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
static void set_op(struct argp_state *state, enum pav_op op);
static bool opts_valid(struct pav_opts *opts);

enum opt_keys {
        OPT_KEY_INVALID = 1,
        OPT_KEY_DECODE,
        OPT_KEY_PLOTPNG,
        OPT_KEY_VERSION = 'V',
        OPT_KEY_VERBOSE = 'v',
        OPT_KEY_IN_FILENAME = 'i',
        OPT_KEY_OUT_FILENAME = 'o',
        OPT_KEY_LOOPS = 'c'
};

enum opt_groups {
        OPT_GROUP_COMMAND = 1,
        OPT_GROUP_REQUIRED,
        OPT_GROUP_OPTIONAL
};

static struct argp_option options[] =
{
    {0, 0, 0, OPTION_DOC, "Commands:", OPT_GROUP_COMMAND},
    {"decode", OPT_KEY_DECODE, 0, 0, "Decode a USART capture"},
    {"plotpng", OPT_KEY_PLOTPNG, 0, 0, "Plot an analog capture to a PNG"},

//    {0, 0, 0, OPTION_DOC, "Requireds:", OPT_GROUP_REQUIRED},

    {0, 0, 0, OPTION_DOC, "Options:", OPT_GROUP_OPTIONAL},
    {"loops", OPT_KEY_LOOPS, "COUNT", OPTION_ARG_OPTIONAL, "Loop a capture COUNT times through the decoder", OPT_GROUP_OPTIONAL},
    {"verbose", OPT_KEY_VERBOSE, 0, OPTION_ARG_OPTIONAL, "Write additional information to stdout", OPT_GROUP_OPTIONAL},

    {0}
};

const char *argp_program_version = "alpha";
const char *argp_program_bug_address = "<jack@bradach.net>";
static char doc[] =
    "Protocol Analyzer Viewer";
static char args_doc[] = "[input] [<output>]";
static struct argp argp = {options, parse_opt, args_doc, doc};

void parse_cmdline(int argc, char *argv[], struct pav_opts *args)
{
        set_defaults(args);
        argp_parse(&argp, argc, argv, 0, 0, args);
}

static void set_defaults(struct pav_opts *opts)
{
        opts->op = PAV_OP_INVALID;
        opts->verbose = false;

        opts->fin = NULL;
        opts->fout = NULL;
        opts->loops = 1;

        /* Is the capture file being piped in? */
        if (!isatty(fileno(stdin)))
            opts->fin = stdin;

        /* Should we pipe out the results? */
        if (!isatty(fileno(stdout))) {
                opts->fout = stdout;
                stdout = freopen(NULL, "wb", stdout);
        }
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct pav_opts *opts = state->input;

    switch (key) {

    case OPT_KEY_VERSION:
        set_op(state, PAV_OP_VERSION);
        break;

    case OPT_KEY_DECODE:
        set_op(state, PAV_OP_DECODE);
        break;

    case OPT_KEY_PLOTPNG:
        set_op(state, PAV_OP_PLOTPNG);
        break;

    case OPT_KEY_VERBOSE:
        g_verbose = true;
        break;

    case OPT_KEY_LOOPS:
        opts->loops = arg ? atoi(arg) : 1;
        break;

    case ARGP_KEY_ARG:
        /* Each argument consumed gets us one step through the if/else tree.
         * If we get too many, it'll puke!
         */
        if (!opts->fin) {
            strncpy(opts->fin_name, arg, 64);
            opts->fin = fopen(arg, "rb");
            if (!opts->fin) {
                fprintf(stderr, "Unable to open input file '%s'!\n", arg);
                argp_state_help(state, stderr, ARGP_HELP_USAGE);
                exit(EXIT_FAILURE);
            }
        } else if(!opts->fout) {
            strncpy(opts->fout_name, arg, 64);
            opts->fout = fopen(arg, "wb");
            if (!opts->fout) {
                fprintf(stderr, "Unable to open ouput file '%s'!\n", arg);
                argp_state_help(state, stderr, ARGP_HELP_USAGE);
                exit(EXIT_FAILURE);
            }
        } else {
            /* BLAARGHGLE! */
            fprintf(stderr, "Too many parameters provided %s!\n", arg);
            argp_state_help(state, stderr, ARGP_HELP_STD_HELP);
            exit(EXIT_FAILURE);
        }

        break;

    case ARGP_KEY_END:
        if (!opts_valid(opts)) {
            argp_state_help(state, stderr, ARGP_HELP_STD_HELP);
            exit(EXIT_FAILURE);
        }
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

/* Check whether the provided options are valid. */
static bool opts_valid(struct pav_opts *opts)
{
    if (!opts->fin) {
        return false;
    }

    /* If input is a pipe (eg, stdin), reopen it as binary. */
    if (stdin == opts->fin) {
        stdin = freopen(NULL, "rb", stdin);
    }

    if (!opts->fout) {
        opts->fout = stdout;
    }

    if (PAV_OP_INVALID == opts->op) {
        fprintf(stderr, "No command selected;  Pick one!\n");
        return false;
    }

    return true;
}

static void set_op(struct argp_state *state, enum pav_op mode)
{
    struct pav_opts *opts = state->input;

    if (PAV_OP_INVALID != opts->op) {
        fprintf(stderr, "Multiple commands specified;  Pick one!\n");
        argp_usage(state);
    }

    opts->op = mode;
}
