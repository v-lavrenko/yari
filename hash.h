/*
  
  Copyright (c) 1997-2016 Victor Lavrenko (v.lavrenko@gmail.com)
  
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
  char mlock;
  //char   *data;
} hash_t; 

#define nkeys(h) (len((h)->keys->offs)-1)

hash_t *open_hash (char *path, char *access);
void    free_hash (hash_t *h) ;
hash_t *copy_hash (hash_t *src) ;
hash_t *reopen_hash (hash_t *h, char *access);

char *id2key (hash_t *h, uint i) ;
uint  key2id (hash_t *h, char *key) ; 
uint has_key (hash_t *h, char *key) ;
uint id2id (hash_t *src, uint id, hash_t *trg) ;
uint *keys2ids (hash_t *h, char **keys) ; // batch version of key2id

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

