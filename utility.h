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

#ifndef _UTILITY_H
#define _UTILITY_H

/* users of this need: 
    #include <sys/types.h>
    #include <string.h>
*/

/**************************************************
********************* MACROS
**************************************************/
#define STR_LEN_MAX 1024
#define PTR_ARR_MAX ((unsigned long)-1)
#define PROC_MEMINFO_BUFFERS_LINENUMBER 3
#define PROC_MEMINFO_CACHED_LINENUMBER 4
#define ASCII_NUM_MIN 48 /* 0 */
#define ASCII_NUM_MAX 57 /* 9 */
#define ASCII_UC_MIN 65 /* A */
#define ASCII_UC_MAX 90 /* Z */
#define ASCII_LC_MIN 97 /* a */
#define ASCII_LC_MAX 122 /* z */
#define ASCII_MIN 48
#define ASCII_MAX 122

/**************************************************
********************* TYPES
**************************************************/

extern long int lrintf(float x);

/**************************************************
********************* FUNCTION PROTOTYPES
**************************************************/

/* Make a null-terminated copy of argv that's argc long */
char **utility_argvcopy(int argc, const char const * const *argv);

/* Free null-terminated copy returned by utility_argvcopy */
void utility_argvcfree(char **argv);

/* Returns NULL if source contains no \0 w/in STR_LEN_MAX
   otherwise returns freshly allocated copy of source string */
char *utility_strcpy(const char const *source);

/* Returns concatenation of first+second overwriting \0 from first 
   or NULL on failire */
char *utility_strcat(const char const *first, const char const *second);
char *utility_strcat3(const char const *first, 
                      const char const *second, 
                      const char const *third);

/* Returns 1 if what is a number or letter ASCII character */
int utility_is_alnum(char what);

/* Returns newly allocated copy of source with all non-ASCII 
   letters/numbers removed*/
char *utility_only_alnum(const char const *source);

/* Returns possibly moved pointer to haystack with all instances of needle
   replaced by replacement none of which can be NULL */
char *utility_strsnr(char *haystack,
                     const char const *needle, 
                     const char const *replacement);


/* returns newly allocated path guaranteed to contain trailing / */
char *utility_fixpath(const char const *path);

/* returns newly allocated concatenation of fixedpath and filename */
char *utility_fullpath(const char const *path, const char const *filename);

/* returns the size of path/file or -1 of failure */
off_t utility_filesize(const char const *pathfile);

/* returns number of pointers in null-terminated array if pointers arr 
   does not count the null terminator! */
long utility_ptr_arr_len(const void const **arr);

/* return details on memory in bytes or as a percentage */
unsigned long utility_mem_total(void) __attribute__ ((always_inline));
unsigned long utility_mem_available(void) __attribute__ ((always_inline));
unsigned long utility_mem_used(void) __attribute__ ((always_inline));
char utility_mem_percent_available(void);
char utility_mem_percent_used(void);

#endif /* _UTILITY_H */
