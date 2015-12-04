/*

   Copyright (C) 1997-2014 Victor Lavrenko

   All rights reserved. 

   THIS SOFTWARE IS PROVIDED BY VICTOR LAVRENKO AND OTHER CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "coll.h"

#ifndef HASHTABLE
#define HASHTABLE

// note: putting offs_t into indx won't save any space: 
// indx is half-full: 2N * offs_t == N * offs_t + 2N * uint
typedef struct { 
  char *access;
  char   *path;
  uint   *indx; // indx[code] -> id
  uint   *code; // code[id] = hashcode
  coll_t *keys; // keys[id] = string 
  //char   *data;
} hash_t; 

#define nkeys(h) (len((h)->keys->offs)-1)

hash_t *open_hash (char *path, char *access);
void    free_hash (hash_t *h) ;
hash_t *copy_hash (hash_t *src) ;

char *id2key (hash_t *h, uint i) ;
uint  key2id (hash_t *h, char *key) ; 
uint has_key (hash_t *h, char *key) ;

//unsigned keyIn (hash_t *t, char *key) ;
//hash_t *hnew (unsigned num_els, char *path) ;
//hash_t *hmmap (char *path, access_t access);

//void hwrite (hash_t *t, char *path) ;
//hash_t *hread (char *path) ;
//unsigned hsizeof (hash_t *t) ;

inline uint murmur3 (const char *key, uint len) ;
inline uint murmur3uint (uint key) ;

//inline uint OneAtATime (const char *key, uint len) ;
//inline uint SBox (const char *key, uint len) ;

typedef struct { // obsolete now
  uint code; // hashcode of the key
  char key[0]; // string representation
} hkey_t;


#endif

