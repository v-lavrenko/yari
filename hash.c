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

#include "hash.h"

//float HASH_LOAD = 0.9; 
//uint  HASH_FUNC = 0; // 0:multiadd 1:murmur3 2:OneAtATime
//uint  HASH_PROB = 0;  // 0:linear 1:quadratic 2:secondary
ulong COLLISIONS = 0;

hash_t *copy_hash (hash_t *src) {
  hash_t *trg = safe_calloc (sizeof (hash_t));
  trg->code = copy_vec (src->code);
  trg->indx = copy_vec (src->indx);
  trg->access = strdup ("w");
  trg->keys = open_coll (NULL, NULL);
  uint i = 0, n = nvecs (src->keys);
  while (++i <= n) {
    char *key = get_chunk (src->keys, i);
    put_chunk (trg->keys, i, key, strlen(key) + 1);
  }
  return trg;
}

inline static hash_t *open_hash_inmem () {
  hash_t *h = safe_calloc (sizeof (hash_t));
  h->keys = open_coll (NULL, NULL);
  h->indx = new_vec (1024, sizeof(uint));
  h->code = new_vec (0, sizeof(uint));
  h->access = strdup ("w");
  return h;
}

hash_t *open_hash (char *_path, char *_access) {
  if (*_access != 'w' && file_exists (_path) && !file_exists ("%s/hash.code",_path)) { 
    fprintf (stderr, "\n\nERROR: %s is outdated, re-index or downgrade\n\n", _path);
    exit(1); 
  }
  if (!_path) return open_hash_inmem ();
  char *path = strdup (_path), *access = strdup(_access);
  access[1] = 0; // make sure there's no '+' at the end
  hash_t *h = safe_calloc (sizeof (hash_t));
  h->access = access;
  h->path = path;
  h->keys = open_coll (path, access); // expect_random_access (h->keys->vecs,1<<20);
  h->code = open_vec (cat(path,"/hash.code"), access, sizeof(uint));
  h->indx = open_vec (cat(path,"/hash.indx"), access, sizeof(uint));
  if (0 == len(h->indx)) h->indx = resize_vec (h->indx, 1023);
  //h->data = open_mmap (path, access, 0); grow_mmap (h->data, 0);
  return h;
}

void free_hash (hash_t *h) {
  if (!h) return;
  //free_mmap (h->data);
  free_coll (h->keys);
  free_vec  (h->code);
  free_vec  (h->indx);
  free (h->access);
  free (h->path);
  memset (h, 0, sizeof(hash_t));
  free (h);
}

hash_t *reopen_hash (hash_t *h, char *access) {
  char *path = h->path ? strdup(h->path) : NULL;
  free_hash (h);
  h = open_hash (path, access);
  if (path) free (path);
  return h;
}

inline char *id2key (hash_t *h, uint id) { return get_chunk (h->keys, id); }

// find a slot in indx that is empty or matches the key
inline uint *href (hash_t *h, char *key, uint code) {
  uint *H = h->indx, N = len(h->indx), o = code % N, id; // c = code, i=1;
  while ((id = H[o])) { // stop if we find an empty slot
    if (h->code[id] == code) { // hashcode match for id
      char *k = get_chunk (h->keys, id); // stored key
      if (!strcmp (key, k)) break; // string match
    }
    o = (o+1) % N; // linear probing ... seems fastest on big datasets ... wtf??
    // o = (o + i*i) % N; ++i; // quadratic probing
    // o = (c = murmur3uint (c)) % N; // secondary hashing
  }
  return H+o;
}

void hrehash (hash_t *h) {
  uint id, n = nvecs(h->keys), N = next_pow2(2*len(h->indx)) - 1;
  h->indx = resize_vec (h->indx, N);
  memset (h->indx, 0, ((ulong)N)*sizeof(uint));
  for (id = 1; id <= n; ++id) { // for each key in the table
    char *key = strdup (get_chunk (h->keys, id)); // key string
    uint *slot = href (h, key, h->code[id]); // find new slot
    *slot = id; // store the id of that key in the slot
    free (key);
  }
}

uint has_key (hash_t *h, char *key) {
  if (!h || !key) return 0;
  uint code = murmur3 (key, strlen(key));
  uint *slot = href (h, key, code);
  return *slot;
}

uint key2id (hash_t *h, char *key) {
  if (!h || !key) return 0;
  uint code = murmur3 (key, strlen(key));
  uint *slot = href (h, key, code);
  if (*slot || h->access[0] == 'r') return *slot; // key already in table
  uint id = nvecs(h->keys)+1;
  put_chunk (h->keys, id, key, strlen(key)+1);
  h->code = resize_vec (h->code, id+1);
  h->code[id] = code;
  if (id > 0.3*len(h->indx)) { hrehash(h); slot = href (h, key, code); }
  return (*slot = id);
}

uint id2id (hash_t *src, uint id, hash_t *trg) {
  if (src == trg) return id;
  if (!src || !trg) return 0;
  char *key = id2key (src,id);
  return key ? key2id (trg,key) : 0;
}

// adapted from http://www.team5150.com/~andrew/noncryptohashzoo/Murmur3.html

inline uint rot ( uint x, char r ) { return (x << r) | (x >> (32 - r)); }

#define mmix3(h,k) { k *= m1; k = rot(k,r1); k *= m2; h *= 3; h ^= k; }

inline uint murmur3 (const char *key, uint len) {
  const uint m1 = 0x0acffe3d, m2 = 0x0e4ef5f3, m3 = 0xa729a897;
  const uint r1 = 11, r2 = 18, r3 = 18;
  uint h = len, k = 0;
  const uint *dwords = (const uint *)key;
  while( len >= 4 ) {
    k = *dwords++;
    mmix3(h,k);
    len -= 4;
  }
  const char *tail = (const char *)dwords;
  switch (len ) {
  case 3: k ^= tail[2] << 16;
  case 2: k ^= tail[1] << 8;
  case 1: k ^= tail[0];
    mmix3(h,k);
  }
  h ^= h >> r2;
  h *= m3;
  h ^= h >> r3;
  return h;
}

inline uint murmur3uint (uint key) {
  const uint m1 = 0x0acffe3d, m2 = 0x0e4ef5f3, m3 = 0xa729a897;
  const uint r1 = 11, r2 = 18, r3 = 18;
  uint h = 4, k = key;
  mmix3(h,k);
  h ^= h >> r2;
  h *= m3;
  h ^= h >> r3;
  return h;
}

// good hashes from http://www.team5150.com/~andrew/noncryptohashzoo/

inline uint OneAtATime (const char *key, uint len) {
  register uint hash = 0;
  while (len--) {
    hash += ( *key++ );
    hash += ( hash << 10 );
    hash ^= ( hash >> 6 );
  }
  hash += ( hash << 3 );
  hash ^= ( hash >> 11 );
  hash += ( hash << 15 );
  return ( hash );
}

inline uint multiadd_hashcode (char *s) {
  //if (HASH_FUNC == 1) return murmur3 (s, strlen(s));
  //if (HASH_FUNC == 2) return OneAtATime (s, strlen(s));
  register uint result = 0; 
  while (*s) result = result * 129 + *s++;
  return result;
}

const uint SBoxTable[256] = {
  0x4660c395, 0x3baba6c5, 0x27ec605b, 0xdfc1d81a, 0xaaac4406, 0x3783e9b8, 0xa4e87c68, 0x62dc1b2a,
  0xa8830d34, 0x10a56307, 0x4ba469e3, 0x54836450, 0x1b0223d4, 0x23312e32, 0xc04e13fe, 0x3b3d61fa,
  0xdab2d0ea, 0x297286b1, 0x73dbf93f, 0x6bb1158b, 0x46867fe2, 0xb7fb5313, 0x3146f063, 0x4fd4c7cb,
  0xa59780fa, 0x9fa38c24, 0x38c63986, 0xa0bac49f, 0xd47d3386, 0x49f44707, 0xa28dea30, 0xd0f30e6d,
  0xd5ca7704, 0x934698e3, 0x1a1ddd6d, 0xfa026c39, 0xd72f0fe6, 0x4d52eb70, 0xe99126df, 0xdfdaed86,
  0x4f649da8, 0x427212bb, 0xc728b983, 0x7ca5d563, 0x5e6164e5, 0xe41d3a24, 0x10018a23, 0x5a12e111,
  0x999ebc05, 0xf1383400, 0x50b92a7c, 0xa37f7577, 0x2c126291, 0x9daf79b2, 0xdea086b1, 0x85b1f03d,
  0x598ce687, 0xf3f5f6b9, 0xe55c5c74, 0x791733af, 0x39954ea8, 0xafcff761, 0x5fea64f1, 0x216d43b4,
  0xd039f8c1, 0xa6cf1125, 0xc14b7939, 0xb6ac7001, 0x138a2eff, 0x2f7875d6, 0xfe298e40, 0x4a3fad3b,
  0x066207fd, 0x8d4dd630, 0x96998973, 0xe656ac56, 0xbb2df109, 0x0ee1ec32, 0x03673d6c, 0xd20fb97d,
  0x2c09423c, 0x093eb555, 0xab77c1e2, 0x64607bf2, 0x945204bd, 0xe8819613, 0xb59de0e3, 0x5df7fc9a,
  0x82542258, 0xfb0ee357, 0xda2a4356, 0x5c97ab61, 0x8076e10d, 0x48e4b3cc, 0x7c28ec12, 0xb17986e1,
  0x01735836, 0x1b826322, 0x6602a990, 0x7c1cef68, 0xe102458e, 0xa5564a67, 0x1136b393, 0x98dc0ea1,
  0x3b6f59e5, 0x9efe981d, 0x35fafbe0, 0xc9949ec2, 0x62c765f9, 0x510cab26, 0xbe071300, 0x7ee1d449,
  0xcc71beef, 0xfbb4284e, 0xbfc02ce7, 0xdf734c93, 0x2f8cebcd, 0xfeedc6ab, 0x5476ee54, 0xbd2b5ff9,
  0xf4fd0352, 0x67f9d6ea, 0x7b70db05, 0x5a5f5310, 0x482dd7aa, 0xa0a66735, 0x321ae71f, 0x8e8ad56c,
  0x27a509c3, 0x1690b261, 0x4494b132, 0xc43a42a7, 0x3f60a7a6, 0xd63779ff, 0xe69c1659, 0xd15972c8,
  0x5f6cdb0c, 0xb9415af2, 0x1261ad8d, 0xb70a6135, 0x52ceda5e, 0xd4591dc3, 0x442b793c, 0xe50e2dee,
  0x6f90fc79, 0xd9ecc8f9, 0x063dd233, 0x6cf2e985, 0xe62cfbe9, 0x3466e821, 0x2c8377a2, 0x00b9f14e,
  0x237c4751, 0x40d4a33b, 0x919df7e8, 0xa16991a4, 0xc5295033, 0x5c507944, 0x89510e2b, 0xb5f7d902,
  0xd2d439a6, 0xc23e5216, 0xd52d9de3, 0x534a5e05, 0x762e73d4, 0x3c147760, 0x2d189706, 0x20aa0564,
  0xb07bbc3b, 0x8183e2de, 0xebc28889, 0xf839ed29, 0x532278f7, 0x41f8b31b, 0x762e89c1, 0xa1e71830,
  0xac049bfc, 0x9b7f839c, 0x8fd9208d, 0x2d2402ed, 0xf1f06670, 0x2711d695, 0x5b9e8fe4, 0xdc935762,
  0xa56b794f, 0xd8666b88, 0x6872c274, 0xbc603be2, 0x2196689b, 0x5b2b5f7a, 0x00c77076, 0x16bfa292,
  0xc2f86524, 0xdd92e83e, 0xab60a3d4, 0x92daf8bd, 0x1fe14c62, 0xf0ff82cc, 0xc0ed8d0a, 0x64356e4d,
  0x7e996b28, 0x81aad3e8, 0x05a22d56, 0xc4b25d4f, 0x5e3683e5, 0x811c2881, 0x124b1041, 0xdb1b4f02,
    0x5a72b5cc, 0x07f8d94e, 0xe5740463, 0x498632ad, 0x7357ffb1, 0x0dddd380, 0x3d095486, 0x2569b0a9,
  0xd6e054ae, 0x14a47e22, 0x73ec8dcc, 0x004968cf, 0xe0c3a853, 0xc9b50a03, 0xe1b0eb17, 0x57c6f281,
  0xc9f9377d, 0x43e03612, 0x9a0c4554, 0xbb2d83ff, 0xa818ffee, 0xf407db87, 0x175e3847, 0x5597168f,
  0xd3d547a7, 0x78f3157c, 0xfc750f20, 0x9880a1c6, 0x1af41571, 0x95d01dfc, 0xa3968d62, 0xeae03cf8,
  0x02ee4662, 0x5f1943ff, 0x252d9d1c, 0x6b718887, 0xe052f724, 0x4cefa30b, 0xdcc31a00, 0xe4d0024d,
  0xdbb4534a, 0xce01f5c8, 0x0c072b61, 0x5d59736a, 0x60291da4, 0x1fbe2c71, 0x2f11d09c, 0x9dce266a,
};

inline uint SBox (const char *key, uint len) {
  register uint h = len;
  for ( ; len & ~1; len -= 2, key += 2 )
    h = ( ( ( h ^ SBoxTable[(uint)key[0]] ) * 3 ) ^ SBoxTable[(uint)key[1]] ) * 3;
  if ( len & 1 )
    h = ( h ^ SBoxTable[(uint)key[0]] ) * 3;
  h += ( h >> 22 ) ^ ( h << 4 );
  return h;
}

inline uint sfrand (uint seed) { return seed *= 16807; }

/*
void hwrite (hash_t *t, char *_path) {
  char *path = _path ? strdup (_path) : NULL;
  assert (path);
  vwrite (t->index,  cat (path, ".indx"));
  vwrite (t->offset, cat (path, ".offs"));
  vwrite (t->keys,   cat (path, ".keys"));
  free (path);
}

hash_t *hread (char *_path) {
  char *path = _path ? strdup (_path) : NULL;
  hash_t *t = calloc (1, sizeof (hash_t));
  assert (path);
  t->index  = vread (cat (path, ".indx"));
  t->offset = vread (cat (path, ".offs"));
  t->keys   = vread (cat (path, ".keys"));
  assert (t && t->keys && t->offset && t->index);
  free (path);
  return t;
}
*/

/*
// checks whether value i can be used for a given key
int hvalid (hash_t *t, unsigned i, char *key, unsigned hashcode) {
  if (i == 0) return 1;
  if (t->offset[i-1].code != hashcode) return 0;
  if (!strcmp (key, i2key (t, i))) return 1;
  //fprintf (stdout, "'%s' and '%s' both have hashcode %u\n",
  //  key, i2key (t, i), hashcode); fflush (stdout);
  return 0;
}
*/

// #define hvalid(t,i,k,h) (!i || ((t->offset[i-1].code == h) && !strcmp (k, t->keys + t->offset[i-1].key)))

/*
void *get_blob (mmap_t *map, off_t *offs, uint id) {
  uint n = len(offs), next_id = (id < n) ? id+1 : 0;
  if (!id || id >= n || !offs[id]) return NULL;
  return move_mmap (map, offs[id], offs[next_id] - offs[id]);
}

void *add_blob (mmap_t *map, off_t *offs, off_t size) {
  uint id = len(offs);
  offs = resize_vec (offs, id+1);
  offs[id] = offs[0];
  offs[0] += align8 (size);
  expand_mmap (map, offs[0]);
  return offs;
  // move_mmap (map, offs[id], offs[0] - offs[id]);
}
*/

/*
hash_t *hnew (unsigned num_els, char *_path) {
  char *path = _path ? strdup (_path) : NULL;
  hash_t *t = (hash_t *) calloc (1, sizeof (hash_t));
  t->mode   = path ? write_shared : write_private;
  t->keys   = vnew (num_els * 16, sizeof (char),     cat (path, ".keys"));
  t->offset = vnew (num_els,      sizeof (hkey_t),   cat (path, ".offs"));
  t->index  = vnew (num_els * 2,  sizeof (unsigned), cat (path, ".indx"));
  memset (t->index, 0, vtotal (t->index) * vesize (t->index));
  if (path) free (path);
  return t;
}

hash_t *hmmap (char *_path, access_t access) {
  char *path = _path ? strdup (_path) : NULL;
  hash_t *t = (hash_t *) calloc (1, sizeof (hash_t));
  assert (path && "path to hmmap cannot be null");
  t->mode   = access;
  t->keys   = vmmap (cat (path, ".keys"), access);
  t->offset = vmmap (cat (path, ".offs"), access);
  t->index  = vmmap (cat (path, ".indx"), access);
  free (path);
  return t;
}
*/

/*
hkey_t *mk_key (ulong code, char *key) {
  hkey_t *k = safe_malloc (sizeof(hkey_t) + strlen(key));
  k->code = code; 
  strcpy (k->key, key);
  return k;
}
*/

/* part of old keyIn 
  ///////////////////////////// make there is enough space to add elements
  if (vcount(t->index) >= 0.5 * vtotal(t->index)) 
    hrehash (t, 2 * vtotal(t->index) + 1);
  t->keys = vcheck (t->keys, vcount(t->keys) + len);
  t->offset = vcheck (t->offset, vcount(t->offset) + 1);
  ///////////////////////////// check if the key is already in the table

  ///////////////////////////// enter key into the table if needed
  assert (vcount (t->index) == vcount (t->offset));
  memcpy (t->keys + vcount (t->keys), key, len);
  t->offset [vcount(t->offset)] . key = vcount (t->keys);
  t->offset [vcount(t->offset)] . code = hashcode;
  vcount (t->keys) += len;
  *index = vcount (t->offset) = ++ vcount (t->index);
  return *index;
*/

/*
unsigned hsizeof (hash_t *t) {
  return sizeof (hash_t) + 
    vsizeof (t->keys) + vsizeof (t->offset) + vsizeof (t->index);
}
*/

