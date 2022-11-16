/*
  
  Copyright (c) 1997-2021 Victor Lavrenko (v.lavrenko@gmail.com)
  
  This file is part of YARI.
  
  YARI is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  YARI is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.
  
  You should have received a copy of the GNU General Public License
  along with YARI. If not, see <http://www.gnu.org/licenses/>.
  
*/

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include "types.h"

#ifndef MMAP
#define MMAP

typedef struct mmap { 
  char *mode;        // "r" = read-only, "w" = write-only,  "a" = all
  int file;          // file which is mapped
  off_t flen;        // max. size that is valid to map
  off_t offs;        // offset at which the mapped region starts
  off_t size;        // number of bytes in the mapped region
  char *data;        // pointer to the mapped region
  struct mmap *next; // overflow map (for random-access)
} mmap_t;

extern off_t MAP_SIZE;
//extern int MAP_MODE; //  MAP_LOCKED | MAP_NONBLOCK | MAP_POPULATE etc

mmap_t *open_mmap (char *path, char *access, off_t size) ;
void free_mmap (mmap_t *M) ;
void write_mmap (mmap_t *map, char *path) ;
void *move_mmap (mmap_t *map, off_t beg, off_t end) ;
void grow_mmap_file (mmap_t *map, off_t size) ;
void *mmap_region (int fd, off_t offs, off_t size, char *access) ;
void unmap_region (void *region, off_t offs, off_t size);
void expect_random_access (mmap_t *M, off_t size) ;

off_t align8 (off_t x) ;
ulong next_pow2 (ulong x) ;
uint ilog2 (ulong x) ;
uint fast_log2 (unsigned x) ;
off_t page_align (off_t offs, char side) ; // floor:'<'   ceiling:'>'
ulong physical_memory () ;

void *safe_malloc (size_t size) ;
void *safe_calloc (off_t size) ;
void *safe_realloc (void *buf, off_t size) ;
int safe_open (char *path, char *access) ;
FILE *safe_fopen (char *path, char *access) ;
int popen2 (const char *command, pid_t *_pid) ;
FILE *safe_popen (char *access, char *fmt, ...) ;
void nonblock(FILE *f) ;
void *safe_mmap (int fd, off_t offset, off_t size, char *access);
void *safe_remap (int fd, void *buf, off_t osize, off_t nsize);
off_t safe_truncate (int fd, off_t size) ;
off_t safe_lseek (int fd, off_t offs, int whence) ;
off_t safe_read (int fd, void *buf, off_t count) ;
off_t safe_write (int fd, void *buf, off_t count) ;
off_t safe_pread (int fd, void *buf, off_t size, off_t offset) ;
off_t safe_pwrite (int fd, void *buf, off_t size, off_t offset) ;
void stracat (char **dst, int *n, char *src) ;
char *acat (char *s1, char *s2) ; // must free result
char *fmt (char *buf, const char *format, ...) ;
char *fmtn (int sz, const char *format, ...) ;
void zcat (char **buf, int *sz, char *new) ; // realloc + append new to buf+sz
void zprintf (char **buf, int *sz, const char *fmt, ...) ; // realloc + sprintf to buf+sz
void memcat (char **trg, int *used, char *src, int sz) ; // realloc + append to trg+used

char *___itoa (uint i) ;
char *___ftoa (char *fmt, float f) ;
float vtime () ;
int file_exists (char *fmt, ...) ;
off_t file_size (char *fmt, ...) ;
time_t file_modified (char *fmt, ...) ;
void rm_dir (char *dir) ;
void cp_dir (char *src, char *trg) ;
void mv_dir (char *src, char *trg) ; // delete target, rename source
void mkdir_parent (char *_path) ; // unsafe: system
void rmdir_parent (char *path) ; // unsafe: system
void show_spinner () ;
void show_progress (ulong n, ulong N, char *s) ;
double getprm (char *params, char *name, double def) ;
char *getprms (char *params, char *name, char *def, char eol) ;
char *getprmp (char *params, char *name, char *def) ;
char *getarg (int argc, char *argv[], char *arg, char *def) ;

#if (defined __APPLE__ || defined _WIN32 || defined __CYGWIN__)
#define	open64 open
#define	fopen64 fopen
#define	mmap64 mmap
#define	ftruncate64 ftruncate
#define	lseek64 lseek
//#define MAP_SIZE (1<<26)
#else
//#define MAP_SIZE (1<<28)
#endif

#define DEPRECATED(a,b) {						\
    static uint SEEN = 0;						\
    char f = "WARNING: %s is DEPRECATED, please use: %s\n";		\
    if (++SEEN == 1) fprintf(stderr,f,a,b);				\
  }

#endif

//#define off_t long long int


