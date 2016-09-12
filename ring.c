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

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <stdio.h>
#include "version.h"
#include "utility.h"
#include "ring.h"

/**************************************************
********************* PRIVATE FUNCTION DEFINITIONS
**************************************************/
/* returns newly allocated shm/sem name composed of the concatenation
   of the parameters in the form:  <PROGNVR_s>-<cmdbasename><unique> */
static char *__new_name(const char const *cmdbasename,
                        const char const *unique);

/* Create new shared memory segment and set initial values 
   return NULL if already exists or on failure */
static shmseg_t *__new_shmseg(unsigned long keep,
                              const char const *name,
                              const char const *outdir,
                              const char const *wrapper);

/* returns existing shared memory segment referenced by name 
   or NULL on failure */
static shmseg_t *__get_shmseg(const char const *name);

/* unmapps shared memory segment process address space - DOES NOT DESTROY IT */
static void __free_shmseg(shmseg_t *shmseg);

/* creates a new LOCKED named semaphore and returns it
   or NULL on failure / if one already exists with name */
static sem_t *__new_locked_sem(const char const *name);

/* locks and returns existing named semaphore or NULL on failure */
static sem_t *__get_locked_sem(const char const *name);

/* closes named semaphore, does not destroy it */
static void __free_sem(sem_t *sem);

/* Return pointer to logring element index or NULL if index out of range */
static char *__logring_index(shared_t *shared, size_t index);

/* returns pointer to last empty entry in logring vector
   or NULL if logring full.  */
static char *__logring_endptr(shared_t *shared);

/* returns NULL if logring vector is not full, otherwise
   returns copy of first element, and moves all elements
   up, overwriting the first. */
static char *__logring_pop(shared_t *shared);

/* Allocates memory for new shared_t structure */
static shared_t *__allocate_shared_t(const char const *cmdbasename,
                                     const char const *unique);
/**************************************************
********************* PRIVATE MACROS
**************************************************/
#define SHMSEGLEN(keep) ( sizeof(shmseg_t)+/*one arry included free of charge*/\
                          ((keep - 1) * (MAXDIRSTRLEN)) \
                        )
#define OUTDIRP(shared) (&(shared->shmseg->outdir))
#define WRAPPERP(shared) (&(shared->shmseg->wrapper))
#define LOGRINGP(shared) ((char*) &(shared->shmseg->logring))

/**************************************************
********************* PRIVATE FUNCTIONS
**************************************************/
static char *__new_name(const char const *cmdbasename,
                        const char const *unique) {

    return utility_strcat3(PROGVERXY_s"-", cmdbasename, unique);
}

static shmseg_t *__new_shmseg(unsigned long keep,
                              const char const *name,
                              const char const *outdir,
                              const char const *wrapper) {
    shmseg_t *newone=NULL;
    int fd=0;

    if ((keep < 1) || (wrapper == NULL) || (name == NULL))
        return NULL; /* guarantee it's always one */
    if (outdir == NULL)
        keep = 1; /* force to empty logring */
    fd = shm_open(name,
                  O_RDWR | O_CREAT | O_EXCL,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (fd > 0) {
        ftruncate(fd,SHMSEGLEN(keep));
        newone = mmap(NULL, SHMSEGLEN(keep),
                      PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd, 0);
        close(fd);
        if (newone != MAP_FAILED) {
            /* guarantee null terminators */
            if (outdir != NULL)
                strncpy(newone->outdir, outdir, MAXDIRSTRLEN - 1);
            strncpy(newone->wrapper, wrapper, MAXCOMMANDLEN - 1);
            newone->keep = keep;
            /* ftruncate guarantees everything else is zeros */
            return newone;
        }
    }
    return NULL;
}


static shmseg_t *__get_shmseg(const char const *name) {
    shmseg_t *shmseg=NULL;
    int fd;
    unsigned long keep=0;
    
    fd = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (fd > 0) {
        /* need to mmap it twice to get stored size */
        shmseg = mmap(NULL, sizeof(shmseg_t), 
                      PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd, 0);
        if (shmseg != MAP_FAILED) {
            keep = shmseg->keep;
            munmap(shmseg, sizeof(shmseg_t));
            if (keep > 0) { /* must always be at least 1 */
                shmseg = (shmseg_t *) mmap(NULL, SHMSEGLEN(keep),
                                           PROT_READ | PROT_WRITE, MAP_SHARED,
                                           fd, 0);
                if (shmseg != MAP_FAILED)
                    return shmseg;
            }
        }
        close(fd);
    }
    return NULL;
}

static void __free_shmseg(shmseg_t *shmseg) {
    unsigned long keep=0;

    if (shmseg != NULL) {
        keep = shmseg->keep;
        munmap(shmseg, SHMSEGLEN(keep));
    }
}

static sem_t *__new_locked_sem(const char const *name) {
    sem_t *sem=NULL;

    sem = sem_open(name, 
                   O_CREAT | O_EXCL,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
                   0);
    if (sem == SEM_FAILED)
        return NULL;
    return sem;
}

static sem_t *__get_locked_sem(const char const *name) {
    sem_t *sem=NULL;

    sem = sem_open(name, 0);
    if (sem == SEM_FAILED)
        return NULL;
    sem_wait(sem);
    return sem;
}

static void __free_sem(sem_t *sem) {
    sem_close(sem);
}

static char *__logring_index(shared_t *shared, size_t index) {
    size_t offset=0;
    char *entry=NULL;

    if (index >= (shared->shmseg->keep - 1)) /* at/beyond terminator index*/
        return NULL;
    offset = index * MAXDIRSTRLEN;
    entry = LOGRINGP(shared) + offset;
    return entry;
}

char *__logring_endptr(shared_t *shared) {
    size_t counter = shared->shmseg->keep - 2; /*last possible non-term entry */
    char *endptr = __logring_index(shared, counter);
    
    /* searching backwards is the fastest assuming full log */
    while( (*endptr == '\0') && (counter > 0) ) {
        counter--;
        endptr = __logring_index(shared, counter);
    }
    if ( counter >= (shared->shmseg->keep - 2) )
        return NULL; /* logring is full */
    else if (*endptr != '\0') /* return next, but not last entry) */
        return __logring_index(shared, counter + 1);
    else /* log is totally empty, return first entry */
        return LOGRINGP(shared); /* log is entirely empty */
}

static char *__logring_pop(shared_t *shared) {
    char *endptr=NULL;
    char *entry2=NULL;
    char *popped=NULL;
    size_t copylength=0;

    /* point at second to last entry */
    endptr = __logring_index(shared, shared->shmseg->keep - 2);
    /* Check if logring is full */
    if (*endptr == '\0')
        return NULL; /* not full, nothing to pop */
    /* make copy of entry to return */
    popped = utility_strcpy( __logring_index(shared, 0)  );
    /* point at second entry */
    entry2 = __logring_index(shared, 1);
    /* -1 means move trailing terminator entry as well */
    copylength = (shared->shmseg->keep - 1) * MAXDIRSTRLEN;
    /* copy entry #2 through/including trailing terminator, one position up */
    memmove( LOGRINGP(shared), entry2, copylength);
    /* guarantee last two entries are full of \0's */
    endptr = __logring_index(shared, shared->shmseg->keep - 2);
    memset(endptr, 0, MAXDIRSTRLEN * 2);
    return popped;
}

static shared_t *__allocate_shared_t(const char const *cmdbasename,
                                     const char const *unique) {
    shared_t *shared=NULL;

    shared = malloc(sizeof(shared_t));
    memset(shared,0,sizeof(shared_t));
    shared->name = __new_name(cmdbasename, unique);
    return shared;
}

/**************************************************
********************* FUNCTIONS
**************************************************/

void lock_shared(shared_t *shared) {
    sem_wait(shared->sem);
}

void unlock_shared(shared_t *shared) {
    sem_post(shared->sem);
}

shared_t *new_shared(unsigned long keep,
                     const char const *outdir,
                     const char const *wrapper,
                     const char const *cmdbasename,
                     const char const *unique) {
    shared_t *newone=NULL;

    if ((cmdbasename == NULL) || (unique == NULL))
        return NULL;
    newone = __allocate_shared_t(cmdbasename,unique);
    newone->sem = __new_locked_sem(newone->name);
    if (newone->sem != NULL) {
        newone->shmseg = __new_shmseg(keep, newone->name, outdir, wrapper);
        unlock_shared(newone);
        if (newone->shmseg != NULL) 
            return newone;
    }
    /* failed to create semaphore or shared memory */
    free_shared(newone); /* only free memory */
    return NULL;
}

shared_t *get_shared(const char const *cmdbasename,
                     const char const *unique) {
    shared_t *newone=NULL;
    
    newone = __allocate_shared_t(cmdbasename,unique);
    newone->sem = __get_locked_sem(newone->name);
    if (newone->sem != NULL) {
        newone->shmseg = __get_shmseg(newone->name);
        unlock_shared(newone); /* unlock */
        if (newone->shmseg != NULL) {
            return newone;
        }
    }
    free_shared(newone); /* only free memory */
    return NULL;
}

void free_shared(shared_t *shared) {
    if (shared != NULL) {
        __free_sem(shared->sem);
        __free_shmseg(shared->shmseg);
        free(shared->name);
        memset(shared,0,sizeof(shared_t));
    }
}

void destroy_shared(shared_t *shared) {
    char *name_copy=NULL;

    if (shared != NULL) {
        name_copy = utility_strcpy(shared->name); /* free_shared frees name */
        free_shared(shared);
        shm_unlink(name_copy);
        sem_unlink(name_copy);
        free(name_copy);      
    }
}

const char *logring_index(shared_t *shared, size_t index) {
    return (const char *)__logring_index(shared,index);
}

char *logring_roll(shared_t *shared, const char const *newentry) {
    char *popped=NULL;
    char *endptr=NULL;
    char *newentrycopy=NULL;

    if ((shared == NULL) || (shared->shmseg->keep < 3) || (newentry == NULL))
        return NULL; /* nothing to do */
    lock_shared(shared);
    endptr = __logring_endptr(shared);
    if (endptr == NULL) { /* log is FULL */
        popped = __logring_pop(shared); /* also rotates entries */
        endptr = __logring_endptr(shared); /* get new end */
    }
    newentrycopy = malloc(MAXDIRSTRLEN);
    memset(newentrycopy,0,MAXDIRSTRLEN);
    /* guarantee newentry size and terminating NULL */
    strncpy(newentrycopy, newentry, MAXDIRSTRLEN - 1);
    memcpy(endptr,newentrycopy,MAXDIRSTRLEN);
    unlock_shared(shared);
    free(newentrycopy);
    return popped;
}

void set_tracing(shared_t *shared) {
    if (shared != NULL) {
        shared->shmseg->tracing = 1;
        shared->shmseg->begins += 1;
    }
}

void unset_tracing(shared_t *shared) {
    if (shared != NULL) {
        shared->shmseg->tracing = 0;
        shared->shmseg->ends += 1;
    }
}

int get_tracing(shared_t *shared) {
    if ((shared == NULL) || 
        (shared->shmseg == NULL) || 
        (shared->shmseg->tracing == 0))
        return 0;
    else
        return 1;
}


