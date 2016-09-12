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

#ifndef _OPTIONS_H
#define _OPTIONS_H

/**************************************************
********************* TYPES
**************************************************/

typedef enum mode_e {
    MODE_BEGINMODES, /* check value, do not use */
    MODE_STATS, /* print out stats */
    MODE_EXECUTE, /* Execute command, wrapped or otherwise and exit */
    MODE_INIT, /* setup shared stuff or fail if already setup */
    MODE_BEGIN, /* Switch to executing trace command instead */
    MODE_END, /* Switch back to executing command normally */
    MODE_FINI, /* tear down semaphore and shared memory */
    MODE_ENDMODES /* check value, do not use */
} mode_t;

typedef struct options_s {
    mode_t mode; /* MODE_NOMODE < mode < MODE_LASTMODE */
    char *command; /* wrapped command to execute and parameters */
    char *cmdbasename; /* basename of command w/o parameters */
    unsigned long keep; /* number of historical output dirs to preserve */
    char *outdir; /* base directory to use for strace -o option */
    char *wrapper; /* trace command and any parameters */
    char *unique; /* uniquely identifying string */
} options_t;

/**************************************************
********************* MACROS
**************************************************/
#define MAGIC "@@@"
#define DEFAULT_MODE MODE_BEGINMODES
#define DEFAULT_KEEP 10 /* hard coded string in argp_options[] below */
#define DEFAULT_OUTDIR "/tmp/"PROGNAM"-strace/"
#define DEFAULT_COMMAND NULL
#define DEFAULT_WRAPPER "strace -f -ff -t -o @@@"
#define DEFAULT_UNIQUE "X"

/**************************************************
********************* GLOABALS
**************************************************/
const static struct argp_option argp_options[] = {
    { "init",'i', NULL,0,"Initialize shared data.",1 },
    { "keep",'k',"number",0,"Number of output directories to retain", 1 },
    { "",0,NULL,OPTION_DOC,"Default: 10",1 },
    { "outdir", 'o', "path", 0, "Full path to output base directory", 2 },
    { "",0,NULL,OPTION_DOC,"Default: "DEFAULT_OUTDIR,2 },
    { "wrapper", 'w', "command", 0, "Wrapper command and arguments.", 3 },
    { "",0,NULL,OPTION_DOC,"The sequence \""MAGIC"\" will be replaced", 3 },
    { "",0,NULL,OPTION_DOC,"by output filename in the form:", 3 },
    { "",0,NULL,OPTION_DOC,
                     "<outdir>/YYYY-MM-DD_HH:MM:SS_PID-<PID>/<command>", 3 },
    { "",0,NULL,OPTION_DOC,"Default: \""DEFAULT_WRAPPER"\"", 3 },
    { "unique",'u',"string",0,"Keep multiple "PROGNAM"'s from conflicting.",4 },
    { "",0,NULL,OPTION_DOC,"on the same command with differing outdirs", 4 },
    { "begin",'b',NULL,0,"Begin executing with wrapper command.",5 },
    { "end",'e',NULL,0,"End executing with wrapper command.",5 },
    { "fini",'f',NULL,0,"Clean up shared data.",5 },
    { "stats", 's', NULL, 0, "Print statistics.",5},
    { 0 }
};

/**************************************************
********************* FUNCTION DEFINITIONS
**************************************************/

/* clean up options processing data */
void options_fini(void);

/* prints long program usage message to stderr */
void options_showusage(const char const *message);

/* returns pointer to options_t structure after possibly parsing argc/argv 
   or NULL on failure.  Returned structure should not be modified in any way */
options_t *options_get(int argc, const char const * const *argv);

#endif /* _OPTIONS_H */
