#include "hash.h"
#include "textutil.h"

int load_tokens (char *out, char *hash) {
  ulong NR = 0;
  ushort *O = open_vec (out, "w", sizeof(ushort));
  hash_t *H = open_hash (hash, "r");
  char tok[1000];
  while (fgets (tok, 999, stdin)) {
    noeol(tok); // strip newline
    ushort id = key2id(H,tok);
    if (id) O = append_vec (O, &id);
    if (((++NR)>>20) & 1) show_progress ((NR>>20), 0, "M tokens"); 
  }
  fprintf(stderr, "\nDONE: %ld tokens -> %s\n", NR, out);
  free_vec(O);
  free_hash(H);
  return 0;
}

void sample (ushort *V, hash_t *H, long idx, uint sz, char how) {
  if (idx < 0) idx = random();
  idx %= (len(V) - sz); // wrap if V[idx:idx+sz] would overflow
  uint i;
  putchar('[');
  for (i=idx; i < idx+sz; ++i) {
    if (i > idx) fputs(", ", stdout); // comma-separate tokens
    if (how == 'i') { printf("%d", V[i]); continue; } // [1, 2, 3, 4, 5]
    char *tok = id2key(H,V[i]);
    if (how == 't') { printf ("'%s'", tok); continue; } // ['I', 'am', 'a']
  }
  puts("]"); // adds newline
}

int rnd_sample (char *vec, char *dic, char *prm) {
  ushort *V = open_vec (vec, "r", sizeof(ushort));
  hash_t *H = open_hash (dic, "r");
  long id = getprm(prm,"id=",0);
  uint sz = getprm(prm,"sz=",128);
  char *how = getprmp (prm, "how=", "tok");
  sample (V, H, id, sz, *how);
  free_vec(V);
  free_hash(H);
  return 0;
}

// ------------------------------ main ------------------------------

#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : "")

int usage () {
  fprintf (stderr, 
	   "usage: bpe -load VEC DICT < tokens\n"
	   "           -rand VEC DICT [id=0,sz=128,how=tok]\n"
	   );
  return 1;
}

int main (int argc, char *argv[]) {
  srandom(time(0));
  if (argc < 4) return usage();
  if (!strcmp(a(1),"-load")) return load_tokens (arg(2), arg(3));
  if (!strcmp(a(1),"-rand")) return  rnd_sample (arg(2), arg(3), a(4));
  return 0;
}

/* this is ugly...
#include <microhttpd.h>

static enum MHD_Result handler
(void *cls, // context argument from MHD_start_daemon
 struct MHD_Connection * connection,
 const char *url, // requested url 
 const char *method, const char *version, // GET,POST and HTTP version
 const char *up_data, size_t * up_size, // from POST, PUT etc
 void **ptr // I can set this, will be oreserved across calls
 ) {
  const char * page = cls;
  int ret;
  
  if (strcmp(method, "GET")) return MHD_NO; // only support GET
  if (*upload_data_size) return MHD_NO; // upload data in a GET
  
  *ptr = NULL; // clear context pointer 
  struct MHD_Response *response = MHD_create_response_from_buffer
    (strlen(page), (void*) page, MHD_RESPMEM_PERSISTENT);
  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  return ret;
}

#define PAGE "<html><head><title>libmicrohttpd demo</title>"	\
               "</head><body>libmicrohttpd demo</body></html>"

int start_httpd(char *prm) {
  uint port = getprm(prm,"port=",8000);
  uint flags = MHD_USE_THREAD_PER_CONNECTION, END = MHD_OPTION_END;
  struct MHD_Daemon *d = MHD_start_daemon
    (flags, // OR of MHD_FLAG
     port, 
     NULL, NULL, // AcceptPolicyCallback (auth) + arg to it 
     &handler, PAGE, // AccessHandlerCallback + arg to it 
     END);  
  assert (d && "MHD_start_daemon returned NULL");
  (void) getc (stdin); // wait for keypress
  MHD_stop_daemon(d);
  return 0;  
}

*/
