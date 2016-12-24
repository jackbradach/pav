/* File: pav_argp.c
 *
 * Protocol Analyzer Viewer - arguments parsing.
 *
 * Author: Jack Bradach <jack@bradach.net>
 *
 * Copyright (C) 2016 Jack Bradach <jack@bradach.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
static void find_demo_capture(struct pav_opts *opts);

enum opt_keys {
        OPT_KEY_INVALID = 1,
        OPT_KEY_DECODE,
        OPT_KEY_PLOTPNG,
        OPT_KEY_GUI,
        OPT_KEY_VERSION = 'V',
        OPT_KEY_VERBOSE = 'v',
        OPT_KEY_IN_FILENAME = 'i',
        OPT_KEY_OUT_FILENAME = 'o',
        OPT_KEY_RANGE_BEGIN = 'b',
        OPT_KEY_RANGE_END = 'e',
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
    {"gui", OPT_KEY_GUI, 0, 0, "Interactive GUI mode"},

//    {0, 0, 0, OPTION_DOC, "Requireds:", OPT_GROUP_REQUIRED},

    {0, 0, 0, OPTION_DOC, "Options:", OPT_GROUP_OPTIONAL},
    {"loops", OPT_KEY_LOOPS, "COUNT", OPTION_ARG_OPTIONAL, "Loop a capture COUNT times through the decoder", OPT_GROUP_OPTIONAL},
    {"begin", OPT_KEY_RANGE_BEGIN, "IDX", OPTION_ARG_OPTIONAL, "Sample range begin (default zero)", OPT_GROUP_OPTIONAL},
    {"end", OPT_KEY_RANGE_END, "IDX", OPTION_ARG_OPTIONAL, "Sample range end (default last sample)", OPT_GROUP_OPTIONAL},
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
        opts->range_begin = 0;
        opts->range_end = 0;

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

    case OPT_KEY_GUI:
        set_op(state, PAV_OP_GUI);
        break;

    case OPT_KEY_RANGE_BEGIN:
        opts->range_begin = atoll(arg);
        break;

    case OPT_KEY_RANGE_END:
        opts->range_end = atoll(arg);
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
        if (!opts->fin) {
            find_demo_capture(opts);
        }

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

static void find_demo_capture(struct pav_opts *opts)
{
    const char demo_path[] = "/usr/share/doc/pav/captures/";
    const char demo_file[] = "uart_analog_115200_50mHz.bin.gz";
    char full[512] = {0};
    FILE *fp;

    strncat(full, demo_path, 512);
    strncat(full, demo_file, 512);

    fp = fopen(full, "rb");
    if (!fp) {
        printf("Unable to find demo capture at < %s >\n", full);
        return;
    }

    opts->fin = fp;
    strncpy(opts->fin_name, demo_file, 512);
}
