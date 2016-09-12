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

#ifndef _RINGWRAP_H
#define _RINGWRAP_H

/**************************************************
********************* TYPES
**************************************************/

typedef enum exitcode_e {
    E_SUCCESS, /* Normal, successful exit */
    E_DONTUSE, /* Needed to preserve common exit_group(1) */
    E_ARGP, /* Error parsing command line arguments */
    E_INIT, /* Error initializing */
    E_NOSHARED, /* shared structure doesn't exist */
    E_OUTDIR, /* Error creating output directory */
    E_RMOUTDIR, /* Error removing output directory */
    E_NOKO, /* both -k and -o were not specified together */
    E_NOCMD, /* No command specified for execution */
} exitcode_t;

/**************************************************
********************* MACROS
**************************************************/
#define TEMPLATE "%F_%T"
#define RMCOMMAND "rm --force --preserve-root --recursive"

/**************************************************
********************* FUNCTION DEFINITIONS
**************************************************/

/* prints out current statistics to stderr */
void print_stats(options_t *options, shared_t *shared);

/* verify both -k and -o options were specified */
int get_ko_result(options_t *options);

/* retrieve shared data and report result */
int get_shared_result(options_t *options, shared_t **shared);

/* initialize shared based on options */
int init(options_t *options, shared_t **shared);

/* returns new <options->outdir>/YYYY-MM-DD_HH:MM:SS_PID/<cmdbasename> 
   creating date/time directory and returning full path or NULL on failure */
char *outputdir(shared_t *shared);

/* recursivly removes path pointed to by delandfree then frees delandfree.
   returns non-zero on failiure */
int deldir(char *delandfree);

/* depending on shared->shmseg->tracing either executes 
   options->command or options->trace options->command returns exit code.
   outdir will be allocated and set to the string of the output directory 
   used */
int execute(options_t *options, shared_t *shared, char **outdir);

/* Fork child process to rotate logs and remove old log directory if needed
   Free's outdir. */
int ringroll(shared_t *shared, char **outdir);

/* Clean up allocated memory */
void fini(shared_t *shared);

/* main program function */
int main(int argc, const char * const * const argv);

#endif /* _RINGWRAP_H */
