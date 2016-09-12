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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "utility.h"

/**************************************************
********************* GLOBALS
**************************************************/
static size_t SIZE_T_CHARS_LEN=-1;

/**************************************************
********************* FUNCTIONS
**************************************************/

char **utility_argvcopy(int argc, const char const * const *argv) {
    char **argvc=NULL;
    
    argvc = malloc((argc+1) * sizeof(char *));
    memset(argvc, 0, (argc+1) * sizeof(char *));
    for(;argc > 0; argc--)
        argvc[argc-1] = utility_strcpy(argv[argc-1]);
    return argvc;
}   

void utility_argvcfree(char **argv) {
    char *argvp=argv[0];
    for(;*argvp != '\0'; argvp++)
        if ((argvp != NULL) && (*argvp != '\0'))
            free(argvp);
    free(argv);
}

char *utility_strcpy(const char const *source) {
    char *newstring = NULL;
    size_t len=0;

    if ((source == NULL) || (*source == '\0'))
        return strdup("");
    len = strlen(source);
    if ((len + 2) >= STR_LEN_MAX)
        return NULL;
    newstring = malloc(len + 2); /* two extra for safety */
    memset(newstring, 0, len + 2);
    memcpy(newstring, source, len); /* Final len+1 and len+2 bytes forced \0 */
    return newstring;
}

char *utility_strcat(const char const *first, const char const *second) {
    char *newstring = NULL;
    size_t len1 = 0;
    size_t len2 = 0;

    if (first == NULL)
        return utility_strcpy(second);
    if (second == NULL)
        return utility_strcpy(first);
    len1 = strlen(first);
    len2 = strlen(second);
    if ((len1 + len2 + 2) >= STR_LEN_MAX)
        return NULL;
    newstring = malloc(len1 + len2 + 2); /* two extra for safety */
    memset(newstring, 0, len1 + len2 + 2);
    memcpy(newstring, first, len1);
    memcpy(newstring + len1, second, len2);
    return newstring;
}

char *utility_strcat3(const char const *first, const char const *second,
                      const char const *third) {
    char *newstring1 = NULL, *newstring2 = NULL;

    newstring1 = utility_strcat(first,second);
    newstring2 = utility_strcat(newstring1,third);
    free(newstring1);
    return newstring2;
}

int utility_is_alnum(char what) {
    if ((what >= ASCII_NUM_MIN) && (what <= ASCII_NUM_MAX))
        return 1;
    if ((what >= ASCII_UC_MIN) && (what <= ASCII_UC_MAX))
        return 1;
    if ((what >= ASCII_LC_MIN) && (what <= ASCII_LC_MAX))
        return 1;
    return 0;
}

char *utility_only_alnum(const char const *source) {
    size_t source_len=0;
    size_t src_counter=0;
    size_t dst_counter=0;
    char *newstr=NULL;
    char *result=NULL;

    if ((source == NULL) || (*source == '\0'))
        return utility_strcpy("");
    source_len = strlen(source);
    newstr = malloc(source_len);
    memset(newstr,0,source_len);
    for (;src_counter < source_len; src_counter++) {
        if (utility_is_alnum(source[src_counter]) == 0) 
            continue;
        newstr[dst_counter] = source[src_counter];
        dst_counter++;
    }
    result = utility_strcpy(newstr); /* only keep allocated what's needed */    
    free(newstr);
    return result;
}

/* TODO: test for needle location at: start, middle, end, */
char *utility_strsnr(char *haystack, 
                     const char const *needle, 
                     const char const *replacement) {
    char *result = NULL;
    char *other_haystack=NULL;

    result = strstr(haystack, needle);
    if ((result == NULL) || (result == haystack))
        return haystack; /* No (more) needles found */
    else { /* found needle */
        *result = '\0'; /* split haystack in two */
        result += strlen(needle); /* point at start of second half */
        /* search remaining string */
        other_haystack = utility_strsnr(result, needle, replacement);
        /* combine result */
        other_haystack = utility_strcat3(haystack, replacement, other_haystack);
    }
    return other_haystack;
}

char *utility_fixpath(const char const *path) {
    char *newpath=NULL; 
    
    if (path[strlen(path) - 1] != '/')
        newpath = utility_strcat(path,"/");
    else
        newpath = utility_strcpy(path);
    return newpath;
}   

char *utility_fullpath(const char const *path, const char const *filename) {
    char *fixedpath=NULL;
    char *fullpath=NULL;
    
    fixedpath = utility_fixpath(path);
    fullpath = utility_strcat(fixedpath,filename);
    free(fixedpath);
    return fullpath;
}

off_t utility_filesize(const char const *pathfile) {
    struct stat s;
    int r=-1;
    const char *e=NULL;

    r = stat(pathfile, &s);
    if (r != 0) {
        e = strerror(errno);
        fprintf(stderr, "Error stat()'ing %s: %s\n", pathfile, e);
        return -1;
    }
    return s.st_size;
}

long utility_ptr_arr_len(const void const **arr) {
    long len=0;

    while (1) {
        if (arr[len] == NULL)
            return len;
        len++;
        if (len > PTR_ARR_MAX) {
            fprintf(stderr, "utility.c bug: Array @ %p is too long\n", arr);
            return -1;
        }
    }
}

unsigned long utility_mem_buffered_cached(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    char *buffer = malloc(1024);
    int index;
    int line = PROC_MEMINFO_CACHED_LINENUMBER;
    unsigned long howmuch=0;

    if (f == NULL)
        return (unsigned long)-1;
    for(;line > 0; line--) {
        fgets(buffer,1024,f);
    }
    for(index = 0; (buffer[index] < 0x30) || (buffer[index] > 0x39); index++);
    howmuch = strtoul(buffer + index, NULL, 0) * 1024; /* units in kb */
    rewind(f);

    line = PROC_MEMINFO_BUFFERS_LINENUMBER;
    for(;line > 0; line--) {
        fgets(buffer,1024,f);
    }
    for(index = 0; (buffer[index] < 0x30) || (buffer[index] > 0x39); index++);
    howmuch += strtoul(buffer + index, NULL, 0) * 1024; /* units in kb */    

    free(buffer);
    fclose(f);    
    return howmuch;
}

unsigned long utility_mem_total(void) {
    return sysconf(_SC_PAGESIZE) * sysconf(_SC_PHYS_PAGES);
}

unsigned long utility_mem_available(void) {
    return (sysconf(_SC_PAGESIZE) * sysconf(_SC_AVPHYS_PAGES)) +
           utility_mem_buffered_cached();
}

unsigned long utility_mem_used(void) {
    return utility_mem_total() - utility_mem_available();
}

char utility_mem_percent_available(void) {
    float roundup=0;
    
    roundup = ((float) utility_mem_available() /
               (float) utility_mem_total()) * 100.0;
    if ((roundup >= 0.0) && (roundup <= 100.0)) {
        return roundup;
    } else {
        return -1;
    }
}

char utility_mem_percent_used(void) {
    return 100 - utility_mem_percent_available();
}


