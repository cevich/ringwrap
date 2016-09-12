/*
#
# Copyright (C) 2010 by Chris Evich <cevich@redhat.com>
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
*/

#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "version.h"
#include "options.h"
#include "ring.h"
#include "ringwrap.h"
#include "utility.h"

/**************************************************
********************* PRIVATE DEFINITIONS
**************************************************/
options_t *options = NULL;
static void __options_new(void);
static error_t __parser(int key, char *arg, struct argp_state *state);
const char *argp_program_version = PROGVER_s;
const char *argp_program_bug_address = PROGAUTHOR;
static struct argp __argp = { 
    argp_options,
    __parser,
    "\"/path/to/command [arguments]\"",
    "\nWraps \"/path/to/command [arguments]\" when triggered with --begin/-b. "
     " Otherwise, executes it normally.\v"

    "Note: Parameters specified at initialization must also be used during "
    "execution.  If desired, --keep/-k and --outdir/-o must be "
    "specified together.  The --unique/-u parameter is optional.\n"
};

/**************************************************
********************* FUNCTIONS
**************************************************/
static void __options_new(void) {
    options = malloc(sizeof(options_t));
    memset(options, 0, sizeof(options));
    options->mode = DEFAULT_MODE;
    options->command = DEFAULT_COMMAND;
    options->keep = DEFAULT_KEEP + 1;
    options->outdir = utility_fixpath(DEFAULT_OUTDIR);
    options->wrapper = utility_strcpy(DEFAULT_WRAPPER);
    options->unique = utility_strcpy(DEFAULT_UNIQUE);
}

void multimode(void) {
    options_showusage("Multiple modes specified.\n\n");
    exit(E_ARGP);
}

static error_t __parser(int key, char *arg, struct argp_state *state) {
    options_t *options = (options_t *) state->input;

    switch (key) {
        case 's':
            if (options->mode != MODE_BEGINMODES)
                multimode();
            options->mode = MODE_STATS;
            break;
        case 'i':
            if (options->mode != MODE_BEGINMODES)
                multimode();
            options->mode = MODE_INIT;
            break;
        case 'k':
            options->keep = strtoul(arg,NULL,0) + 1;
            break;
        case 'o':
            if (strlen(arg) > 2) {
                free(options->outdir);
                options->outdir = utility_fixpath(arg); /* returns copy */
            } else
                fprintf(stderr,"WARNING: Ignoring outdir %s\n",arg);
            break;
        case 'w':
            if (strlen(arg) > 3) {
                free(options->wrapper);
                options->wrapper = utility_strcpy(arg);
            } else
                fprintf(stderr, "WARNING: Ignoring wrapper %s\n", arg);
            break;
        case 'u':
            if (strlen(arg) > 1) {
                free(options->unique);
                options->unique = utility_only_alnum(arg);
            } else
                fprintf(stderr, "WARNING: Ignoring unique %s\n", arg);
            break;
        case 'b':
            if (options->mode != MODE_BEGINMODES)
                multimode();
            options->mode = MODE_BEGIN;
            break;
        case 'e':
            if (options->mode != MODE_BEGINMODES)
                multimode();
            options->mode = MODE_END;
            break;
        case 'f':
            if (options->mode != MODE_BEGINMODES)
                multimode();
            options->mode = MODE_FINI;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

void options_fini(void) {
    if (options == NULL)
        return;
    free(options->command);
    free(options->outdir);
    free(options->wrapper);
    free(options->unique);
    memset(options, 0, sizeof(options));
    free(options);
    options = NULL;
}

void options_showusage(const char const *message) {
    argp_help(&__argp, stderr, ARGP_HELP_LONG | 
                               ARGP_HELP_SHORT_USAGE,
                               PROGNAM);
    if (message != NULL)
        fprintf(stderr, "\nERROR: %s", message);
}

options_t *options_get(int argc, const char const * const *argv) {
    char **argvc = NULL;
    error_t error=0;
    int arg_index;
    char *oldstr=NULL;

    if (options != NULL)
        return options; /* options already parsed */
    /* parse options */
    __options_new();
    argp_err_exit_status = E_ARGP;
    argvc = utility_argvcopy(argc,argv);
    error = argp_parse(&__argp,argc,argvc,0,&arg_index,(void *)options);
    if (error != 0) 
        return NULL;
    /* default to execute mode */
    if (options->mode == MODE_BEGINMODES)
        options->mode = MODE_EXECUTE;
    /* parse non-option arguments */
    if (argvc[arg_index] != NULL) {
        free(options->cmdbasename);
        options->cmdbasename = utility_only_alnum(argvc[arg_index]);
        free(oldstr);
        options->command = utility_strcpy(argvc[arg_index]);
        arg_index++;
    }
    for(;argvc[arg_index] != NULL;arg_index++) {
        if (strlen(argvc[arg_index]) == 0)
            continue;
        oldstr = options->command;
        options->command = utility_strcat3(options->command, 
                                           " ", 
                                           argvc[arg_index]);
        free(oldstr);
    }
    utility_argvcfree(argvc);
    return options;
}

