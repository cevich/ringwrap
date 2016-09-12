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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <argp.h>
#include "version.h"
#include "utility.h"
#include "ring.h"
#include "options.h"
#include "ringwrap.h"

/**************************************************
********************* FUNCTIONS
**************************************************/

void print_logring(shared_t *shared) {
    unsigned long counter=0;
    const char *log;

    for(;(counter < shared->shmseg->keep - 1) &&
         (counter < 5); counter++) {
        log = logring_index(shared,counter);
        fprintf(stderr,"%-4lu: ", counter);
        if ((log == NULL) || (*log == '\0')) {
            fprintf(stderr, "(empty)\n");
            break;
        }
        else
            fprintf(stderr, "%s\n", log);
    }
}

void print_stats(options_t *options, shared_t *shared) {
    fprintf(stderr, "Statistics:\n");
    fprintf(stderr, "\tCommand: %s\n", options->command);
    fprintf(stderr, "\tCommand Hash: %s%s\n", options->cmdbasename,
                                              options->unique);
    if (get_tracing(shared) == 1) 
        fprintf(stderr, "\tWrapping currently: ON\n");
    else
        fprintf(stderr, "\tWrapping currently: OFF\n");
    fprintf(stderr, "\tUnwrapped Executions: %lu\n", 
                       shared->shmseg->unwrappedexecutions);
    fprintf(stderr, "\tWrapper Command: %s\n", shared->shmseg->wrapper);
    fprintf(stderr, "\tWrapped Executions: %lu\n", 
                       shared->shmseg->wrappedexecutions);
    fprintf(stderr, "\tOutdir: %s\n", shared->shmseg->outdir);
    fprintf(stderr, "\tKeep: %lu\n", shared->shmseg->keep - 1);
    fprintf(stderr, "\tBegins: %lu\n", shared->shmseg->begins);
    fprintf(stderr, "\tEnds: %lu\n", shared->shmseg->ends);
    fprintf(stderr, "\n");
    fprintf(stderr, "Logring:\n");
    print_logring(shared);
}

int get_ko_result(options_t *options) {
    if ( ((options->outdir == NULL) && (options->keep > 2)) ||
         ((options->outdir != NULL) && (options->keep < 3)) ) {
        fprintf(stderr, "ERROR: You must specify -k/--keep and -o/--outdir "
                          "parameters together.\nKeep must be more than 1.\n");
        return E_NOKO;
    } else if ( (options->outdir != NULL) && 
                (options->keep > 2) && 
                (strstr(options->wrapper, MAGIC) == NULL)
              ) {
        fprintf(stderr, "WARNING: -k/--keep and -o/--outdir specified without %s in wrapper command!\n", MAGIC);
        /* make noise but not fatal problem */
    }
    return E_SUCCESS;
}

int get_shared_result(options_t *options, shared_t **shared) {
    *shared = get_shared(options->cmdbasename,
                         options->unique);
    if (*shared != NULL) {
        return E_SUCCESS;
    } else {
        fprintf(stderr,"Failed to obtain shared data, maybe command or "
                          "parameters differ from\nthose used at original "
                          "initialization?\n\n");
        return E_NOSHARED;
    }
}

int init(options_t *options, shared_t **shared) {
    int result = E_INIT; /* failure by default */

    switch (options->mode) {
        case MODE_STATS:
            if ( (result = get_shared_result(options,shared)) == E_SUCCESS )
                print_stats(options, *shared);
            break;
        case MODE_INIT:
            result = get_ko_result(options);
            if ( (result == E_SUCCESS) && (options->outdir != NULL) &&
                (strstr(options->wrapper, MAGIC) != NULL) ) {
                result = mkdir(options->outdir, S_IRWXU | S_IRWXG);
                if (result != 0)
                    fprintf(stderr, 
                            "WARNING: Create directory %s: %s\n",
                            options->outdir, strerror(errno));
                result = E_SUCCESS;
            } 
            if (result == E_SUCCESS) { /* manditory get_ko_result() success */
                *shared = new_shared(options->keep, 
                                     options->outdir,
                                     options->wrapper,
                                     options->cmdbasename,
                                     options->unique);
                if (*shared != NULL) { /* successful */
                    unlock_shared(*shared);
                    fprintf(stderr,"Successfully initialized shared data\n");
                    result = E_SUCCESS;
                } else {
                    options_showusage("Failed to initialize shared data, "
                                      "maybe it was already initialized?\n");
                    result = E_NOSHARED;
                }
            }
            break;
        case MODE_FINI:
            if ( (result = get_shared_result(options,shared)) == E_SUCCESS ) {
                lock_shared(*shared); /* be kind to others */
                destroy_shared(*shared);
                *shared = NULL;
                fprintf(stderr,"Successfully destroyed shared data "
                               "(preserving any logged output)\n");
            }
            break;
        case MODE_BEGIN:
            get_ko_result(options); /* print warning if needed */
            if ((result = get_shared_result(options,shared)) == E_SUCCESS) {
                lock_shared(*shared);
                if (get_tracing(*shared) == 0) {
                    set_tracing(*shared);
                    fprintf(stderr, "Switched tracing on\n");
                }
                unlock_shared(*shared);
            }
            break;
        case MODE_END:
            get_ko_result(options); /* print warning if needed */
            if ((result = get_shared_result(options,shared)) == E_SUCCESS) {
                lock_shared(*shared);
                if (get_tracing(*shared) == 1) {
                    unset_tracing(*shared);
                    fprintf(stderr, "Switched tracing off\n");
                }
                unlock_shared(*shared);
            }
            break;
        case MODE_EXECUTE:
            get_ko_result(options); /* print warning if needed */
            if (options->command == NULL) {
                options_showusage("No Command specified\n");
                result = E_NOCMD;
            } else
                /* Checks for NULL return later */
                *shared = get_shared(options->cmdbasename,
                                     options->unique);
                result = E_SUCCESS; /* always succeeds */
            break;
        default:
            result = E_INIT;
    }
    return result;
}

char *outputdir(shared_t *shared) {
    pid_t pid = getpid();
    size_t length=0;
    char *template=NULL;
    char *outdir=NULL;
    struct tm brokentime = { 0 };
    int result=0;
    time_t t;

    if (shared->shmseg->outdir != NULL) {
        template = malloc(1024);
        snprintf(template, 1024, "%s%s_PID-%u", shared->shmseg->outdir, 
                 TEMPLATE, pid);
        t = time(NULL);
        localtime_r(&t, &brokentime);
        length = strftime(NULL, -1, template, &brokentime);
        outdir = malloc(length + 1);
        strftime(outdir, (length + 1), template, &brokentime);
        free(template);
        result = mkdir(outdir, S_IRWXU | S_IRWXG);
        if (result != 0) {
            fprintf(stderr,
                    "ERROR: Create directory %s: %s\n",
                    outdir, strerror(errno));
            free(outdir);
            return NULL;
        }
        return outdir;
    } else
        return NULL;
}

int deldir(char *delandfree) {
    char *cmd=NULL;
    int result=0;

    if (delandfree != NULL) {
        cmd = utility_strcat3(RMCOMMAND, " ", delandfree);
        result = WEXITSTATUS(system(cmd));
        if (result != 0) {
            fprintf(stderr, "ERROR: %s removal failed.\n", delandfree);
        }
        free(delandfree);
    }
    return result;
}

int execute(options_t *options, shared_t *shared, char **outdir) {
    int result;
    char *cmd=NULL;
    char *outfile=NULL;

    if (shared != NULL) {
        lock_shared(shared);
        if (get_tracing(shared) == 1) {
            cmd = utility_strcat3(shared->shmseg->wrapper," ",options->command);
            if ( (shared->shmseg->keep > 2) && /* assume outdir was set */
                 (strstr(shared->shmseg->wrapper, MAGIC) != NULL)) {
                *outdir = outputdir(shared); /* creates directory also */
                if (*outdir == NULL) /* catch creation errors */
                    return E_OUTDIR;
                outfile = utility_fullpath(*outdir, options->cmdbasename);
                /* do the @@@ substitution */
                cmd = utility_strsnr(cmd, MAGIC, outfile);
                free(outfile);
                /* retain outdir for deldir()*/
            } else /* No magic outdir substitution needed */
                cmd = utility_strcat3(shared->shmseg->wrapper, " ", 
                      options->command);
        } else
            cmd = utility_strcpy(options->command);
        unlock_shared(shared);
    } else
        cmd = utility_strcpy(options->command);
    /* execute the command as a child process outside any locks */
    result = WEXITSTATUS(system(cmd));
    free(cmd);
    return result;
}

int ringroll(shared_t *shared, char **outdir) {
    int result=0;
    pid_t forkresult=-1;

    forkresult = fork();
    if (forkresult == 0) { /* This is the child*/
        /* Acquire lock and Increment counters */
        if (shared != NULL) {
            lock_shared(shared);
            if (get_tracing(shared) == 1)
                shared->shmseg->wrappedexecutions += 1;
            else
                shared->shmseg->unwrappedexecutions += 1;
            unlock_shared(shared);
            /* Rotate output directories if needed - ignores outdir=NULL 
               logring_roll does locking */
            if ( deldir( logring_roll(shared,*outdir) ) != 0 )
                result = E_RMOUTDIR; /* there was a problem */
        }
        return result;
    } else /* This is the parent */
        return 0;
}

void fini(shared_t *shared) {
    free_shared(shared);
    options_fini(); /* options struct pointer is static */
}

int main(int argc, const char * const * const argv) {
    char *outdir=NULL;
    options_t *options=NULL;
    shared_t *shared=NULL;
    exitcode_t exitcode=E_SUCCESS;

    options = options_get(argc, argv);
    if (options == NULL)
        exitcode = E_ARGP;
    else
        exitcode = init(options,&shared);
    if ((exitcode == E_SUCCESS) && (options->mode == MODE_EXECUTE)) {
        exitcode = execute(options, shared, &outdir); /* allocates outdir */
        if (exitcode == E_SUCCESS) 
            exitcode = ringroll(shared, &outdir);
        free(outdir);
    }
    else if (exitcode == E_SUCCESS)
        fprintf(stderr,"(no command was executed)\n");
    fini(shared);
    return exitcode;
}
