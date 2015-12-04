

#define UP(i) ((uint)((i)/2))
#define DOWN(i) (2*(i))
#define RIGHT(i) (2*(i)+1)

inline char *down(char *trie, char *p) {
  
}

char *trie_ins (char *T, uint *N, char char *word) {
  uint i=1;
  
  for (w = word; *w; ++w) {
    while (T[i] && T[i] != *w) i = RIGHT(i); // find a place for *w
    if (!T[i]) T[i] = *w; // found an empty slot => insert
    i = DOWN(i);
    
    if (T[i] == 0) { T[i] = *w; i = DOWN(i); }
    if (T[i] == *w) i = DOWN(i);
    else 
    
  }
  
  
}
