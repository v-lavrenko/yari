#include "mmap.h"

size_t fsize (char *path) {
  struct stat buf;
  memset (&buf, 0, sizeof(buf));
  if (stat (path, &buf)) return 0; // file doesn't exist
  return buf.st_size;
}

ulong sdbm_hash (char *buf, size_t sz, ulong seed) ;

ulong xsum_chunk (int fd, size_t beg, size_t end, ulong seed) {
  size_t mbeg = (beg % 4096) ? (((beg>>12)+0)<<12) : beg; // left-align
  size_t mend = (end % 4096) ? (((end>>12)+1)<<12) : end; // right-align
  size_t msize = mend - mbeg, skip = beg - mbeg, sz = end-beg;
  char *buf = safe_mmap (fd, mbeg, msize, "r");
  ulong result = sdbm_hash (buf+skip, sz, seed);
  munmap (buf, msize);
  //fprintf(stderr,"%d [%lu+%lu] -> [%lu+%lu] %lx -> %lx\n", fd, beg, sz, mbeg, msize, seed, result);
  return result;  
}

ulong xsum_file (char *path) {
  size_t sz = fsize(path), end;
  //fprintf(stderr,"%s len %lu\n", path, sz);
  if (!sz) return 0; // file doesn't exist or is empty -> 0
  ulong xsum = sz;  
  int fd = safe_open (path, "r");
  if (sz < 3*4096) xsum = xsum_chunk (fd, 0, sz, xsum);    
  else {
    for (end = 4096; end < sz; end *= 2) 
      xsum = xsum_chunk (fd, end-4096, end, xsum);
    xsum = xsum_chunk (fd, sz-4096, sz, xsum);    
  }
  close(fd);
  return xsum;
}

void xsum_args (char *A[], int n) {
  int i = -1;
  while (++i < n) printf("%016lx\t%s\n", xsum_file(A[i]), A[i]);
}

void xsum_check(char *file) {
  FILE *in = safe_fopen (file, "r");
  ulong ref = 0, good = 0, bad = 0;
  char *path = NULL;
  while (2 == fscanf(in, "%lx %ms", &ref, &path)) {
    ulong sys = xsum_file(path);
    char *msg = (ref == sys) ? "OK" : "ERROR";
    if (ref == sys) ++good;
    else ++bad;
    printf("%s\t%s\n", msg, path);
    free(path);
  }
  if (in != stdin) fclose(in);
  printf("%lu good, %lu bad\n", good, bad);
}


char *usage = 
  "xsum f1 f2 f3 ... fast log-probing checksum with md5-like output\n"
  "xsum -c file  ... verify checksums in a file\n"
  ;

#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  if (argc < 2) return fprintf (stderr, "%s", usage);
  if (!strcmp(a(1),"-c")) xsum_check(a(2));
  else xsum_args (argv+1, argc-1);
  return 0;
}
