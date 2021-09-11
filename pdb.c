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

#include <math.h>
#include <locale.h>
#include "matrix.h"
#include "textutil.h"

// https://www.wwpdb.org/documentation/file-format-content/format33/sect9.html#HETATM
// https://www.wwpdb.org/documentation/file-format-content/format33/sect2.html#HEADER

/*
HEADER    DNA-RNA HYBRID                          05-DEC-94   100D
MODEL        2
ATOM   6654  OE1AGLU A1139     -17.325  39.535  34.165  0.56 23.10           O
ATOM   6655  OE1BGLU A1139     -19.516  38.808  29.507  0.44 26.92           O
ATOM      1  N   HIS A1796      27.257  -9.140  37.352  1.00112.91           N
ATOM      1  O5'   C A   1      -4.549   5.095   4.262  1.00 28.71           O
TER     205        G A  10
ENDMDL
MODEL        3
HETATM    1  N1  MCY A   1       2.931  -4.785   1.818  1.00  0.00           N
HETATM    2  C2  MCY A   1       2.017  -4.867   0.796  1.00  0.00           C
ATOM     32  P    DC A   2       7.767  -2.312   3.534  1.00  0.00           P
*/


// 
//          1         2         3         4         5         6         7         8
// 123456789-123456789-123456789-123456789-123456789-123456789-123456789-123456789-
// HEADER    DNA-RNA HYBRID                          05-DEC-94   100D
// MODEL        2
// TER     205        G A  10
// ENDMDL
//
// ATOM   6654  OE1AGLU A1139     -17.325  39.535  34.165  0.56 23.10           O
// ATOM      1  N   HIS A1796      27.257  -9.140  37.352  1.00112.91           N
// 
// 1-4: "ATOM" 
// 7-11: atom serial number (6-11 allowed, ) ... always uint, 6 always empty?
// 13-16: atom name ... CA, N, C, O, H, CB, HA ... 132+ total
// 17: alternate location indicator ... rarely A or B 
// 18-20: residue name ... 20 + D{ATCGI}
// 22: chain identifier ... A,B,C..X
// 23-26: residue sequence number ... uint, can be -1
// 27: code for insertions of residues ... L,A,B,g,a,c,f,d,e,b...
// 31-38: X ... can be negative [-110 .. 290]
// 39-46: Y
// 47-54: Z
// 55-60: occupancy ... 1.0 >>> 0.5 > 0 > 0.7 > 0.6 ...
// 61-66: temperature ... 12 > 100 > 121 > ...
// 73-76: segment id ... always empty
// 77-78: element symbol ... C O N H D S P SE
//
// every line exactly _80_ chars long
// 1-4: "ATOM"
// 31-38:X 39-46:Y 47-54:Z

typedef struct { // parsed representation of ATOM line
  uint el; // element name: CA, N, C, O, H, CB, HA ..
  uint n; // number in PDB sequence
  uint i; // index in atom_t vector
  float x, y, z;
} atom_t;

#define L2(a) (((a)->x)*((a)->x) + ((a)->y)*((a)->y) + ((a)->z)*((a)->z))

int cmp_patom_el (const void *a, const void *b) { long A = (*(atom_t**)a)->el, B = (*(atom_t**)b)->el, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_patom_i (const void *a, const void *b) { long A = (*(atom_t**)a)->i, B = (*(atom_t**)b)->i, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_patom_n (const void *a, const void *b) { long A = (*(atom_t**)a)->n, B = (*(atom_t**)b)->n, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_patom_x (const void *a, const void *b) { float A = (*(atom_t**)a)->x, B = (*(atom_t**)b)->x, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_patom_y (const void *a, const void *b) { float A = (*(atom_t**)a)->y, B = (*(atom_t**)b)->y, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_patom_z (const void *a, const void *b) { float A = (*(atom_t**)a)->z, B = (*(atom_t**)b)->z, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }

int cmp_atom_el (const void *a, const void *b) { long A = ((atom_t*)a)->el, B = ((atom_t*)b)->el, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_atom_i (const void *a, const void *b) { long A = ((atom_t*)a)->i, B = ((atom_t*)b)->i, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_atom_n (const void *a, const void *b) { long A = ((atom_t*)a)->n, B = ((atom_t*)b)->n, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_atom_x (const void *a, const void *b) { float A = ((atom_t*)a)->x, B = ((atom_t*)b)->x, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_atom_y (const void *a, const void *b) { float A = ((atom_t*)a)->y, B = ((atom_t*)b)->y, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }
int cmp_atom_z (const void *a, const void *b) { float A = ((atom_t*)a)->z, B = ((atom_t*)b)->z, d = A-B; return d>0 ? +1 : d<0 ? -1 : 0; }

char *skipws (char *s) { while (*s == ' ') ++s; return s; }

char *chopws (char *s) {
  while (*s == ' ') ++s; // skip leading spaces
  char *w = s;
  while (*w && *w != ' ') ++w; // find trailing spaces
  *w = '\0'; // null-terminate
  return s;
}

uint span2id (char *beg, uint sz, hash_t *H) {
  char *span = strndup (beg,sz);
  char *key = chopws (span);
  uint id = key2id (H, key);
  free(span);
  return id;
}

atom_t line2atom (char *line, hash_t *ELS, uint i) {
  atom_t atom = {0,0,0,0,0,0};
  atom.i = i;
  atom.n = atoi (skipws (line+6));  //  7-11: atom serial number (uint)
  atom.x = atof (skipws (line+30)); // 31-38: X ... can be negative [-110 .. 290]
  atom.y = atof (skipws (line+38)); // 39-46: Y
  atom.z = atof (skipws (line+46)); // 47-54: Z
  atom.el = span2id (line+12,4,ELS); // 13-16: atom name: CA, N, C, O, H, CB, HA ...
  return atom;
}

int pdb_load (char *_PDB, char *_IDS, char *_ELS) {
  coll_t *PDB = open_coll (_PDB, "a+");  // 
  hash_t *IDS = open_hash (_IDS, "a");
  hash_t *ELS = open_hash (_ELS, "a");
  atom_t *A = NULL;
  ulong NR = 0, NS = 0, NM = 0, NA = 0, NNS = 180;
  char *L = calloc(90,1), *ID = calloc(20,1);
  while (++NR && fgets(L,85,stdin)) {    
    assert (L[80] == '\n');
    L[80] = '\0';
    if (!strncmp(L,"HEADER",6) && ++NS) { // ID = PDB code
      memcpy(ID,L+62,4); ID[4] = '\0'; // 63-66 PDB idCode
    }
    if (!strncmp(L,"MODEL",5)) { // append PDB code with model number
      uint msn = atoi (skipws (L+10)); // 11-14 model serial number
      if (msn > 1) sprintf(ID+4,".%d",msn); // new id: 406D.2 .3 .4 etc
    }
    if (!strncmp(L,"ATOM",4) && ++NA) { // add atoms to vec
      if (!A) A = new_vec (0,sizeof(atom_t));
      atom_t a = line2atom(L,ELS,len(A));
      A = append_vec (A, &a);
    }
    if (!strncmp(L,"END",3) && A && ++NM) { // flush vec to PDB[ID]
      uint id = key2id (IDS, ID);
      put_vec (PDB,id,A);
      free_vec (A);
      A=NULL;      
      if (!(NS%100)) show_progress (NS/1000, NNS, "k structs");
    }
  }
  printf ("\nDONE: %'ld files | %'ld models | %'ld atoms | %d types\n", NS, NM, NA, nkeys(ELS));
  free_coll(PDB);  free_hash(IDS);  free_hash(ELS); free(ID); free(L);
  return 0;
}

void pdb_dump_vec (atom_t *A, uint top, hash_t *ELS, char *ID) {
  atom_t *a, *aEnd = A+(top?top:len(A));
  for (a = A; a < aEnd; ++a) {
    char *el = id2str(ELS,a->el);
    printf ("%s\t%s\t%d\t%.3f\t%.3f\t%.3f\t%.3f\n", ID, el, a->n, a->x, a->y, a->z, sqrt(L2(a)));
    free(el);
  }  
}

void pdb_dump_pvec (atom_t **P, uint top, hash_t *ELS, char *ID) {
  uint i, n = top ? top : len(P);
  for (i = 0; i < n; ++i) {
    atom_t *a = P[i];
    if (!a) continue;
    char *el = id2key(ELS,a->el);
    printf ("%s\t%s\t%d\t%.3f\t%.3f\t%.3f\n", ID, el, a->n, a->x, a->y, a->z);
  }
}

int pdb_dump (char *_PDB, char *_IDS, char *_ELS, char *prm) {
  coll_t *PDB = open_coll (_PDB, "r+");
  hash_t *IDS = open_hash (_IDS, "r");
  hash_t *ELS = open_hash (_ELS, "r");
  int hdr = strstr (prm, "hdr") || strstr (prm, "header");
  int ptr = strstr (prm, "ptr") || strstr (prm, "pointer");
  char *sortN = strstr (prm, "sort:n");
  char *sortX = strstr (prm, "sort:x");
  char *sortY = strstr (prm, "sort:y");
  char *sortZ = strstr (prm, "sort:z");
  uint top = getprm (prm, "top=", 0);
  char *rid = getprms (prm, "dump:", NULL, ',');
  uint id = rid ? key2id (IDS,rid) : 0;
  uint beg = rid ? id : 1, end = rid ? id : nvecs(PDB);  
  if (hdr) printf ("%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "id", "el", "asn", "x", "y", "z", "L2");
  for (id = beg; id <= end; ++id) {
    char *ID = id2key(IDS,id);
    atom_t *A = get_vec(PDB,id);
    //printf ("%d..%d\n", id, len(A));    
    if (!ptr) {
      //fprintf(stderr, "original\n");
      if (sortX) sort_vec (A, cmp_atom_x);
      if (sortY) sort_vec (A, cmp_atom_y);
      if (sortZ) sort_vec (A, cmp_atom_z);
      if (sortN) sort_vec (A, cmp_atom_n);      
      pdb_dump_vec (A, top, ELS, ID);
    }
    else {
      //fprintf(stderr, "pointers\n");      
      atom_t **P = (atom_t**)pointers_to_vec (A);
      if (sortX) sort_vec (P, cmp_patom_x);
      if (sortY) sort_vec (P, cmp_patom_y);
      if (sortZ) sort_vec (P, cmp_patom_z);
      if (sortN) sort_vec (P, cmp_patom_n);
      pdb_dump_pvec (P, top, ELS, ID);
      free_vec (P);
    }
    free_vec (A);
    printf("\n");
  }
  free_coll(PDB);  free_hash(IDS);  free_hash(ELS);  free(rid);
  return 0;
}

int pdb_test (char *_PDB, char *_IDS, char *_ELS) {
  coll_t *PDB = open_coll (_PDB, "r+");
  hash_t *IDS = open_hash (_IDS, "r");
  hash_t *ELS = open_hash (_ELS, "r");    
  uint id, N = nvecs(PDB);
  for (id = 1; id <= N; ++id) {
    char *ID = id2key (IDS,id);
    atom_t *A = get_vec (PDB,id);
    atom_t **P = (atom_t**) pointers_to_vec(A);
    uint i, n = len(A);
    for (i = 1; i < n; ++i) // assert: serial numbers == 1,2...n
      if (A[i].n <= A[i-1].n) printf ("%s serial n:%d <= %d\n", ID, A[i].n, A[i-1].n);
    for (i = 0; i < n; ++i) // assert: pointers == vector elements
      if (P[i] != A+i) printf ("%s pointer P[%d] %d != %d A[%d]\n", ID, i, P[i]->n, A[i].n, i);
    for (i = 0; i < n; ++i) // assert: pointer offse i == vector index A[i].i
      if (P[i]->i != i) printf ("%s P[%d]->i %d != %d\n", ID, i, P[i]->i, i);
    sort_vec (P, cmp_patom_x);
    for (i = 1; i < n; ++i) // assert: increasing X 
      if (P[i]->x < P[i-1]->x) printf ("%s P[%d]->x %.3f < %.3f\n", ID, i, P[i]->x, P[i-1]->x);
    sort_vec (P, cmp_patom_y);
    for (i = 1; i < n; ++i) // assert: increasing Y  
      if (P[i]->y < P[i-1]->y) printf ("%s P[%d]->y %.3f < %.3f\n", ID, i, P[i]->y, P[i-1]->y);
    sort_vec (P, cmp_patom_z);
    for (i = 1; i < n; ++i) // assert: increasing Z  
      if (P[i]->z < P[i-1]->z) printf ("%s P[%d]->z %.3f < %.3f\n", ID, i, P[i]->z, P[i-1]->z);
    sort_vec (P, cmp_patom_n);
    for (i = 0; i < n; ++i) // assert: pointers == vector elements (after re-sorting)
      if (P[i] != A+i) printf ("%s resorted P[%d] %d != %d A[%d]\n", ID, i, P[i]->n, A[i].n, i);
    free_vec(A); free_vec(P);
    if (!(id%100)) show_progress(id/1000,N/1000,"k models");
  }
  free_coll(PDB); free_hash(IDS); free_hash(ELS);
  return 0;
}

int pdb_info (char *prm, char *_PDB, char *_IDS) {
  coll_t *PDB = open_coll (_PDB, "r+");
  hash_t *IDS = *_IDS ? open_hash (_IDS, "r") : NULL;
  uint id, N = nvecs(PDB);
  if (!strcmp(prm,"size")) printf("%s\t%d models\n", _PDB, N);
  else for (id = 1; id <= N; ++id) {
      char *ID = id2str (IDS, id);
      uint NA = len_vec (PDB, id); // chunk_sz(PDB,id)  - sizeof(vec_t) / sizeof(atom_t) ??
      printf ("%s\t%d atoms\n", ID, NA);
      free(ID);
    }
  free_coll(PDB); free_hash(IDS);
  return 0;
}

atom_t **mk_balls (atom_t *A, float R, uint el) {
  uint i, n = len(A);
  atom_t **B = new_vec (n, sizeof(atom_t*)); // B[i] = ball of radius R centered on A[i]
  for (i = 0; i < n; ++i) {
    atom_t *a, *b = A+i; // center atom
    if (b->el != el) continue; // wrong element
    B[i] = new_vec (0, sizeof(atom_t));
    for (a = A; a < A+n; ++a) {
      float dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;
      if (dx > +R || dy > +R || dz > +R ||
	  dx < -R || dy < -R || dz < -R) continue; // outside cube
      if (sqrt (dx*dx + dy*dy + dz*dz) > R) continue; // outside ball
      B[i] = append_vec(B[i],a); // inside
    }
  }
  return B;
}

void center_ball (atom_t *B, atom_t *c) { // center every atom in the ball around c
  atom_t *b = B-1, *end = B+len(B);
  while (++b < end) { b->x -= c->x; b->y -= c->y; b->z -= c->z; }
}

void put_balls (atom_t *A, atom_t **B, char *ID, coll_t *BALLS, hash_t *BIDS) {
  char BID[99]; uint i, n = len(A);
  for (i=0; i<n; ++i) {
    if (!B[i]) continue;
    sprintf (BID, "%s:%d", ID, A[i].n); // ball id = CODE:POS e.g. 100D:1234
    uint bid = key2id (BIDS, BID);
    center_ball (B[i],A+i);
    put_vec (BALLS, bid, B[i]);
  }
}

int pdb_ball (char *prm, char *_BALLS, char *_BIDS, char *_PDB, char *_IDS, char *_ELS) {
  coll_t *BALLS = open_coll (_BALLS, "w+");
  hash_t *BIDS = open_hash (_BIDS, "w");
  coll_t *PDB = open_coll (_PDB, "r+");
  hash_t *IDS = open_hash (_IDS, "r");
  hash_t *ELS = open_hash (_ELS, "r");  
  float R = getprm(prm,"r=",5); // ball radius (Angstroms)
  uint el = strstr(prm,",CA") ? key2id (ELS, "CA") : 0; // center element (CA)  
  uint id, N = nvecs(PDB);
  for (id = 1; id <= N; ++id) {
    char *ID = id2key (IDS,id);    
    atom_t  *A = get_vec (PDB,id);
    atom_t **B = mk_balls (A, R, el);
    //printf ("%s: %d atoms, %d balls\n", ID, len(A), num_balls(B));
    put_balls (A, B, ID, BALLS, BIDS);
    free_vec (A);
    free_2D ((void**)B);
    show_progress (id/1000, N/1000, "k models");
  }
  free_coll(BALLS); free_coll(PDB); free_hash(BIDS); free_hash(IDS); free_hash(ELS);
  return 0;
}

ix_t *ball2peel (atom_t *A) { // i:element x:distance to origin
  atom_t *a;
  ix_t *V = new_vec (0, sizeof(ix_t));
  for (a = A; a < A+len(A); ++a) {
    float x = a->x, y = a->y, z = a->z;
    ix_t new = {a->el, sqrt(x*x + y*y + z*z)}; 
    V = append_vec (V, &new);
  }
  sort_vec (V, cmp_ix_i);
  return V;
}

int pdb_peel (char *_PEELS, char *_BALLS) {
  coll_t *PEELS = open_coll (_PEELS, "w+");
  coll_t *BALLS = open_coll (_BALLS, "r+");
  uint id, N = nvecs(BALLS);
  for (id = 1; id <= N; ++id) {
    atom_t *A = get_vec (BALLS, id);
    ix_t *V = ball2peel (A);
    put_vec (PEELS, id, V);
    free_vec (A); free_vec (V);
    show_progress (id/1000, N/1000, "k peels");
  }
  free_coll(BALLS); free_coll(PEELS);
  return 0;
}

jix_t *ball2jix (atom_t *A) { // j:element i:offset x:distance
  jix_t *V = new_vec (0, sizeof(jix_t));
  atom_t *a;
  for (a = A; a < A+len(A); ++a) {
    float x = a->x, y = a->y, z = a->z, d = sqrt (x*x + y*y + z*z);
    jix_t new = {a->el, a-A, d};
    V = append_vec (V, &new);
  }
  sort_vec (V, cmp_jix); // increasing j:element then i:offset
  return V;
}

void pdb_dump_atom (char *id, atom_t *a) {
  printf ("%s: %d %d %.3f %.3f %.3f %.3f\n", id, a->i, a->el, a->x, a->y, a->z, sqrt(L2(a)));
}

void pdb_show_L2s (char *id, atom_t *A, uint n) {
  atom_t *a;
  printf ("%s el:%d [%d]:", id, A->el, n);
  for (a=A; a<A+n; ++a) printf (" %d:%.3f", a->i, sqrt(L2(a)));
  printf ("\n");
}

uint count_same_el (atom_t **a, atom_t **end) { // count atoms of the same element as a
  atom_t **x = a; 
  while (x < end && (*x)->el == (*a)->el) ++x;
  return x-a; 
}

float L2diff (atom_t *a, atom_t *b) {
  if (!a || !b) return 1000;
  float L2a = sqrt(L2(a)), L2b = sqrt(L2(b));
  return ABS (L2a - L2b);
}

typedef struct {atom_t *a, *b;} ab_t;

ab_t *add_ab (ab_t *AB, atom_t *a, atom_t *b) { ab_t ab={a,b}; return append_vec (AB, &ab); }

atom_t aNULL = {0, 0, 0, 1000, 1000, 1000}, *pNULL = &aNULL;

atom_t **nearest (atom_t **a, atom_t **B, int nb, atom_t **skip) {
  atom_t **best = &pNULL, **b = B-1, **end = B+nb;
  for (b = B; b < end; ++b)
    if ((b!=skip) && *b && (!best || L2diff(*a,*b) < L2diff(*a,*best))) best = b;
  return best;
}

ab_t *align_sub_balls (ab_t *AB, atom_t **A, int na, atom_t **B, int nb, float near, float far) {
  atom_t **a;
  for (a = A; a < A+na; ++a) {
    atom_t **b  = nearest (a, B, nb, 0); // b  is match for a
    atom_t **b2 = nearest (a, B, nb, b); // b2 is 2nd best for a
    atom_t **a2 = nearest (b, A, na, a); // a2 is 2nd best for b
    float ab = L2diff(*a,*b), ab2 = L2diff(*a,*b2), ba2 = L2diff(*b,*a2);
    uint ok = (ab < near) && (ab2 > far) && (ba2 > far); // a,b near; a2,b2 far
    if (ok) {
      AB = add_ab (AB, *a, *b); // align a <-> b
      *a = *b = NULL; // do not reuse a,b
    }
  }
  return AB;
}

ab_t *align_balls (atom_t **A, atom_t **B, float near, float far) {
  ab_t *AB = new_vec (0, sizeof(ab_t));
  atom_t **a=A, **aEnd = A+len(A); sort_vec (A, cmp_patom_el);
  atom_t **b=B, **bEnd = B+len(B); sort_vec (B, cmp_patom_el);
  while (a < aEnd && b < bEnd) {
    if      ((*a)->el < (*b)->el) ++a; // skip lonely a
    else if ((*a)->el > (*b)->el) ++b; // skip lonely b
    else { // a,b = same element => posible match
      uint na = count_same_el(a,aEnd), nb = count_same_el(b,bEnd);
      if (na>1 || nb>1) { // a[0:na] <-> b[0:nb]
	AB = align_sub_balls (AB, a, na, b, nb, near, far); 
      }
      else if (L2diff(*a,*b) < near) {
	AB = add_ab(AB,*a,*b); // a <-> b
	*a = *b = NULL;	// disable a,b
      }
      a += na;
      b += nb;
    }
  }
  return AB;
}

void print_alignment (ab_t *AB, atom_t **A, atom_t **B, hash_t *ELS) {
  ab_t *ab; 
  for (ab = AB; ab < AB+len(AB); ++ab) {
    atom_t *a = ab->a, *b = ab->b;
    float da = sqrt(L2(a)), db = sqrt(L2(b)), dab = ABS(da-db);
    assert (a->el == b->el);
    char *el = id2key (ELS,a->el);
    printf ("AB %.3f %s %+.3f %+.3f %+.3f %+.3f %+.3f %+.3f\n",
	    dab, el, a->x, a->y, a->z, b->x, b->y, b->z);
  }
  atom_t **p; uint na=0, nb=0;  
  printf("\n");
  for (p = A; p < A+len(A); ++p) if (*p) {
      atom_t a = **p; ++na;
      char *el = id2key (ELS,a.el);
      printf ("A %.3f %s %+.3f %+.3f %+.3f\n", sqrt(L2(&a)), el, a.x, a.y, a.z);
    }
  printf("\n");
  for (p = B; p < B+len(B); ++p) if (*p) {
      atom_t b = **p; ++nb;
      char *el = id2key (ELS,b.el);      
      printf ("B %.3f %s %+.3f %+.3f %+.3f\n", sqrt(L2(&b)), el, b.x, b.y, b.z);
    }
  //printf ("%d aligned, left: %d=%d %d=%d \n", len(AB), na, len(A)-len(AB), nb, len(B)-len(AB));
}

int pdb_align (char *_BALLS, char *_BIDS, char *_ELS, char *_a, char *_b, char *prm) {
  coll_t *BALLS = open_coll (_BALLS, "r+");
  hash_t *BIDS = open_hash (_BIDS, "r");
  hash_t *ELS = open_hash (_ELS, "r");
  float near = getprm(prm,"near=",0.1), far = getprm(prm,"far=",0.5);
  //char *aid = getprms(prm,"a=",0,','), *bid = getprms(prm,"b=",0,',');
  uint a = key2id (BIDS,_a), b = key2id (BIDS,_b);
  atom_t *_A = get_vec (BALLS,a), *_B = get_vec (BALLS,b);
  atom_t **A = (atom_t**)pointers_to_vec (_A);
  atom_t **B = (atom_t**)pointers_to_vec (_B);
  ab_t *AB = align_balls (A, B, near, far);
  print_alignment (AB, A, B, ELS);
  free_coll(BALLS); free_hash(BIDS); free_hash(ELS);
  free_vec(_A); free_vec(_B); free_vec(A); free_vec(B); free_vec(AB);
  return 0;
}

float *atofv (char *a, char sep) {
  char **S = split (a, sep); uint i, n = len(S);
  float *X = new_vec (n, sizeof(float));  
  for (i=0; i<n; ++i) X[i] = atof(S[i]);
  free_vec (S);
  return X;
}

ix_t *peel2hist (ix_t *P, hash_t *ELS, hash_t *BINS, float *thr) {
  uint i; char _[99];
  ix_t *H = const_vec (len(P), 1);
  for (i = 0; i < len(P); ++i) {
    char *el = id2key(ELS,P[i].i);
    float x = P[i].x, *t = thr, *last = thr+len(thr)-1;
    while (*t <= x && t <= last) ++t; // 1st threshold above x       x t
    float *lo1 = MAX(thr,t-1), *hi1 = MIN(last,t);     // 0-----2---.-4-----6
    //float *lo2 = MAX(thr,t-1), *hi2 = MIN(last,t+1);   // ---1-----3-----5---
    //printf ("%s %.3f -> [%.1f,%.1f) [%.1f,%.1f)\n", el, x, 
    H[i].i   = key2id(BINS,fmt(_,"%s_%.1f_%.1f",el,*lo1,*hi1)); // 2..4
    //H[2*i].i   = key2id(BINS,fmt(_,"%s_%.1f_%.1f",el,*lo1,*hi1)); // 2..4
    //H[2*i+1].i = key2id(BINS,fmt(_,"%s_%.1f_%.1f",el,*lo2,*hi2)); // 3..5        
  }
  sort_vec (H, cmp_ix_i);
  uniq_vec (H);
  return H;
}

ix_t *quantize_peel (ix_t *P, hash_t *ELS, hash_t *BINS, float dx) {
  uint i; char _[99];
  ix_t *H = const_vec (2*len(P), 1);
  for (i = 0; i < len(P); ++i) {
    char *e = id2key(ELS,P[i].i);
    float k = floorf(P[i].x / dx), K = MAX(0,k-1);
    //H[i].i  = key2id(BINS,fmt(_,"%s_%.1f_%.1f",e,k*dx,(k+2)*dx));
    H[2*i].i   = key2id(BINS,fmt(_,"%s_%.1f_%.1f",e,K*dx,(k+1)*dx));
    H[2*i+1].i = key2id(BINS,fmt(_,"%s_%.1f_%.1f",e,k*dx,(k+2)*dx));
  }
  sort_vec (H, cmp_ix_i);
  uniq_vec (H);
  return H;
}

int pdb_hist (char *_HIST, char *_BINS, char *_PEEL, char *_ELS, char *prm) {
  coll_t *HIST = open_coll (_HIST, "w+");
  hash_t *BINS = open_hash (_BINS, "w");
  coll_t *PEEL = open_coll (_PEEL, "r+");
  hash_t *ELS  = open_hash (_ELS, "r");
  float dx = getprm(prm,"dx=",1);
  //float *thr = atofv (_bnd, ',');
  //printf("thresholds: %.1f %.1f ... %.1f\n", thr[0], thr[1], thr[len(thr)-1]);
  uint id, N = nvecs(PEEL);
  for (id = 1; id <= N; ++id) {
    ix_t *P = get_vec (PEEL, id);
    //ix_t *H = peel2hist (P, ELS, BINS, thr);
    ix_t *H = quantize_peel (P, ELS, BINS, dx);
    put_vec (HIST, id, H);
    free_vec (H); free_vec (P);
    show_progress (id/1000, N/1000, "k histograms");
  }
  free_coll(HIST); free_coll(PEEL); free_hash(BINS); free_hash(ELS); 
  return 0;
}

int pdb_eval (char *_SIMS, char *_BALL, char *_BIDS, char *_ELS, char *prm) {
  coll_t *SIMS = open_coll (_SIMS, "r+");
  coll_t *BALL = open_coll (_BALL, "r+");
  hash_t *BIDS = open_hash (_BIDS, "r");
  hash_t *ELS  = open_hash (_ELS, "r");
  uint top = getprm (prm, "top=", 0);
  char *QID = getprms (prm, "id=", NULL, ',');
  uint qid = QID ? key2id (BIDS, QID) : 0;
  printf ("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "id", "el", "asn", "x", "y", "z", "L2", "sim");
  ix_t *S = get_vec (SIMS, qid), *s;
  uint N = top ? MIN(top,len(S)) : len(S);
  sort_vec (S, cmp_ix_X);
  for (s = S; s < S+N; ++s) {
    char *ID = id2key(BIDS,s->i);
    atom_t *A = get_vec(BALL,s->i), *a;
    for (a = A; a < A+len(A); ++a) {
      char *el = id2key(ELS,a->el);
      printf ("%s\t%s\t%d\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n", ID, el, a->n, a->x, a->y, a->z, sqrt(L2(a)), s->x);
    }
    free_vec (A);
  } 
  free_vec (S); free(QID);
  free_coll (SIMS); free_coll (BALL); free_hash (BIDS); free_hash (ELS);
  return 0;
}

char *usage = 
  "pdb load PDB IDS ELS ... read PDB structures from stdin\n"
  "pdb size PDB         ... number of models\n"
  "pdb info PDB [IDS]   ... size for each model\n"  
  "pdb test PDB IDS ELS ... sanity checks on PDB and sorting\n"
  "pdb dump:100D PDB IDS ELS ... dump PDB structures to stdout\n"
  "pdb ball,r=5,CA BALLS BIDS PDB IDS ELS ... 5A balls around every CA atom\n"
  "pdb peel PEELS BALLS\n"
  "pdb align,near=0.1,far=0.5 BALLS BIDS ELS A B ... align balls with ids A,B\n"
  "pdb hist,dx=1 HIST BINS PEELS ELS ... quantize distances into el_bin\n"
  "pdb eval,id=ID,top=10 SIMS BALLS BIDS ELS ... dump balls in SIMS[ID] for eval\n"
  ;

#define a(i) ((i < argc) ? argv[i] : "")

int main (int argc, char *argv[]) {
  setlocale(LC_NUMERIC, "");
  if      (!strncmp(a(1),"load",4)) return pdb_load (a(2), a(3), a(4));
  else if (!strncmp(a(1),"info",4)) return pdb_info (a(1), a(2), a(3));
  else if (!strncmp(a(1),"size",4)) return pdb_info (a(1), a(2), a(3));
  else if (!strncmp(a(1),"test",4)) return pdb_test (a(2), a(3), a(4));
  else if (!strncmp(a(1),"dump",4)) return pdb_dump (a(2), a(3), a(4), a(1));
  else if (!strncmp(a(1),"ball",4)) return pdb_ball (a(1), a(2), a(3), a(4), a(5), a(6));
  else if (!strncmp(a(1),"peel",4)) return pdb_peel (a(2), a(3));
  else if (!strncmp(a(1),"alig",4)) return pdb_align (a(2), a(3), a(4), a(5), a(6), a(1));
  else if (!strncmp(a(1),"hist",4)) return pdb_hist (a(2), a(3), a(4), a(5), a(1));
  else if (!strncmp(a(1),"eval",4)) return pdb_eval (a(2), a(3), a(4), a(5), a(1));
  else return printf ("%s", usage);
  return 0;
}


//uint *rowmin (float **X) { // index of minimal column for each row
//  uint r, c, m, nr = len(X), nc = len(X[0]), *M = new_vec (nr, sizeof(uint));
//  for (r=0; r<nr; ++r) {    
//    for (m=c=0; c<nc; ++c) if (X[r][c] < X[r][m]) m = c;
//    M[r] = m;
//  }
//  return M;
//}

//uint *colmin (float **X) { // index of minimal row for each column
// uint r, c, m, nr = len(X), nc = len(X[0]), *M = new_vec (nc, sizeof(uint));
//  for (c=0; c<nc; ++c) {    
//    for (m=r=0; r<nr; ++r) if (X[r][c] < X[m][c]) m = r;
//    M[c] = m;
//  }
//  return M;
//}

//int best_col_in_row (float **AB, int r, int nc, int skip) {
//  int best = skip ? 0 : 1, c;
//  for (c = 0; c < nc; ++c) if (c != skip && AB[r][c] < AB[r][best]) best = c;
//  return best;
//}
//
//int best_row_in_col (float **AB, int c, int nr, int skip) {
//  int best = skip ? 0 : 1, r;
//  for (r = 0; r < nr; ++r) if (r != skip && AB[r][c] < AB[best][c]) best = r;
//  return best;
//}

//float *nearest (float a, float *B, float *skip) {
//  float *best = (B != skip) ? B : B+1, *b = B-1, *end = B+len(B);
//  while (++b < end) if (b != skip && ABS(a-*b) < ABS(a-*best)) best = b;
//  assert (best >= B && best < end && best != skip);
//  return best;
//}

//float *L2s (atom_t *A, int n) { 
//  float *X = new_vec(n,sizeof(float));
//  while (--n>=0) X[n] = sqrt(L2(A+n));
//  return X;
//}
