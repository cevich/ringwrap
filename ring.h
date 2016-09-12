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

#ifndef _RING_H
#define _RING_H

/**************************************************
********************* MACROS
**************************************************/
#define MAXDIRSTRLEN 256 /* characters needed for longest
                            possible directory name*/
#define MAXCOMMANDLEN 1024 /* characters needed for longest 
                              possible command line */

/**************************************************
********************* TYPES
**************************************************/

typedef struct shmseg_s {
    int tracing; /* 0 = not tracing; 1 = tracing; */
    unsigned long wrappedexecutions;
    unsigned long unwrappedexecutions;
    unsigned long keep;
    unsigned long begins;
    unsigned long ends;
    char outdir[MAXDIRSTRLEN];
    char wrapper[MAXCOMMANDLEN];
    char logring[MAXDIRSTRLEN]; /* shared memory char vector, 
                                   num_entries tall  */
} shmseg_t;

typedef struct shared_s {
    char *name; /* name of the shared memory segment & semaphore */
    sem_t *sem; /* semephore struct if open/needed */
    shmseg_t *shmseg; /* shared memory segment structure */
} shared_t;

/**************************************************
********************* FUNCTION DEFINITIONS
**************************************************/

/* Lock / unlock shared data */
void lock_shared(shared_t *shared);
void unlock_shared(shared_t *shared);

/* allocates and returns newly initialized shared_t pointer
   or NULL on failure.  Newly structure returned in LOCKED state. */
shared_t *new_shared(unsigned long keep,
                     const char const *outdir,
                     const char const *wrapper,
                     const char const *cmdbasename,
                     const char const *unique);

/* allocates and returns shared_t pointer for existing 
   semaphore and logring or NULL on failure */
shared_t *get_shared(const char const *cmdbasename,
                     const char const *unique);

/* closes shared data - DOES NOT DESTROY IT */
void free_shared(shared_t *shared);

/* locks, then destroys shared memory segment and semaphore */
void destroy_shared(shared_t *shared);

/* returns pointer to entry in logring, might be pointing at Null */
const char *logring_index(shared_t *shared, size_t index);

/* If logring vector is full, popps off first entry, rotates remaining
   entries, and returns copy of the first.  If logring vector is not full
   appends newentry and returns NULL. Does own locking. */
char *logring_roll(shared_t *shared, const char const *newentry);

/* set shared->tracing = 1. Requires Locking. */
void set_tracing(shared_t *shared);

/* set shared->tracing = 0. Requires Locking. */
void unset_tracing(shared_t *shared);

/* returns 1 if shared->shmseg->tracing is/was 1 otherwise 0 */
int get_tracing(shared_t *shared);

/* Retrieve copy of current logring vector */
char *get_logring_copy(shared_t *shared);

#endif /* _RING_H */
