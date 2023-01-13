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

#define _GNU_SOURCE // necessary for safe_mremap
#include "mmap.h"

//void warn (char *s) { fprintf (stderr, "%s\n", s); }

off_t MAP_SIZE = 1<<30;
//int MAP_MODE = 0; //  MAP_LOCKED | MAP_NONBLOCK | MAP_POPULATE

mmap_t *open_mmap (char *path, char *access, off_t size) {
  mmap_t *M = safe_calloc (sizeof (mmap_t));
  M->mode = access;
  M->file = safe_open (path, access);
  M->flen = safe_lseek (M->file, 0, SEEK_END);
  M->offs = 0;
  //if (!size) size = MAX(M->flen,1<<30);
  size = MAX(M->flen,1<<30);
  M->size = page_align (size,'>');
  if (M->flen < M->size) {
    if (*access == 'r') M->size = page_align (M->flen,'>');
    else M->flen = safe_truncate (M->file, M->size); }
  M->data = safe_mmap (M->file, M->offs, M->size, M->mode);
  M->next = 0;
  return M;
}

void expect_random_access (mmap_t *M, off_t size) {
  if (!M->next) M->next = safe_calloc (sizeof (mmap_t));
  M->next->mode = M->mode;
  M->next->file = M->file;
  M->next->flen = M->flen;
  M->next->offs = 0;
  M->next->size = size;
  M->next->data = 0;
  M->next->next = 0;
}

void free_mmap (mmap_t *M) {
  if (!M) return;
  if (M->data) munmap (M->data, M->size); 
  if (M->next) free_mmap (M->next);
  else if (M->file) close (M->file); // close only once
  memset (M, 0, sizeof (mmap_t));
  free(M);
}

void write_mmap (mmap_t *map, char *path) {
  int file = safe_open (path, "w");
  off_t size = 0;
  char buf [1<<20];
  safe_lseek (file, 0, SEEK_SET);
  safe_lseek (map->file, 0, SEEK_SET);
  while (0 < (size = safe_read (map->file, buf, 1<<20)))
    safe_write (file, buf, size);
  close (file);
}

off_t MMAP_MOVES = 0;

void *move_mmap (mmap_t *M, off_t offs, off_t size) { // thread-unsafe (if M shared)
  if (offs >= M->offs && (offs+size) <= (M->offs + M->size))
    return M->data + (offs - M->offs); // region already in map
  if (offs+size > M->flen) assert (0 && "move_mmap: offs+size > flen");
  if (M->data) munmap (M->data, M->size); // release old map
  M->data = safe_mmap (M->file, 0, (M->size=M->flen), M->mode);
  MMAP_MOVES += 1; // += M->size;
  return M->data + offs;
}

// checks whether the region [offs..+size] is available from map
// if not -- shifts map to include the region, 
// making sure the mapping aligns on a page boundary
// if random-access: uses pread() outside of main map
void *move_mmap_old (mmap_t *M, off_t offs, off_t size) {
  if (offs >= M->offs && (offs+size) <= (M->offs + M->size))
    return M->data + (offs - M->offs); // region already in map
  if (M->next) return move_mmap (M->next, offs, size);
  if (0) fprintf (stderr, "[%'lu %'lu] outside map(%d) [%'lu %'lu]\n", 
		  (ulong)offs, (ulong)(offs+size), M->file,
		  (ulong)(M->offs), (ulong)(M->offs + M->size));
  assert (offs+size <= M->flen);
  if (M->data) munmap (M->data, M->size); // release old map
  off_t beg = page_align (offs, '<'), end = page_align (offs+size, '>');
  if (end - beg > M->size) M->size = end - beg;
  if (beg + M->size <= M->flen) M->offs = beg; // fits in file: align offs=beg
  else M->offs = M->flen - M->size; // extends past EOF => align end=EOF
  M->data = safe_mmap (M->file, M->offs, M->size, M->mode);
  assert (offs >= M->offs && (offs+size) <= (M->offs + M->size));
  MMAP_MOVES += 1; // += M->size;
  return M->data + (offs - M->offs); 
}

inline void *move_mmap_older (mmap_t *map, off_t offs, off_t size) {
  off_t beg = offs, end = offs+size;
  if (beg < map->offs || end > (map->offs+map->size)){
    assert (beg < end && end < map->flen && end-beg < map->size);
    if (map->data) munmap (map->data, map->size); // release old map
    off_t aligned = page_align (beg, '<'); // page-aligned offset
    if (aligned + map->size < map->flen) // region fits inside file
      map->offs = aligned; // align on page bound
    else // region extends past the end of file
      map->offs = page_align (map->flen, '>') - map->size;
    map->data = safe_mmap (map->file, map->offs, map->size, map->mode);
    assert (beg >= map->offs && end <= (map->offs + map->size));
  }
  return map->data + (beg - map->offs);
}

// map region beg..end from file (align boundaries properly)
void *mmap_region (int fd, off_t offs, off_t size, char *access) {
  off_t beg = page_align (offs,'<'), end = page_align (offs+size,'>');
  char *buf = safe_mmap (fd, beg, end-beg, access);
  char *result = buf + (offs-beg);
  return result;
}

void unmap_region (void *region, off_t offs, off_t size) {
  off_t beg = page_align(offs,'<'), end = page_align(offs+size,'>');
  munmap (region - (offs-beg), end-beg);
}

void grow_mmap_file (mmap_t *map, off_t size) {
  if (map->flen > size) return; // enough space
  map->flen = next_pow2 (size); // page_align (2 * size, '>');
  safe_truncate (map->file, map->flen); // expand file
  if (map->next) map->next->flen = map->flen; // update overflow map
}

void grow_mmap (mmap_t *map, off_t size) {
  grow_mmap_file (map, size);
  if (map->size >= map->flen) return;
  if (map->data) munmap (map->data, map->size); // release old map
  map->size = map->flen;
  map->data = safe_mmap (map->file, 0, map->size, map->mode);
  MMAP_MOVES += 1; // += M->size;
}

////////////////////////////////////////////////////////////
////// Convenience routines
////////////////////////////////////////////////////////////

// This is specific for Win32 platforms. Win32 platforms have an annoying
// habit of converting \n into \r\n on any write, and \r\n to \n on a read.
// This screws up any attempts to align the contents of the file for mmaps.
// The solution is either to use setmode (fd, O_BINARY), or OR the access 
// flags with O_BINARY. If O_BINARY is not defined, we're on Unix, where
// there are only binary files.
#ifndef O_BINARY
#define O_BINARY 0
#endif

inline off_t align8 (off_t x) { return (((x-1) >> 3) + 1) << 3; }

// fast power-2 ceiling of a 32-bit integer
/*
inline uint next_pow2 (uint x) {
  --x; //            1000 0001 (129)
  x |= x >>  1; //   1100 0001
  x |= x >>  2; //   1111 0001
  x |= x >>  4; //   1111 1111
  x |= x >>  8; 
  x |= x >> 16;
  return x + 1; // 1 0000 0000 (256)
}
*/

// fast power-2 ceiling of a 64-bit integer
inline ulong next_pow2 (ulong x) {
  --x; //            1000 0001 (129)
  x |= x >>  1; //   1100 0001
  x |= x >>  2; //   1111 0001
  x |= x >>  4; //   1111 1111
  x |= x >>  8; 
  x |= x >> 16;
  x |= x >> 32; 
  return x + 1; // 1 0000 0000 (256)
}

// fast base-2 logarithm of a 32-bit integer (beats bit-arithmetic)
/*
inline uint ilog2 (uint x) {
  register uint l=0;
  if(x >= 1<<16) { x>>=16; l|=16; }
  if(x >= 1<<8) { x>>=8; l|=8; }
  if(x >= 1<<4) { x>>=4; l|=4; }
  if(x >= 1<<2) { x>>=2; l|=2; }
  if(x >= 1<<1) l|=1;
  return l;
}
*/

// fast base-2 logarithm of a 64-bit integer (beats bit-arithmetic)
inline uint ilog2 (ulong x) { 
  register uint l=0;
  if(x >= 1l<<32) { x>>=32; l|=32; }
  if(x >= 1<<16) { x>>=16; l|=16; }
  if(x >= 1<<8) { x>>=8; l|=8; }
  if(x >= 1<<4) { x>>=4; l|=4; }
  if(x >= 1<<2) { x>>=2; l|=2; }
  if(x >= 1<<1) l|=1;
  return l;
}

// returns page-aligned ceiling of a number
inline off_t page_align (off_t offs, char side) { // should be thread-safe
  static off_t psize = 0; 
  if (!psize) psize = sysconf (_SC_PAGESIZE);
  off_t floor = psize * (uint) (offs / psize);
  off_t ceil = (floor < offs) ? floor + psize : floor;
  return side == '>' ? ceil : floor;
}

void *safe_malloc (size_t size) {
  void *buf = malloc (size);
  if (!buf) {
    fprintf (stderr, "[malloc] failed on %lu bytes: [%d] ", (ulong)size, errno);
    perror (""); assert (0); 
  }
  return buf;
}

void *safe_calloc (size_t size) {
  void *buf = calloc (1, size);
  if (!buf) {
    fprintf (stderr, "[calloc] failed on %lu bytes: [%d] ", (ulong)size, errno);
    perror (""); assert (0); 
  }
  return buf;
}

void *safe_realloc (void *buf, size_t size) {
  buf = realloc (buf, size);
  if (!buf) {
    fprintf (stderr, "[realloc] failed on %lu bytes: [%d] ", (ulong)size, errno);
    perror (""); assert (0); 
  }
  return buf;
}

// realloc buf if new size if greater than old
void *lazy_realloc (void *buf, size_t *old, size_t new) {
  if (*old >= new) return buf; // big enough
  buf = realloc (buf, new);
  if (!buf) {
    fprintf (stderr, "[realloc] failed on %lu bytes: [%d] ", (ulong)new, errno);
    perror (""); assert (0); 
  }
  *old = new;  
  return buf;
}

int safe_open (char *path, char *access) {
  int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int flags = 
    (*access == 'r') ? (O_BINARY | O_RDONLY) :
    (*access == 'a') ? (O_BINARY | O_RDWR | O_CREAT) :
    (*access == 'w') ? (O_BINARY | O_RDWR | O_CREAT | O_TRUNC) : 0;
  if (!strcmp (path, "-") ||
      !strcmp (path, "/dev/stdin")) return fileno (stdin);
  int fd = open64 (path, flags, permissions);
  if (fd < 0) {
    fprintf (stderr, "[open] failed to open '%s' for '%s': [%d] ", path, access, errno);
    perror (""); assert (0);
  }
  return fd;
}

FILE *safe_fopen (char *path, char *access) {
  FILE *f = 0; // handle /dev/stdin as real stdin
  if (!strcmp (path, "/dev/stdin") || !strcmp (path, "-")) f = stdin;
  else f = fopen64 (path, access);
  if (!f) {
    fprintf (stderr, "[fopen] failed to open '%s' for '%s': [%d] ", path, access, errno);
    perror (""); assert (0);
  }
  return f;
}

FILE *safe_popen (char *access, char *fmt, ...) { // unsafe: popen()
  char cmd [1000];
  va_list args;
  va_start (args, fmt);
  vsprintf (cmd, fmt, args);
  va_end (args);
  FILE *p = popen (cmd, access); // unsafe
  if (!p) {
    fprintf (stderr, "[popen] failed to open '%s' for '%s': [%d] ", cmd, access, errno);
    perror (""); assert (0); }
  return p; 
}

int popen2 (const char *command, pid_t *_pid) { // unsafe: popen()
  int fd[2]; // read fd[0] <-- fd[1] write 
  
  if (pipe(fd)) { perror ("pipe failed"); return 0; }
  
  pid_t pid = fork();
  
  if (pid == 0) { // we are the child
    
    setpgid(0,0); // make our pid the process group leader 
    
    close(fd[0]); // will not be reading from pipe
    dup2(fd[1],1); // replace stdout with fd[1]
    // close(fd[1]); // needed?
    
    execl("/bin/sh", "sh", "-c", command, NULL);
    perror("execl");
    exit(1);
  }
  
  if (pid < 0) { perror ("fork failed"); return 0; }
  
  if (_pid) *_pid = pid; // save child's PID for killing
  
  close(fd[1]); // will not be writing to pipe
  
  return fd[0];
  
  //FILE *in = fdopen (fd[0], "r"); // will read from the "read" end
  //if (!in) perror ("fdopen failed");
  //return in;
}

void nonblock(FILE *f) {
  int fd = fileno(f), flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

void *safe_mmap (int fd, off_t offset, off_t size, char *access) {
  int mprot = (access[0] == 'r') ? PROT_READ  : (PROT_READ|PROT_WRITE);
  //int mflag = (access[1] == '+') ? MAP_SHARED : (MAP_SHARED|MAP_POPULATE); // pre-populate hashes/vecs
  int mflag = (access[1] == '!') ? (MAP_SHARED|MAP_POPULATE) : MAP_SHARED; // pre-populate when forced
  if (access[1] == '!') fprintf (stderr, "[mmap:%d] pre-fetching %ld+%ldMB\n", fd, (long)offset, (long)(size>>20));
  //if (1) mflag = MAP_SHARED;
  //fprintf (stderr, "[mmap] fd:%d:%s off:%ld sz:%ld mprot:%d mflag:%d\n", fd, access, offset, size, mprot, mflag);
  if (size == 0) return NULL;
  void *buf = mmap64 (NULL, size, mprot, mflag, fd, offset);
  if ((buf == (void *) -1) || (buf == NULL)) {
    fprintf (stderr, "[mmap] failed on %lu bytes: [%d] ", (ulong)size, errno);
    perror (""); assert (0); 
  }
  return buf;
}

void *safe_remap (int fd, void *buf, off_t osize, off_t nsize) {
  if (fd) safe_truncate (fd, nsize);
#ifdef MREMAP_MAYMOVE
  buf = mremap (buf, osize, nsize, MREMAP_MAYMOVE);
#else
  munmap(buf, osize); 
  buf = safe_mmap (fd, 0, nsize, "w"); // specifying "w" here is BAAD, used in resize_vec()
#endif
  if ((buf == (void*) -1) || (buf == NULL)) {
    fprintf (stderr, "[mremap] failed on %lu bytes: [%d] ", (ulong)nsize, errno);
    perror (""); assert (0); 
  }
  return buf;
}

off_t safe_truncate (int fd, off_t size) {
  if (ftruncate64 (fd, size)) {
    fprintf (stderr, "[ftruncate] failed on size %lu [%d]", (ulong) size, errno);
    perror (""); assert (0); 
  }
  return size;
}

off_t safe_lseek (int fd, off_t offs, int whence) {
  off_t result = lseek64 (fd, offs, whence);
  if (result == -1) {
    fprintf (stderr, "[lseek] failed on offset %lu [%d]", (ulong) offs, errno);
    perror (""); assert (0); 
  }
  return result;
}

off_t safe_read (int fd, void *buf, off_t size) {
  ssize_t result = read (fd, buf, size);
  if (result == -1) {
    fprintf (stderr, "[read] returned %lu on %lu bytes: [%d]", 
	     result, (ulong) size, errno);
    perror (""); assert (0); 
  }
  return (off_t) result;
}

off_t safe_write (int fd, void *buf, off_t size) {
  ssize_t result = write (fd, buf, size);
  if (result == -1) {// || ((off_t) result != size)) {
    fprintf (stderr, "[write] returned %lu on %lu bytes: [%d] ", 
	     result, (ulong) size, errno);
    perror (""); assert (0); 
  }
  return (off_t) result;
}

off_t safe_pread (int fd, void *buf, off_t size, off_t offset) {
  ssize_t result = pread (fd, buf, size, offset);
  if (result != size) {
    fprintf (stderr, "[pread] returned %lu on %lu bytes: [%d]", 
	     result, (ulong) size, errno);
    perror (""); assert (0); 
  }
  return (off_t) result;
}

off_t safe_pwrite (int fd, void *buf, off_t size, off_t offset) {
  ssize_t result = pwrite (fd, buf, size, offset);
  if (result == -1) {// || ((off_t) result != size)) {
    fprintf (stderr, "[pwrite] returned %lu on %lu bytes: [%d] ", 
	     result, (ulong) size, errno);
    perror (""); assert (0); 
  }
  return (off_t) result;
}

char *___itoa (uint i) { // thread-unsafe: static + buffer overflow
  static char buf[100];
  sprintf (buf, "%u", i);
  return buf;
}

char *___ftoa (char *fmt, float f) { // thread-unsafe: static + buffer overflow
  static char buf[100];
  sprintf (buf, fmt, f);
  return buf;
}

char *acat (char *s1, char *s2) {
  if (!s1 || !s2) return NULL;
  char *s = malloc (strlen(s1) + strlen(s2) + 1);
  strcpy (s,s1);
  strcat (s,s2);
  return s;
}

// append src to *dst, re-allocating *dst if needed
void stracat (char **dst, size_t *n, char *src) {
  if (!dst || !n) assert (0 && "[stracat] null pointers");
  if (!*dst) *dst = safe_calloc (*n = 5);
  size_t ls = strlen(src)+1, ld = strlen (*dst);
  if ((ls + ld) >= (*n)) *dst = safe_realloc (*dst, (*n = *n + ls));
  memcpy ((*dst) + ld, src, ls);
}

char *fmt (char *buf, const char *format, ...) {
  assert (buf && format);
  va_list args;
  va_start (args, format);
  vsprintf (buf, format, args);
  va_end (args);
  return buf;
}

char *fmtn (int sz, const char *format, ...) {
  assert (sz && format);
  char *buf = malloc(sz); *buf='\0';
  va_list args;
  va_start (args, format);
  vsnprintf (buf, sz, format, args);
  va_end (args);
  return buf;
}

void apply (void (*func)(void *), ...) {
  void *arg = NULL;
  va_list args;
  va_start (args, func); // stops at first NULL
  while ((arg = va_arg (args, void*))) func(arg);
  va_end (args);
}

// like sprintf, but appends to end of buf, reallocating if needed
void zprintf (char **buf, int *sz, const char *fmt, ...) {
  if (!*buf) *sz = 0;
  char *tmp = NULL;
  va_list args;
  va_start (args, fmt);
  int tmp_sz = vasprintf (&tmp, fmt, args), old_sz = *sz;
  if (tmp_sz < 0) { fprintf (stderr, "vasprintf(%s) failed: no memory?\n", fmt); return; }
  va_end (args);
  *buf = safe_realloc (*buf, old_sz+tmp_sz+1);
  memcpy ((*buf)+old_sz, tmp, tmp_sz);
  (*buf)[old_sz + tmp_sz] = '\0';
  *sz = old_sz + tmp_sz;
  free (tmp);
}

// like memcpy, but append to the trg+used
void memcat (char **trg, int *used, char *src, int sz) {
  if (!*trg) *used = 0; // no buffer -> zero used
  *trg = safe_realloc (*trg, *used+sz+1);
  memcpy ((*trg)+(*used), src, sz);
  *used = *used + sz;
  (*trg)[*used] = '\0';
}

void zcat0 (char **buf, int *sz, char *str, ...) {  
  va_list args;
  va_start (args, str);
  char *next = str;
  while (next) {
    memcat (buf, sz, next, strlen(next)+1);
    next = va_arg (args, char*);
  }
  va_end (args);
}

void zcat (char **buf, int *sz, char *new) { memcat (buf, sz, new, strlen(new)); }

/*
void stracat_test () {
  char *s = NULL, *s1 = "abcdefg", *s2 = "1234567";
  int i, n = 0;
  for (i = 0; i < 1000; ++i) {
    stracat (&s, &n, s1);
    stracat (&s, &n, s2);
  }
  free (s);
}
*/

float vtime () { // thread-unsafe: static
  static long clock_speed = 0;
  static clock_t last = 0;
  struct tms buf;
  float result;
  clock_t this = times (&buf);
  if (!clock_speed) clock_speed = sysconf (_SC_CLK_TCK);
  if (!last) last = this;
  result = (float) (this - last) / clock_speed;
  last = this;
  return result;
}

int file_exists (char *fmt, ...) {
  char path [9999];
  va_list args;
  va_start (args, fmt);
  vsprintf (path, fmt, args);
  va_end (args);
  struct stat buf;
  return (0 == stat (path, &buf));
}

// time when the file was last modified
time_t file_modified (char *fmt, ...) {
  char path [9999];
  va_list args;
  va_start (args, fmt);
  vsprintf (path, fmt, args);
  va_end (args);
  struct stat buf;
  memset (&buf, 0, sizeof(buf));
  //if (!file_exists(file_name)) return 0;
  if (stat (path, &buf)) return 0; // file doesn't exist
  return buf.st_mtime;
}

off_t file_size (char *fmt, ...) {
  char path [9999];
  va_list args;
  va_start (args, fmt);
  vsprintf (path, fmt, args);
  va_end (args);
  struct stat buf;
  memset (&buf, 0, sizeof(buf));
  if (stat (path, &buf)) return 0; // file doesn't exist
  return buf.st_size;
}

void cp_dir (char *src, char *trg) { // unsafe: system()
  if (!src || !trg) return;
  char x[1000], *cmd = fmt (x, "mkdir -p %s; cp %s/* %s", trg, src, trg);
  if (system (cmd)) { fprintf (stderr, "ERROR: %s\n", cmd); perror(""); assert(0); }
}

void rm_dir (char *dir) { // unsafe: system()
  if (!dir) return;
  char x[1000], *cmd = fmt (x, "rm -rf %s", dir);
  if (system (cmd)) { fprintf (stderr, "ERROR: %s\n", cmd); perror(""); assert(0); }
}

void mv_dir (char *src, char *trg) { // delete target, rename source
  if (!src || !trg) return;
  rm_dir (trg); // unsafe: system()
  if (rename (src, trg)) {
    fprintf (stderr, "ERROR: mv %s %s [%d] ", src, trg, errno);
    perror (""); assert (0); 
  }
}

void mkdir_parent (char *_path) { // unsafe: system
  char *path = strdup(_path), *eop = strrchr (path, '/'), cmd[1000];
  if (!eop) return;
  *eop = '\0';
  if (file_exists (path)) return;
  sprintf (cmd, "mkdir -p %s", path);
  if (system (cmd)) { fprintf (stderr, "ERROR: %s\n", cmd); assert(0); }
  free(path);
}

void rmdir_parent (char *path) { // unsafe: system
  char *eop = strrchr (path, '/'); // last slash
  if (!eop) return;
  char *pfx = strndup(path,eop-path);
  rm_dir (pfx);
  free (pfx);
}

inline void show_spinner () { // thread-unsafe: static
  static int i = 0;
  char spin[] = "|/-\\";
  fprintf (stderr, "%c\r", spin[++i%4]);
}

inline void show_progress (ulong done, ulong total, char *msg) { // thread-unsafe: static
  static ulong dots = 0, prev = 0, line = 50;
  static time_t last = 0, begt = 0;
  time_t this = time(0);
  //printf ("%d %d %d\n", this, last, CLOCKS_PER_SEC);
  //if (this - last < CLOCKS_PER_SEC) return; 
  if (this == last) return;
  last = this;
  fprintf (stderr, ".");
  if (!begt) begt = this;
  if (done < prev) prev = done; 
  if (++dots < line) return;
  double todo = total-done, di = done-prev, ds = this-begt, rpm = 60*di/ds, ETA = todo/rpm;
  //double ETA = ((double)(N-n)) / ((n-m) * 60 / line); // minutes
  if (!total) fprintf (stderr, "%ld%s @ %.0f / minute\n", done, msg, rpm);
  else {      fprintf (stderr, "%ld / %ld%s", done, total, msg);
    if (ETA < 60)        fprintf (stderr, " ETA: %.1f minutes\n", ETA);
    else if (ETA < 1440) fprintf (stderr, " ETA: %.1f hours\n", ETA/60);
    else                 fprintf (stderr, " ETA: %.1f days\n", ETA/1440);
  }
  prev = done;
  begt = this;
  dots = 0;
}

double getprm (char *params, char *name, double def) {
  if (!(params && name)) return def;
  unsigned n = strlen (name);
  char *p = strstr (params, name);
  return p ? atof (p + n) : def;
}

char *getprmp (char *params, char *name, char *def) {
  if (!(params && name)) return def;
  char *p = strstr (params, name);
  return p ? p + strlen(name) : def;
}

char *getprms (char *params, char *name, char *def, char eol) {
  if (!(params && name)) return def ? strdup(def) : NULL;
  unsigned n = strlen (name);
  char *p = strstr (params, name), *end;
  if (!p) return def ? strdup(def) : NULL;
  if ((end = index (p+n, eol))) *end = 0; // null-terminate
  char *result = strdup(p+n); // take a copy
  if (end) *end = eol; // undo null-termination
  return result;
}

char *getarg (int argc, char *argv[], char *arg, char *def) {
  if (!arg) return def;
  char **a = argv, **end = argv + argc - 1;
  while (++a < end) if (!strcmp(*a,arg)) return *++a;
  return def;
}

#if (defined __APPLE__)
#include <sys/sysctl.h>
inline ulong physical_memory () {
  int mib[2] = { CTL_HW, HW_MEMSIZE };
  ulong size = 0, len = sizeof(size);
  sysctl (mib, 2, &size, &len, NULL, 0);
  return size;
}
#else
inline ulong physical_memory () {
  return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
}
#endif
