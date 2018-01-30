/*

------  http://lexicalresearch.com/software.html ------

Kstem is a morphology routine for information retrieval.
It is designed to return a root rather than a truncated
word form, and to group word forms on the basis of semantics.
The code is open-source, and is licensed under the LGPL.

Kstem Documentation
-------------------

Kstem is a morphological analyzer that reduces morphological variants to a
root form.  For example, `elephants'->`elephant', `amplification'->`amplify',
and `european'->`europe'.  This type of reduction is the norm in information
retrieval, and is referred to as "stemming".  Unlike previous stemmers, Kstem
tries to avoid conflating variants that have different meanings.  For example
`memorial' is related to `memory', and `memorize' is also related to `memory',
but reducing those variants to `memory' would also conflate `memorial' with
`memorize'.  For more information, please see "Viewing Morphology as an Inference
Process", by Robert Krovetz, Proceedings of the ACM-SIGIR Conference on Research
and Development in Information Retrieval, pp. 191-202, 1993.

The primary advantage of Kstem over previous stemmers (e.g, the
Porter stemmer) is that it returns words instead of truncated word forms.
It is also much easier to modify, and very flexible.  It will allow any
word form to be kept as is, and it will allow any word form to be conflated
to any other word form.  In general, Kstem requires a word to be in the
lexicon (the basic list of words that the system knows about) before it
will reduce one word form to another.  Some endings are highly productive,
and Kstem will remove them even if the root form is not found in the
lexicon (e.g., `-ness', `-ly').  If a variant is explicitly mentioned in
the lexicon, it is assumed to be unrelated to the (presumed) root, and
the conflation is not done.  For example, the lexicon needs to contain
the word `curly', or it would be reduced to the presumed root, `cur'.
Similarly, the word `factorial' needs to be included, or it would be
reduced to `factory' (by analogy to `matrimonial'->`matrimony').  In
contrast, we want to allow `immunity'->`immune', but avoid reducing
`station'->`state', or `authority'->`author'.  This is done by making
sure that the root form (`immune') is mentioned in the lexicon, and
omitting any variant (`immunity') that you want to be related to that root.

There are instances in which we need an explicit mention of which variant
is related to a root.  For example, irregular morphology (`matrices'->`matrix')
or cases in which the reduction would not be permitted due to length
restrictions (`doing'->`do').  A direct-conflation file is used to allow
these reductions to be performed.  This file can also be used to over-ride
the normal operation of the system.  That is, if the user wants one word
form to be reduced to another, this file can be used to "hard-wire" that
result.  It consists of a list of pairs between the original word form and
the word form that results.  This file should be used with care - it is
possible to repeatedly stem a file and obtain different results than
stemming it just once (in general, once a file is stemmed, stemming it
again will not change it).

The routine requires several files that store the basic lexicon, and explicit
conflations due to irregular morphology.  The environment variable STEM_DIR
should be set to the name of the directory where those files are located.
The list of files is as follows:

   head_word_list.txt   -   the basic list of words in the lexicon

   dict_supplement.txt  -   this is an additional list of words that
                            can be used without entailing a modification
                            of the basic word list.  This is used to
                            allow (or prevent) domain specific conflations.

   e_exception_words.txt -   this is used to determine whether to retain an
                            "e" at the end of a word.  For example, `suites'
                            is reduced to `suite', but `suited' to `suit'.
                            This list tells us which words are in that category.

   direct_conflations.txt - this allows us to directly conflate one word form
                            to another.  It is used to allow `doing'->`do',
                            which would normally not be allowed because of
                            length restrictions, and to provide a way to
                            over-ride the normal rules used by the system.

   country_nationality.txt - this is a list of conflations between nationalities
                             and countries (e.g., `British'->`Britain') or
                             words relatng to continents (`European'->`Europe').
                             It *could* be part of the direct-conflations file,
                             but it is kept separate for ease of modification.

   proper_nouns.txt     -   this is a list of proper nouns that we do not want
                            to have stemmed (e.g., `Italy', `Athens', and
                            `Inverness').  Just as the country_nationality file
                            is kept separate from the direct_conflation file,
                            this is kept separate from the dict_supplement
                            file for the same reason.

I have set the file permissions for all of the lexicon files to be read-only,
except for dict_supplement.txt and direct_conflations.txt.  Those are the
only files you need to change.  If a word is being stemmed and you do not
want it to be stemmed, then add it to the dict_supplement file.  For example,
if xyzness was being stemmed to xyz and you didn't want that to happen,
add "xyzness" (without the quotes) to the dict_supplement.txt file.  If
you want a particular reduction to occur, add it in the direct_conflation
file.  For example, if you want "won" to be converted to "win", then add
the line: "won win" to the direct_conflation file.

To use Kstem, you must first make a call to read_dict_info().  This loads the
above files into memory.  The stemmer is then called by saying: stem(word, thestem),
where "word" and "thestem" are pointers to characters (char *).  The user is
responsible for allocating storage for the input word and the result (thestem).
Both read_dict_info and stem are of type VOID.

The distribution includes the above mentioned files, as well as public-kstem.c
(the source code file), test-kstem.c (a routine to interactively determine the
value returned for any word-form), kstem-file.c (example source code for stemming
all the words in a file), and a makefile.  It also includes hash.c and hash.h,
which support the use of hash tables.

This release of Kstem uses a modified public-domain list of words as the
head_word_list, and the list is still being refined to obtain the best
performance.  It should be considered a beta-release of the system,
and several roots and variants may be missing from the file.

For comments and questions about Kstem, please contact Bob Krovetz
(kstem@lexicalresearch.com)


   This is a stemmer that handles inflectional morphology and the
   most common forms of derivational morphology.  It first checks a 
   word against the dictionary, and if it is found it leaves it alone.
   If not, it handles inflectional endings (plurals into singular form,
   and past tense and "ing" endings into present tense), and then
   conflates the most common derivational variants.
   
   Author: Bob Krovetz
   
   Version 15 - This is the first distributable version.  The 
   basic lexicon (using the Longman dictionary) was 
   replaced by a modified  /usr/dict/words (a standard 
   file on Unix systems), and hard-coded pathnames were 
   replaced with environment variables.  The changes
   made in version 14 were undone.
   
   
   Version 14 - Even if variants are mentioned in the dictionary,
   ignore them for certain endings (inflectional, 
   -ion, -ment, and -ity).
   
   Note: this version was done as part of an effort
   to conflated word forms that are in the dictionary.
   This does make some correct conflations, but it
   will also conflate state/station, depart/department.
   It is not enough for a word to have the potential to
   be a variant, there has to be evidence to relate them.
   
   
   Version 13 - Includes a supplemental dictionary, and three direct
   mapping tables (one for run-ons,  another for
   proper nouns such as `British->Britain', and
   a third for simple sense-links between words in
   the dictionary).  The supplemental dictionary is
   used to allow conflations (by specifying a root),
   or to prevent them (by specifying a variant).
   For example, if `factorial' is not in the lexicon,
   it would be converted to `factory' (analogous to
   `matrimonial'/`matrimony').
   
   
   Previous versions involved the basic inflectional morphology
   (plurals, tensed verbs, and aspect), and tests of specific 
   derivational endings.
   
   
   Modification history:
   8/26/92   Revised to use hash tables for dictionary look-up instead 
   of an indexed sequential file
   
   12/13/92   Revised to also do some simple derivational conflation.
   Initially this will be conservative, and not conflate
   forms that are in the dictionary (most of them).  If
   the form isn't in the dictionary, and the stem isn't
   in the dictionary, we will attempt a reasonable guess
   at the stem (for some endings), otherwise we will leave
   it alone.
   
   7/26/93   Modified the structure of a hashtable entry.  We can now
   include more complex structures.  This allows us to make
   direct mappings between lexical items, which is necessary
   for reducing `periodically->periodic', `Britain->British',
   and various irregular variations.  This also gives us
   much greater flexibility, since any word form can be reduced
   to any other word form.  A supplemental dictionary is also
   provided for.  This allows words to be left unstemmed, and
   lets some stemming go through (since the stemmer wants the
   root form to appear in the dictionary).
   
   
   8/2/93    Revised to bypass the dictionary for certain endings
   (inflected forms, -ion, -ment, and -ity).  That is, 
   even if the dictionary contains words with these endings,
   we will check to if there is a possible root which is
   also in the dictionary, and use that if it is found.
   This results in further conflations, but also causes
   a number of errors (e.g., `appointment/appoint', but
   `department/depart').  Feh!  
   
   8/3/93    The file was changed so that the dictionary is no longer
   bypassed.
   
   5/25/94    Modified to use environment variables, and generally
   changed to be made available for distribution.

   12/3/94 (MIK) Removed all integer to pointer casts (which caused
   segmentation faults on 64-bit machines).  A dynamic table is created
   in memory to hold dictentry's for each direct conflation, proper noun
   and exception word, plus one default entry to cover all the "normal"
   words in the dictionary.  The hash dictionary now stores the index into
   this table for each word in the dictionary.  A new call, getdep, was
   added to get the actuall dictentry for a word.  Also, some code was
   cleanned up. (using fprintf(stderr, instead of exit(), etc.)

   12/8/94 (MIK) Added the unit testing code

   3/20/95 (RJK) Fixed bug with -ble, -nce, -ncy.

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#define vowel(i) (!cons(i))
#define TRUE 1
#define FALSE 0

#define MAX_WORD_LENGTH 25
#define MAX_FILENAME_LENGTH 125  /* including the full directory path */
#define MAX_ROOTS 9000           /* maximum number of root forms in the lexicon */

/* These macros expand to expressions which evaluate to the following: */

#define wordlength (k + 1)     /* the length of word (not an lvalue) */
#define stemlength (j + 1)     /* length of stem within word (not an lvalue) */
#define final_c    (word[k])   /* the last character of word */
#define penult_c   (word[k-1]) /* the penultimate character of word */

#define DIR_SEP_CHAR '/'
#define DIR_SEP_STRING "/"

typedef struct {
 char flag;
 char word1[MAX_WORD_LENGTH];
 char stem1[MAX_WORD_LENGTH];
 char word2[MAX_WORD_LENGTH];
 char stem2[MAX_WORD_LENGTH];
} kstem_HSLOT;


/* These 2 macro used "sizeof(s)-1", then someone changed it to use
   "strlen(s)-1", and forgot to remove the "-1". -- RCC 3/8/96 */
#define ends_in(s) ends(s, strlen(s))      /* s must be a string constant */
#define setsuffix(s) setsuff(s, strlen(s)) /* s must be a string constant */

/* ---------------------------  Hash Table Stuff ----------------------------*/
/* this is a subset of hashobj.h and hashobj.c */

static int stemhtsize;
static kstem_HSLOT *stemht;
static int stemht_init_flag = 0;
static void stemht_init();
#define  stemhash(word, hval){ unsigned short int ptr[6]; strncpy((char *)ptr, word, 12); hval = ((ptr[0]<<4)^ptr[1]^ptr[2]^ptr[3]^ptr[4]^ptr[5]) % stemhtsize; }

#define LOAD_FACTOR (0.90)
#define MIN_ENTRIES (32)   /* should be > (1/1-LOAD_FACTOR); 32 is generous */
#define EXP_FACTOR (2)
#define AVG_STRINGSIZE (8)

#define MATCH(a, b) (strcmp((a),(b)) == 0) /* for keys that are strings */
#define HASH2(a) (a->hashdvsr2 - (a->hvalue % a->hashdvsr2))
#define EMPTY (-1)
#define FAIL (-1)


typedef struct            /* hash table entry */
    {
    int id;             /* "value"; can be anything except FAIL (= -1) */
    int string;         /* "key"; offset of string in string space */
    } hte;

typedef struct            /* hash table object: per-object variables */
    {
    int tblsize;        /* number of entries in hash table */
    int used;           /* number of entries used */
    int hashdvsr2;      /* divisor for second hash function */
    int stringspcsize;  /* size of the string space */
    int stringtop;      /* offs of next available location in string space */
    int hvalue;         /* temp used by hash functions */
    int lastkey;        /* key of an empty entry, from last lookup */
    char *stringspc;      /* base of the string space */
    hte *htable;          /* base of table, which is an array of htes */
    char name[MAX_FILENAME_LENGTH]; /* filename of hashobj on disk */
    } hashobj;

/* ------------------------- Function Declarations --------------------------*/
#define MAX(A,B)  ((A) > (B) ? (A) : (B))
hashobj *get_hashobj(int);
hashobj *read_hashobj(int, char *);
int ho_find_ent(hashobj *t, char *s);
int ho_lookup(hashobj *, char *);
int ho_insert(hashobj *, char *, int);
int set_size_hashobj(hashobj *, double);
void free_hashobj(hashobj *);
int  ho_size(hashobj *);
int  ho_used(hashobj *);
double ho_load_factor();
static int stralloc(hashobj *, int);
static int hash1(hashobj *, char *);
static int prime_lte(int);
static int prime_gte(int) ;
static int prime(int);

/* ---------------------------------------------------------------------------
get_hashobj() creates and initializes a hash table object, and returns a
pointer to it.  The caller provides an estimated number of entries.  The
hashobj created will be big enough to hold this number of entries before
it expands either the hash table or the string space (this is not
guaranteed for the string space).

For best performance on small dictionaries, double the initial size.
*/

hashobj *get_hashobj(int entries)
{
    hashobj *t;       /* t stands for "this", the current object, as in c++ */
    int j;
    
    entries = (int)MAX(entries, MIN_ENTRIES); 
    t = (hashobj *)calloc((size_t)1, (size_t)sizeof(hashobj));
    if (t)
	{
	t->tblsize = prime_gte(entries);
	t->hashdvsr2 = prime_lte(t->tblsize - 2);
	t->stringspcsize = t->tblsize * AVG_STRINGSIZE;
	t->stringspc = (char *)calloc((size_t)t->stringspcsize, (size_t)1);
	t->htable = (hte *)malloc((size_t)(t->tblsize * sizeof(hte)));
	}
    if (t == 0 || t->stringspc == 0 || t->htable == 0)
	{
	fprintf(stderr,"Error allocating hash table memory");
	return(NULL);
	}
    for (j = 0; j < t->tblsize; j++) {
        t->htable[j].id = EMPTY;
	t->htable[j].string = EMPTY;
      }

    return(t);
}

/* ---------------------------------------------------------------------------
ho_lookup() returns FAIL if term not found, the term id otherwise .
*/

int ho_lookup(hashobj *t, char *s)
{
    int index;
    
    index = ho_find_ent(t, s);
    if (index == FAIL)
        return(FAIL);
    return(t->htable[index].id);        /* success */
}

/* ---------------------------------------------------------------------------
ho_find_ent() returns FAIL if term not found, the hash table index otherwise .
If term is not found, it sets lastkey to point to the first empty entry
found.
*/

int ho_find_ent(hashobj *t, char *s)
{
    int key, firstkey;
    int probe_step = 0;
    
    for(firstkey = key = hash1(t, s); t->htable[key].string != EMPTY; )
	{
	if (MATCH(t->stringspc + t->htable[key].string, s))
	{
	    t->lastkey = key;
	    return(key);                                   /* success */
	}
	if (probe_step == 0)
	    probe_step = HASH2(t);
	key = (key + probe_step) % t->tblsize;
	if (key == firstkey)
	    {
	    fprintf(stderr,"Error in lookup: searched entire table unsuccessfully");
	    return(FAIL);
	    }
	}

    t->lastkey = key;
    return(FAIL);
}       

/* ---------------------------------------------------------------------------
ho_insert() returns key if successful, FAIL otherwise
*/

int ho_insert(hashobj *t, char *s, int id)
{
    hte e;
    
    if (id == FAIL)
	{
	fprintf(stderr,"Attempted to insert string %s with id = FAIL", s);
	return(FAIL);
	}

    if (((double)t->used / t->tblsize) > LOAD_FACTOR)
	{ /* increase the size of the hashobj by EXP_FACTOR */
	  if (set_size_hashobj(t, 1.0 / EXP_FACTOR * LOAD_FACTOR) == -2)
	    return FAIL;   /* no memory */
	  ho_lookup(t, s); /* must lookup again to set lastkey */
	}
    
    if (t->htable[t->lastkey].string == EMPTY)
    {
	e.id = id;
	e.string = stralloc(t, strlen(s)+1);
	if (e.string == FAIL)
	    return(FAIL);	/* no more string storage */
   
	strcpy(t->stringspc + e.string, s);
	t->htable[t->lastkey] = e;
	t->used++;
    }
    else                     /* pointing to the right entry? */
    {
	if (!MATCH(t->stringspc + t->htable[t->lastkey].string, s))
	{
	    fprintf(stderr, "string insertion error in hashobj [%s vs %s]\n",
		   t->stringspc + t->htable[t->lastkey].string, s);
            return(FAIL);
	}
        else
	{
            t->htable[t->lastkey].id = id; /* change the existing entry */
	}
    }
    
    return(t->lastkey);
}

	
/* ---------------------------------------------------------------------------
free_hashobj() releases the hash object's storage.
*/

void free_hashobj(hashobj *t)
{
  if (t)
    {
      free(t->htable);
      free(t->stringspc);
      free(t);
    }
}


/* ---------------------------------------------------------------------------
Set the size of the hash table.

load_factor is the load_factor you want the hashobj to be.  It can be
in the range: 0 < load_factor <= LOAD_FACTOR.

return -1 if args are bad; -2 if can't allocate memory

*/
int set_size_hashobj(hashobj *t, double load_factor)
{
    int j;
    hashobj *newht;
    hte *src, *dest;
    int new_size;

    if (!t)
      return -1;
    if (load_factor > LOAD_FACTOR)
      load_factor = LOAD_FACTOR;
    if (load_factor <= 0)
      return -1;

    new_size = (int)(t->used / load_factor) + 1;

    newht = get_hashobj(new_size);
    if (!newht)
      return -2;
    
    free(newht->stringspc);
    newht->stringspc = t->stringspc;
    
    for (j = 0; j < t->tblsize; j++)
	{
	if (t->htable[j].string != EMPTY)
	    {
	    src = t->htable + j;
	    ho_lookup(newht, t->stringspc + src->string);
	    dest = newht->htable + newht->lastkey;
	    dest->string = src->string;
	    dest->id = src->id;
	    }       
	}
    free(t->htable);
    t->htable = newht->htable;
    t->tblsize = newht->tblsize;
    t->hashdvsr2 = newht->hashdvsr2;
    free(newht);

    return 0;
}

/* ---------------------------------------------------------------------------
This memory allocation scheme currently only grows.  It could be made more
sophisticated if delete is implemented.  The reason for doing this, rather than
calling malloc, is that malloc (on most systems) allocates a pointer for each
block; thus it isn't efficient for allocating small (e.g., 10-byte) blocks.
This allocator gets n bytes from the pool.  */

static int stralloc(hashobj *t, int n)
{
    int start = t->stringtop;
    char *p;
    
    while (t->stringtop + n >= t->stringspcsize)  /* will it fit? */
	{
	p = (char *)realloc((void *)t->stringspc, (size_t)(t->stringspcsize * 2));
	if (p == (char *)NULL)
	    {
	    fprintf(stderr,"Ran out of string space");
	    return(FAIL);
	    }
	t->stringspc = p;
	t->stringspcsize *= 2;
	}

    t->stringtop += n;                    /* yes, advance top */
    return(start);                        /* and return offset (old top) */
}

/* ---------------------------------------------------------------------------
Note that this algorithm is machine-dependent.  Big-endian machines must 
define BIG_ENDIAN at compile time in order to compile the correct version
of the function.

The second hash function is implemented as a macro, to minimize overhead.
For reference, here it is:

#define HASH2 (t->hashdvsr2 - (t->hvalue % t->hashdvsr2))

Since this is used to compute the probe, it must be at least 1.  The second
hash function should be sufficiently different from the first to avoid
"secondary clustering".  The reason I subtract the second term from the
first is simply to make hash2 large when hash1 is small.  

How it works:
	  ---------------------
	  |    |    |    |    |
	  ---------------------
	    0    1    2    3
Above is a picture of the 4 bytes of the integer named hvalue.  The
characters of the string are successively xor'd into the bytes of the
integer.  The fourth character goes into p[3], the fifth into p[0], and
so on.  Bits are scrambled by the xor operation.  After all characters
have been processed, the object is treated as an integer, of which we
then take the remainder mod tablesize.
*/

static int hash1(hashobj *t, char *s)
{
    int j;
    char *p;
    
    t->hvalue = 0;
    p = (char *)&t->hvalue;
    
#if (MACHINE_ENDIAN == 'B')       /* don't mess with this */

    for(j=0; *s; j &= 3)
	p[3-j++] ^= *s++;
#else

    for(j=0; *s; j &= 3)
	p[j++] ^= *s++;
    
#endif
/* Make sure that it's positive */
    if(t->hvalue < 0)
	t->hvalue = -(t->hvalue);

    return (t->hvalue % t->tblsize);
}

/* ---------------------------------------------------------------------------
This system requires that the size of the hash table is a prime number.  */

static int prime_lte(int n)       /* return the largest prime <= n */
{
    while(!prime(n))
	n--;
    return(n);
}   

static int prime_gte(int n)       /* return the largest prime >= n */
{
    while(!prime(n))
	n++;
    return(n);
}   

static int prime(int n)             /* return TRUE if n is prime */
{
    int j;
    double root2;
    
    root2 = sqrt((double)n);

    for(j=2; j <= root2; j++)
	if (n % j == 0)
	    return(FALSE);

    return(TRUE);
}

double ho_load_factor()
{
  return LOAD_FACTOR;
}

int ho_size(hashobj *t)
{
  if (t)
    return t->tblsize;
  else
    return -1;
}

int ho_used(hashobj *t)
{
  if (t)
    return t->used;
  else
    return -1;
}

/* -----------------------------  Declarations ------------------------------*/

typedef struct
{
  int exception;        /* is the word an exception to stemming rules? */
  char root[MAX_WORD_LENGTH]; /* used for direct lookup (e.g. irregular variants) */
} dictentry;

/* ------------------------- Function Declarations --------------------------*/


/* ------------------------------ Definitions -------------------------------*/

static char *word;

static int j;    /* INDEX of final letter in stem (within word) */
static int k;    /* INDEX of final letter in word.
		    You must add 1 to k to get the current length of word.  
		    When you want the length of word, use the macro wordlength,
		    which is #defined as (k+1).  Note that wordlength is only
		    used for its value (never assigned to), so this is ok. */

static int dict_initialized_flag = FALSE;  /* ensure we load it before using it */

static hashobj *dict_ht;    /* the hashtable used to store the dictionary */
static dictentry *dep;       /* for general use with dictionary entries    */
static dictentry *main_deps;  /* store the dictentry entries (dynamically sized) */
static int default_val;

/* -------------------------- Function Definitions --------------------------*/

/* COUNT_LINES
 *
 *   Counts the number of lines in the file file_name
 *
 * ARGUMENTS:
 *
 *   file_name -- a pointer to the name of the file to be counted
 *
 * RETURNS:
 *
 *   N         -- number of lines in the file (success)
 *   -1        -- error opening or reading the file (failure)
 */

int count_lines(char *file_name)
{
  int num = 0;
  FILE *fp;
  char temp[3*MAX_WORD_LENGTH];

  if (!file_name) 
    return(-1);
  else if (NULL == (fp = fopen((char *)file_name,"r"))) 
    return(-1);
  
  fgets(temp,3*MAX_WORD_LENGTH,fp);
  while (!feof(fp))  {
    num++;
    fgets(temp,3*MAX_WORD_LENGTH,fp);
  }
  
  fclose(fp);  
  return(num);
}  

/* Expands the directory, adding the file  */

char *add_file(char name[], char directory[], char filename[])
{
  strcpy((char *)name,(char *)directory);
  if (strlen((char *)name) && (name[strlen((char *)name)-1] != DIR_SEP_CHAR) &&
      (filename[0] != DIR_SEP_CHAR))
    strcat ((char *)name, (char *)DIR_SEP_STRING);
  strcat((char *)name, (char *)filename);
  return(name);
}


/* read_dict_info() reads the words from the dictionary and puts them into a
   hash table.  It also stores the other lexicon information
   required by the stemmer (proper noun information, supplemental
   dictionary files, direct mappings for irregular variants, etc.)
   */

void read_dict_info() 
{
  
  FILE *dict_file;                      /* main list of words in dictionary */
  FILE *exception_file;                 /* exceptions to "e" ending (suited/suit, suites/suite) */
  FILE *dict_supplement_file;           /* words not found in the basic dictionary list */
  FILE *proper_noun_file;               /* so we don't have Paris->Pari or Inverness->Inver */
  FILE *country_nationality_file;       /* Italian->Italy, French->France, etc. */
  FILE *direct_conflation_file;         /* variants that can be directly conflated */
  
  
  char *stemdir;                         /* the directory where all these files reside */
  char exc_file_name[MAX_FILENAME_LENGTH], 
       dc_file_name[MAX_FILENAME_LENGTH], 
       hw_file_name[MAX_FILENAME_LENGTH], 
       cn_file_name[MAX_FILENAME_LENGTH], 
       pn_file_name[MAX_FILENAME_LENGTH],
       ds_file_name[MAX_FILENAME_LENGTH];
  
  char word[MAX_WORD_LENGTH];
  char variant[MAX_WORD_LENGTH];
  
  int lookup_value;
  int num_deps = 0;
  int temp_num = 0;
  int dep_count;
  
  /* the hashobj package allows a number to be associated with each word stored in
     the hash table.  The lookup routine returns a value of -1 (FAIL) to indicate
     that the word was not in the lexicon, otherwise it returns the number associated
     with the word.   I cast this value to a pointer to a structure.  The structure 
     has two fields, one that tells whether the word is an exception to words ending
     in "e" (e.g., `automating'->`automate', but `doing'->`do').  The other field is 
     used for a direct conflation (e.g., irregular variants, and mapping between 
     nationalites and countries (`Italian'->`Italy')). */
  
  
  /* get the directory name from an environment variable, and then build
     the name of the file (including the path) in the *_file_name variables */
  stemdir = (char *)getenv((char *)"STEM_DIR");  
         
  if (!stemdir) 
    fprintf(stderr,"The environment variable STEM_DIR is not defined.\nIt must be set to the directory that contains files used by the stemmer.");

  if (strlen(stemdir) > 70) 
    fprintf(stderr,"The directory path in the environment variable STEM_DIR is too long. \nThe limit is 70 characters.");

  add_file((char *)exc_file_name, (char *)stemdir, 
	   (char *)"exception_words.txt");
  add_file((char *)dc_file_name,  (char *)stemdir, 
	   (char *)"direct_conflations.txt");
  add_file((char *)ds_file_name,  (char *)stemdir, 
	   (char *)"dict_supplement.txt");
  add_file((char *)hw_file_name,  (char *)stemdir, 
	   (char *)"head_word_list.txt");
  add_file((char *)cn_file_name,  (char *)stemdir, 
	   (char *)"country_nationality.txt");
  add_file((char *)pn_file_name,  (char *)stemdir, 
	   (char *)"proper_nouns.txt");

  if ((num_deps = count_lines(exc_file_name)) < 0) 
    fprintf(stderr,"Couldn't open/read file: \"%s\"",exc_file_name);
  if ((temp_num = count_lines(dc_file_name)) < 0)
    fprintf(stderr,"Couldn't open/read file: \"%s\"",dc_file_name);
  num_deps += temp_num;
  if ((temp_num = count_lines(cn_file_name)) < 0)
    fprintf(stderr,"Couldn't open/read file: \"%s\"",cn_file_name);
  num_deps += temp_num;

  dict_ht = get_hashobj(num_deps);    /* HASH_DICT_SIZE */

  main_deps = (dictentry *)malloc((num_deps+1) * sizeof (dictentry));
  dep_count = 0;
  default_val = num_deps;
  main_deps[default_val].exception = FALSE;
  main_deps[default_val].root[0] = '\0';

  exception_file = fopen(exc_file_name,  "r");
  if (!exception_file) 
    fprintf(stderr,"Couldn't open file (%s) of words that are exceptions to stemming.",exc_file_name);
  
  /* read in a list of words that are exceptions to the stemming rule I use 
     (i.e., if a word can end with an `e', it does).  So, `automating' -> `automate' 
     according to the normal application of the rule, but `doing' shouldn't become 
     `doe'.  
     */
  
  fscanf(exception_file, "%s", word);
  while (!feof(exception_file))  {
    lookup_value = ho_lookup(dict_ht, word);
    if (lookup_value != FAIL)  {
      fprintf(stderr,"%s (from the exception file) appears to be a duplicate entry (%d), skipping it...", word, lookup_value);
      continue;
    }
    main_deps[dep_count].exception = TRUE;
    main_deps[dep_count].root[0] = '\0';
    ho_insert(dict_ht, word, dep_count++);        
    fscanf(exception_file, "%s", word);
  }
  
  fclose(exception_file);
  
  
  
  /*  store words for which we have a direct conflation.  These are cases that
      would not go through due to length restrictions (`owing'->`owe'), or in which
      the user wishes to over-ride the normal operation of the stemmer */
  
  direct_conflation_file = fopen(dc_file_name, "r");
  if (!direct_conflation_file) 
    fprintf(stderr,"Couldn't open file (%s) of conflation words for the dictionary.",dc_file_name);
  
  fscanf(direct_conflation_file, "%s %s", variant, word);
  while (!feof(direct_conflation_file))  {
    lookup_value = ho_lookup(dict_ht, variant);
    if (lookup_value != FAIL)  {
      fprintf(stderr,"%s (from the conflation file) appears to be a duplicate entry (%d), skipping it...", variant, lookup_value);
      continue;
    }         
    main_deps[dep_count].exception = FALSE;
    strcpy(main_deps[dep_count].root,word);
    ho_insert(dict_ht, variant, (int)dep_count++);
    fscanf(direct_conflation_file, "%s %s", variant, word);
  }
  
  fclose(direct_conflation_file);
  
  
  
  /* store the mappings between countries and nationalities (e.g., British/Britain), 
     and morphology associated with continents (european/europe).  */
  
  country_nationality_file = fopen(cn_file_name, "r");
  if (!country_nationality_file)  
    fprintf(stderr,"Couldn't open file (%s) of variants associated with the names of countries.",cn_file_name);
  
  /* Just as with the previous direct-conflation file, we want to create a direct
     mapping from a variant to a root form.  In this case the mapping is between
     country nationalities and the country name (e.g., British->Britain).  They 
     are kept in separate files for ease of maintenance */
  
  fscanf(country_nationality_file, "%s %s", variant, word);
  while (!feof(country_nationality_file))  {
    lookup_value = ho_lookup(dict_ht, variant);
    main_deps[dep_count].exception = FALSE;
    strcpy(main_deps[dep_count].root,word);
    ho_insert(dict_ht, variant, (int)dep_count++);
    fscanf(country_nationality_file, "%s %s", variant, word);
  }
  
  fclose(country_nationality_file);
  
  
  
  /*  read in a list of words from the general dictionary and store it 
      in the table */
  
  dict_file = fopen(hw_file_name, "r");
  if (!dict_file) 
    fprintf(stderr,"Couldn't open dictionary headword file (%s).",hw_file_name);
  
  fscanf(dict_file, "%s", word);
  while (!feof(dict_file))  {
    lookup_value = ho_lookup(dict_ht, word);
    /* if the word isn't already there, insert it */
    if (lookup_value == FAIL) {                         
      ho_insert(dict_ht, word, (int)default_val);
    }
    
    fscanf(dict_file, "%s", word);
  }
  
  
  fclose(dict_file);
  
  
  /* now store words that are not found in the main dictionary */
  
  dict_supplement_file = fopen(ds_file_name, "r");
  if (!dict_supplement_file) 
    fprintf(stderr,"Couldn't open file (%s) of supplemental words to the dictionary.",ds_file_name);
  
  fscanf(dict_supplement_file, "%s", word);
  while (!feof(dict_supplement_file))  {
    lookup_value = ho_lookup(dict_ht, word);
    if (lookup_value != FAIL) {
      fprintf(stderr,"Word %s (from the supplemental dictionary) appears to have a duplicate entry (%d)...Ignoring it.\n", word, lookup_value);
    }
    else 
      ho_insert(dict_ht, word, (int)default_val);
    fscanf(dict_supplement_file, "%s", word);
  }
  
  fclose(dict_supplement_file);
  
  
  
  /* finally, store proper nouns that would otherwise be altered by
     the stemmer (e.g. `Inverness') */
  
  
  proper_noun_file = fopen(pn_file_name, "r");
  if (!proper_noun_file)  
    fprintf(stderr,"Couldn't open file (%s) of proper nouns.\n",pn_file_name);
  
  fscanf(proper_noun_file, "%s", word);
  while (!feof(proper_noun_file))  {
    lookup_value = ho_lookup(dict_ht, word);
    ho_insert(dict_ht, word, (int)default_val);
    fscanf(proper_noun_file, "%s", word);
  }
  
  fclose(proper_noun_file);
  
  
  dict_initialized_flag = TRUE;
}

/* getdep(word) returns NULL if word is not found in the dictionary,
   and gives a pointer to a dictentry if not */

static dictentry *getdep(char *word)
{
  int lookup_value;
  
  dictentry *dep;
  
  /* don't bother to check for words that are short */
  
  if (strlen((char *)word) <= 1)
    return ((dictentry *)NULL);
  else {
    lookup_value = ho_lookup(dict_ht, word);
    
    if (lookup_value == FAIL) 
      return ((dictentry *)NULL);  /* Not found */
    else
      dep = &(main_deps[lookup_value]);
  }

  if (!((dep->exception == FALSE) || (dep->exception == TRUE))) {
    fprintf(stderr,"Dictionary hash table is corrupted.  Word being looked up was %s, lookup value: %d\n", word, lookup_value);
  }
  return(dep);
}

/* lookup(word) returns FAIL if word is not found in the dictionary, 
   and > 0 otherwise */

static int lookup(char *word)
{
  if (getdep(word) == (dictentry *)NULL)
    return(FAIL);
  return(1);
}

/* cons() returns TRUE if word[i] is a consonant.  (The recursion is safe.) */

static int cons(int i)
{
  char ch;
  
  ch = word[i];
  if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u')
    return(FALSE);
  
  if (ch != 'y' || i == 0)
    return(TRUE);
  else
    return (!cons(i - 1));
}

/* This routine is useful for ensuring that we don't stem acronyms */

static int vowelinstem()
{
  int i;
  
  for (i = 0; i < stemlength; i++) 
    if (vowel(i)) return(TRUE);         /* vowel is a macro */
  return(FALSE);
}

/* return TRUE if word ends with a double consonant */

static int doublec (int i)
{
  if (i < 1)
    return(FALSE);
  
  if (word[i] != word[i - 1])
    return(FALSE);
  
  return(cons(i));
}

/* Passing the length of str is awkward, but important for performance.  Since
   str is always a string constant, we can define a macro ends_in (see the macro
   section of this module) which takes str and determines its length at compile
   time.  Note that str must therefore no longer be padded with spaces in the calls 
   to ends_in (as it was in the original version of this code).
   */

static int ends(char *str, int sufflength)
{
  int r = wordlength - sufflength;    /* length of word before this suffix */
  int match;
  
  if (sufflength > k)
    return(FALSE);
  
  match = (strcmp((char *)word+r, (char *)str) == 0);
  j = (match ? r-1 : k);             /* use r-1 since j is an index rather than length */
  return(match);
}

/* replace old suffix with str */

static void setsuff(char *str, int length)
{
  strcpy((char *)word+j+1, (char *)str);
  k = j + length;
  word[k+1] = '\0';
}

/* convert plurals to singular form, and `-ies' to `y' */

static void plural ()
{
  if (final_c == 's')  {
    if (ends_in("ies")) {
      word[j+3] = '\0';
      k--;
      if (lookup(word) != FAIL)        /* ensure calories -> calorie */
	return;
      k++;
      word[j+3] = 's';             
      setsuffix("y"); 
    }
    else 
      if (ends_in("es")) {
	/* try just removing the "s" */
	word[j+2] = '\0';
	k--;
	
	/* note: don't check for exceptions here.  So, `aides' -> `aide',
	   but `aided' -> `aid'.  The exception for double s is used to prevent
	   crosses -> crosse.  This is actually correct if crosses is a plural
	   noun (a type of racket used in lacrosse), but the verb is much more
	   common */
	
	if ((lookup(word) != FAIL)  && !((word[j] == 's') && (word[j-1] == 's')))
	  return;
	
	/* try removing the "es" */
	
	word[j+1] = '\0';
	k--;
	if (lookup(word) != FAIL)
	  return;
	
	/* the default is to retain the "e" */
	word[j+1] = 'e';
	word[j+2] = '\0';
	k++;
	return;
      }
      else {
	if (wordlength > 3 && penult_c != 's' && !ends_in("ous")) {
	  /* unless the word ends in "ous" or a double "s", remove the final "s" */
	  word[k] = '\0';
	  k--; 
	}
      }
  }   
}

/* convert past tense (-ed) to present, and `-ied' to `y' */

static void past_tense ()
{
  /* Handle words less than 5 letters with a direct mapping  
     This prevents (fled -> fl).  */
  
  if (wordlength <= 4)
    return;
  
  if (ends_in("ied"))  {
    word[j+3] = '\0';
    k--;
    if (lookup(word) != FAIL)             /* we almost always want to convert -ied to -y, but */
      return;                            /* this isn't true for short words (died->die)      */
    k++;                                  /* I don't know any long words that this applies to, */
    word[j+3] = 'd';                      /* but just in case...                              */
    setsuffix("y");
    return;
  }
  
  /* the vowelinstem() is necessary so we don't stem acronyms */
  if (ends_in("ed") && vowelinstem())  {
    /* see if the root ends in `e' */
    word[j+2] = '\0'; 
    k = j + 1;              
    
    if ((dep = getdep(word)) != (dictentry *)NULL)
      if (!(dep->exception))    /* if it's in the dictionary and not an exception */
	return;
    
    /* try removing the "ed" */
    word[j+1] = '\0';
    k = j;
    if (lookup(word) != FAIL)
      return;
    
    
    /* try removing a doubled consonant.  if the root isn't found in
       the dictionary, the default is to leave it doubled.  This will
       correctly capture `backfilled' -> `backfill' instead of
       `backfill' -> `backfille', and seems correct most of the time  */
    
    if (doublec(k))  {
      word[k] = '\0';
      k--;
      if (lookup(word) != FAIL)
	return;
      word[k+1] = word[k];
      k++;
      return; 
    }
    
    
    
    /* if we have a `un-' prefix, then leave the word alone  */
    /* (this will sometimes screw up with `under-', but we   */
    /*  will take care of that later)                        */
    
    if ((word[0] == 'u') && (word[1] == 'n'))  {
      word[k+1] = 'e';                            
      word[k+2] = 'd';                            
      k = k+2;
      return;
    }
    
    
    /* it wasn't found by just removing the `d' or the `ed', so prefer to
       end with an `e' (e.g., `microcoded' -> `microcode'). */
    
    word[j+1] = 'e';
    word[j+2] = '\0';
    k = j + 1;
    return;
  }
}

/* handle `-ing' endings */

static void aspect ()
{
  /* handle short words (aging -> age) via a direct mapping.  This
     prevents (thing -> the) in the version of this routine that
     ignores inflectional variants that are mentioned in the dictionary
     (when the root is also present) */
  
  if (wordlength <= 5)                           
    return;
  
  /* the vowelinstem() is necessary so we don't stem acronyms */
  if (ends_in("ing") && vowelinstem())  {
    
    /* try adding an `e' to the stem and check against the dictionary */
    word[j+1] = 'e';
    word[j+2] = '\0';
    k = j+1;          
    
    if ((dep = getdep(word)) != (dictentry *)NULL)
      if (!(dep->exception))    /* if it's in the dictionary and not an exception */
	return;
    
    /* adding on the `e' didn't work, so remove it */
    word[k] = '\0';
    k--;                                      /* note that `ing' has also been removed */
    
    if (lookup(word) != FAIL)
      return;
    
    /* if I can remove a doubled consonant and get a word, then do so */
    if (doublec(k))  {
      k--;
      word[k+1] = '\0';
      if (lookup(word) != FAIL)
	return;
      word[k+1] = word[k];       /* restore the doubled consonant */
      
      /* the default is to leave the consonant doubled            */
      /*  (e.g.,`fingerspelling' -> `fingerspell').  Unfortunately */
      /*  `bookselling' -> `booksell' and `mislabelling' -> `mislabell'). */
      /*  Without making the algorithm significantly more complicated, this */
      /*  is the best I can do */
      k++;
      return;
    }
    
    /* the word wasn't in the dictionary after removing the stem, and then
       checking with and without a final `e'.  The default is to add an `e'
       unless the word ends in two consonants, so `microcoding' -> `microcode'.
       The two consonants restriction wouldn't normally be necessary, but is
       needed because we don't try to deal with prefixes and compounds, and
       most of the time it is correct (e.g., footstamping -> footstamp, not
       footstampe; however, decoupled -> decoupl).  We can prevent almost all
       of the incorrect stems if we try to do some prefix analysis first */
    
    if (cons(j) && cons(j-1)) {
      k = j;
      word[k+1] = '\0';
      return;
    }
    
    word[j+1] = 'e';
    word[j+2] = '\0';
    k = j+1;
    return;
  }
}

/* handle some derivational endings */

/* this routine deals with -ion, -ition, -ation, -ization, and -ication.  The 
   -ization ending is always converted to -ize */

static void ion_endings ()
{
  int old_k = k;
  
  if (ends_in("ization"))  {   /* the -ize ending is very productive, so simply accept it as the root */
    word[j+3] = 'e';
    word[j+4] = '\0';
    k = j+3;
    return; 
  }
  
  
  if (ends_in("ition")) {     
    word[j+1] = 'e';
    word[j+2] = '\0';
    k = j+1;
    if (lookup(word) != FAIL)     /* remove -ition and add `e', and check against the dictionary */
      return;                    /* (e.g., definition->define, opposition->oppose) */
    
    /* restore original values */
    word[j+1] = 'i';
    word[j+2] = 't';
    k = old_k;
  }
  
  
  if (ends_in("ation"))  {
    word[j+3] = 'e';
    word[j+4] = '\0';
    k = j+3;         
    if (lookup(word) != FAIL)   /* remove -ion and add `e', and check against the dictionary */
      return;                  /* (elmination -> eliminate)  */
    
    
    word[j+1] = 'e';            /* remove -ation and add `e', and check against the dictionary */
    word[j+2] = '\0';           /* (allegation -> allege) */
    k = j+1;
    if (lookup(word) != FAIL)
      return;
    
    word[j+1] = '\0';           /* just remove -ation (resignation->resign) and check dictionary */
    k = j;
    if (lookup(word) != FAIL)
      return;
    
    /* restore original values */
    word[j+1] = 'a';
    word[j+2] = 't';
    word[j+3] = 'i';
    word[j+4] = 'o';            /* no need to restore word[j+5] (n); it was never changed */
    k = old_k;
  }
  
  
  /* test -ication after -ation is attempted (e.g., `complication->complicate' 
     rather than `complication->comply') */
  
  if (ends_in("ication"))  {
    word[j+1] = 'y';
    word[j+2] = '\0';
    k = j+1;
    if (lookup(word) != FAIL)  /* remove -ication and add `y', and check against the dictionary */
      return;                 /* (e.g., amplification -> amplify) */
    
    /* restore original values */
    word[j+1] = 'i';
    word[j+2] = 'c';
    k = old_k;
  }
  
  
  if (ends_in("ion")) {
    word[j+1] = 'e';
    word[j+2] = '\0';
    k = j+1;
    if (lookup(word) != FAIL)    /* remove -ion and add `e', and check against the dictionary */
      return;
    
    word[j+1] = '\0';
    k = j;
    if (lookup(word) != FAIL)    /* remove -ion, and if it's found, treat that as the root */
      return;
    
    /* restore original values */
    word[j+1] = 'i';
    word[j+2] = 'o';
    k = old_k;
  }
  
  
  return;
}

/* this routine deals with -er, -or, -ier, and -eer.  The -izer ending is always converted to
   -ize */

static void er_and_or_endings ()
{
  int old_k = k;
  
  char word_char;                 /* so we can remember if it was -er or -or */
  
  if (ends_in("izer")) {          /* -ize is very productive, so accept it as the root */
    word[j+4] = '\0';
    k = j+3;
    return;
  }
  
  if (ends_in("er") || ends_in("or")) {
    word_char = word[j+1];
    if (doublec(j)) {
      word[j] = '\0';
      k = j - 1;
      if (lookup(word) != FAIL)
	return;
      word[j] = word[j-1];       /* restore the doubled consonant */
    }
    
    
    if (word[j] == 'i') {         /* do we have a -ier ending? */
      word[j] = 'y';
      word[j+1] = '\0';
      k = j;
      if (lookup(word) != FAIL)  /* yes, so check against the dictionary */
	return;
      word[j] = 'i';             /* restore the endings */ 
      word[j+1] = 'e';
    }   
    
    
    if (word[j] == 'e') {         /* handle -eer */
      word[j] = '\0';
      k = j - 1;
      if (lookup(word) != FAIL)
	return;
      word[j] = 'e';
    }
    
    word[j+2] = '\0';            /* remove the -r ending */
    k = j+1;
    if (lookup(word) != FAIL)
      return;
    word[j+1] = '\0';            /* try removing -er/-or */
    k = j;
    if (lookup(word) != FAIL)
      return;
    word[j+1] = 'e';             /* try removing -or and adding -e */
    word[j+2] = '\0';
    k = j+1;
    if (lookup(word) != FAIL)
      return;
    
    word[j+1] = word_char;       /* restore the word to the way it was */
    word[j+2] = 'r';
    k = old_k;                  
  }
  
}

/* this routine deals with -ly endings.  The -ally ending is always converted to -al 
   Sometimes this will temporarily leave us with a non-word (e.g., heuristically
   maps to heuristical), but then the -al is removed in the next step.  */

static void ly_endings ()
{
  int old_k = k;
  
  if (ends_in("ly")) {
    word[j+2] = 'e';             /* try converting -ly to -le */
    if (lookup(word) != FAIL)       
      return;
    word[j+2] = 'y';
    
    word[j+1] = '\0';            /* try just removing the -ly */
    k = j;
    if (lookup(word) != FAIL)
      return;
    if ((word[j-1] == 'a') && (word[j] == 'l'))    /* always convert -ally to -al */
      return;
    word[j+1] = 'l';
    k = old_k;
    
    if ((word[j-1] == 'a') && (word[j] == 'b')) {  /* always convert -ably to -able */
      word[j+2] = 'e';
      k = j+2;
      return;
    }
    
    if (word[j] == 'i') {        /* e.g., militarily -> military */
      word[j] = 'y';
      word[j+1] = '\0';
      k = j;
      if (lookup(word) != FAIL)
	return;
      word[j] = 'i';
      word[j+1] = 'l';
      k = old_k;
    }
    
    word[j+1] = '\0';           /* the default is to remove -ly */
    k = j;
  }
  return;
}

/* this routine deals with -al endings.  Some of the endings from the previous routine
   are finished up here.  */

static void al_endings() 
{
  int old_k = k;
  
  if (ends_in("al"))  {
    word[j+1] = '\0';
    k = j;
    if (lookup(word) != FAIL)     /* try just removing the -al */
      return;
    
    if (doublec(j))  {            /* allow for a doubled consonant */
      word[j] = '\0';
      k = j-1;
      if (lookup(word) != FAIL)
	return;
      word[j] = word[j-1];
    }
    
    word[j+1] = 'e';              /* try removing the -al and adding -e */
    word[j+2] = '\0';
    k = j+1;
    if (lookup(word) != FAIL)
      return;
    
    word[j+1] = 'u';              /* try converting -al to -um */
    word[j+2] = 'm';              /* (e.g., optimal - > optimum ) */
    k = j+2;
    if (lookup(word) != FAIL)
      return;
    
    word[j+1] = 'a';              /* restore the ending to the way it was */
    word[j+2] = 'l';
    word[j+3] = '\0';
    k = old_k;
    
    if ((word[j-1] == 'i') && (word[j] == 'c'))  {
      word[j-1] = '\0';          /* try removing -ical  */
      k = j-2;
      if (lookup(word) != FAIL)
	return;
      
      word[j-1] = 'y';           /* try turning -ical to -y (e.g., bibliographical) */
      word[j] = '\0';
      k = j-1;
      if (lookup(word) != FAIL)
	return;
      
      word[j-1] = 'i';
      word[j] = 'c';
      word[j+1] = '\0';          /* the default is to convert -ical to -ic */
      k = j;
      return;
    }
    
    if (word[j] == 'i') {        /* sometimes -ial endings should be removed */
      word[j] = '\0';           /* (sometimes it gets turned into -y, but we */
      k = j-1;                  /* aren't dealing with that case for now) */
      if (lookup(word) != FAIL)
	return;
      word[j] = 'i';
      k = old_k;
    }
    
  }
  return;
}

/* this routine deals with -ive endings.  It normalizes some of the
   -ative endings directly, and also maps some -ive endings to -ion. */

static void ive_endings() 
{
  int old_k = k;
  
  if (ends_in("ive"))  {
    word[j+1] = '\0';          /* try removing -ive entirely */
    k = j;
    if (lookup(word) != FAIL)
      return;
    
    word[j+1] = 'e';           /* try removing -ive and adding -e */
    word[j+2] = '\0';
    k = j+1;
    if (lookup(word) != FAIL)
      return;
    word[j+1] = 'i';
    word[j+2] = 'v';
    
    if ((word[j-1] == 'a') && (word[j] == 't'))  {
      word[j-1] = 'e';       /* try removing -ative and adding -e */
      word[j] = '\0';        /* (e.g., determinative -> determine) */
      k = j-1;
      if (lookup(word) != FAIL)
	return;
      word[j-1] = '\0';     /* try just removing -ative */
      if (lookup(word) != FAIL)
	return;
      word[j-1] = 'a';
      word[j] = 't';
      k = old_k;
    }
    
    /* try mapping -ive to -ion (e.g., injunctive/injunction) */
    word[j+2] = 'o';
    word[j+3] = 'n';
    if (lookup(word) != FAIL)
      return;
    
    word[j+2] = 'v';       /* restore the original values */
    word[j+3] = 'e';
    k = old_k;
  }
  return;
}

/* this routine deals with -ize endings. */

static void ize_endings() 
{
  int old_k = k;
  
  if (ends_in("ize"))  {
    word[j+1] = '\0';       /* try removing -ize entirely */
    k = j;
    if (lookup(word) != FAIL)
      return;
    word[j+1] = 'i';
    
    if (doublec(j))  {      /* allow for a doubled consonant */
      word[j] = '\0';
      k = j-1;
      if (lookup(word) != FAIL)
	return;
      word[j] = word[j-1];
    }
    
    word[j+1] = 'e';        /* try removing -ize and adding -e */
    word[j+2] = '\0';
    k = j+1;
    if (lookup(word) != FAIL)
      return;
    word[j+1] = 'i';
    word[j+2] = 'z';
    k = old_k;
  }
  return;
}

/* this routine deals with -ment endings. */

static void ment_endings() 
{
  int old_k = k;
  
  if (ends_in("ment"))  {
    word[j+1] = '\0';
    k = j;
    if (lookup(word) != FAIL)
      return;
    word[j+1] = 'm';
    k = old_k;
  }
  return;
}

/* this routine deals with -ity endings.  It accepts -ability, -ibility,
   and -ality, even without checking the dictionary because they are so 
   productive.  The first two are mapped to -ble, and the -ity is remove
   for the latter */

static void ity_endings() 
{
  int old_k = k;
  
  if (ends_in("ity"))  {
    word[j+1] = '\0';             /* try just removing -ity */
    k = j;
    if (lookup(word) != FAIL)
      return;
    word[j+1] = 'e';              /* try removing -ity and adding -e */
    word[j+2] = '\0';
    k = j+1;
    if (lookup(word) != FAIL)
      return;
    word[j+1] = 'i';
    word[j+2] = 't';
    k = old_k;
    
    /* the -ability and -ibility endings are highly productive, so just accept them */
    if ((word[j-1] == 'i') && (word[j] == 'l'))  {   
      word[j-1] = 'l';          /* convert to -ble */
      word[j] = 'e';
      word[j+1] = '\0';
      k = j;
      return;
    }
    
    
    /* ditto for -ivity */
    if ((word[j-1] == 'i') && (word[j] == 'v'))  {
      word[j+1] = 'e';         /* convert to -ive */
      word[j+2] = '\0';
      k = j+1;
      return;
    }
    
    /* ditto for -ality */
    if ((word[j-1] == 'a') && (word[j] == 'l'))  {
      word[j+1] = '\0';
      k = j;
      return;
    }
    
    /* if the root isn't in the dictionary, and the variant *is*
       there, then use the variant.  This allows `immunity'->`immune',
       but prevents `capacity'->`capac'.  If neither the variant nor
       the root form are in the dictionary, then remove the ending
       as a default */
    
    if (lookup(word) != FAIL)   
      return;
    
    /* the default is to remove -ity altogether */
    word[j+1] = '\0';
    k = j;
    return;
  }
}

/* handle -able and -ible */

static void ble_endings() 
{
  int old_k = k;
  char word_char;
  
  if (ends_in("ble"))  {
    if (!((word[j] == 'a') || (word[j] == 'i')))
      return;
    word_char = word[j];
    word[j] = '\0';             /* try just removing the ending */
    k = j-1;
    if (lookup(word) != FAIL) 
      return;
    if (doublec(k))  {          /* allow for a doubled consonant */
      word[k] = '\0';
      k--;
      if (lookup(word) != FAIL)
	return;
      k++;
      word[k] = word[k-1];
    }
    word[j] = 'e';              /* try removing -a/ible and adding -e */
    word[j+1] = '\0';
    k = j;
    if (lookup(word) != FAIL)
      return;
    
    word[j] = 'a';              /* try removing -able and adding -ate */
    word[j+1] = 't';            /* (e.g., compensable/compensate)     */
    word[j+2] = 'e';
    word[j+3] = '\0';
    k = j+2;
    if (lookup(word) != FAIL)
      return;
    
    word[j] = word_char;        /* restore the original values */
    word[j+1] = 'b';
    word[j+2] = 'l';
    word[j+3] = 'e';
    k = old_k;
  }
  return;
}

/* handle -ness */

static void ness_endings() 
{
  if (ends_in("ness"))  {     /* this is a very productive endings, so just accept it */
    word[j+1] = '\0';
    k = j;
    if (word[j] == 'i')  
      word[j] = 'y';
  }
  return;
}

/* handle -ism */

static void ism_endings()
{
  if (ends_in("ism"))  {    /* this is a very productive ending, so just accept it */
    word[j+1] = '\0';
    k = j;
  }
  return;
}

/* handle -ic endings.   This is fairly straightforward, but this is
   also the only place we try *expanding* an ending, -ic -> -ical.
   This is to handle cases like `canonic' -> `canonical' */

static void ic_endings()
{
  if (ends_in("ic")) {
    word[j+3] = 'a';        /* try converting -ic to -ical */
    word[j+4] = 'l';
    word[j+5] = '\0';
    k = j+4;
    if (lookup(word) != FAIL)
      return;
    
    word[j+1] = 'y';        /* try converting -ic to -y */
    word[j+2] = '\0';
    k = j+1;
    if (lookup(word) != FAIL)
      return;
    
    word[j+1] = 'e';        /* try converting -ic to -e */
    if (lookup(word) != FAIL)
      return;
    
    word[j+1] = '\0';       /* try removing -ic altogether */
    k = j;
    if (lookup(word) != FAIL)
      return;
    
    word[j+1] = 'i';        /* restore the original ending */
    word[j+2] = 'c';
    word[j+3] = '\0';
    k = j+2;
  }
  return;
}

/* handle -ency and -ancy */

static void ncy_endings()
{
  if (ends_in("ncy"))  {
    if (!((word[j] == 'e') || (word[j] == 'a')))
      return;
    word[j+2] = 't';          /* try converting -ncy to -nt */
    word[j+3] = '\0';         /* (e.g., constituency -> constituent) */
    k = j+2;
    
    if (lookup(word) != FAIL)
      return;
    
    word[j+2] = 'c';          /* the default is to convert it to -nce */
    word[j+3] = 'e';
    k = j+3;
  }
  return;
}

/* handle -ence and -ance */

static void nce_endings()
{
  int old_k = k;
  
  char word_char;
  
  if (ends_in("nce"))  {
    if (!((word[j] == 'e') || (word[j] == 'a')))
      return;
    word_char = word[j];
    word[j] = 'e';        /* try converting -e/ance to -e (adherance/adhere) */
    word[j+1] = '\0';
    k = j;
    if (lookup(word) != FAIL)
      return;
    word[j] = '\0';       /* try removing -e/ance altogether (disappearance/disappear) */
    k = j-1;
    if (lookup(word) != FAIL)
      return;
    word[j] = word_char;  /* restore the original ending */
    word[j+1] = 'n';
    k = old_k;
  }
  return;
}




char * kstem_stem(char *term)
{
  int i;
  int stem_it = TRUE;
  int hval;
  static char stem[MAX_WORD_LENGTH];

  if (!dict_initialized_flag) 
    read_dict_info();
  
  if (!stemht_init_flag) 
    stemht_init();
  
  k = strlen((char *)term) - 1;
  /* if the word is too long or too short, or not entirely
     alphabetic, just lowercase copy it into stem and return */
  if ((k <= 2-1) || (k >= MAX_WORD_LENGTH-1))
    stem_it = FALSE;
  else
    for (i=0; i<=k; i++)
      if (!isalpha(term[i])) {
	stem_it = FALSE;
	break;
      }
  if (!stem_it) {
    for (i=0; i<=k; i++)
      term[i] = tolower(term[i]);
    term[k+1] = '\0';
    return term;
  }
  /* Check to see if it's in the cache. */
  /* If it's found, mark the slot in which it is found */
  /* Note that there is no need to lowercase the term in this case */
  stemhash(term, hval);
  
  if (strcmp(term, stemht[hval].word1) == 0) 
    {
      strcpy(stem, stemht[hval].stem1);
      stemht[hval].flag = 1; 
      return stem;
    }
  else if (strcmp(term, stemht[hval].word2) == 0) 
    {
      strcpy(stem, stemht[hval].stem2);
      stemht[hval].flag = 2; 
      return stem;
    }
  /* 'word' is a pointer, global to this file, for manipulating the word in
     the buffer provided through the passed in pointer 'stem'. */
  word = stem;

  for (i=0; i<=k; i++)           /* lowercase the local copy */
    word[i] = tolower(term[i]);
  word[k+1] = '\0';

  /* the basic algorithm is to check the dictionary, and leave the word as
     it is if the word is found. Otherwise, recognize plurals, tense, etc.
     and normalize according to the rules for those affixes.  Check against
     the dictionary after each stage, so `longings' -> `longing' rather than
     `long'. Finally, deal with some derivational endings.  The -ion, -er, 
     and -ly endings must be checked before -ize.  The -ity ending must come
     before -al, and -ness must come before -ly and -ive.  Finally, -ncy must
     come before -nce (because -ncy is converted to -nce for some instances).
   */
  
  /* This while loop will never repeat; it is only here to allow the
     break statement to be used to escape as soon as a word is recognized.
     */
  while (1)
    {
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      plural();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      past_tense();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      aspect();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ity_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ness_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ion_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      er_and_or_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ly_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      al_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ive_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ize_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ment_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ble_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ism_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ic_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      ncy_endings();
      if ((dep = getdep(word)) != (dictentry *)NULL) break;
      nce_endings();
      dep = getdep(word);
      break;
    }
  
  /* try for a direct mapping (allows for cases like `Italian'->`Italy' and
     `Italians'->`Italy')
   */
  if (dep != (dictentry *)NULL && dep->root[0] != '\0')  {                 
    strcpy((char *)stem, (char *)dep->root);   
  }
  /* Enter into cache, at the place not used by the last cache hit */
  if (stemht[hval].flag == 2) 
    {
      strcpy(stemht[hval].word1, term);
      strcpy(stemht[hval].stem1, stem);
      stemht[hval].flag = 1; 
    }
  else 
    {
      strcpy(stemht[hval].word2, term);
      strcpy(stemht[hval].stem2, stem);
      stemht[hval].flag = 2;
    }
  return stem;
}

static void stemht_init() 
{
    int i;

    stemht_init_flag = 1;
    stemhtsize = 30013;
    stemht = (kstem_HSLOT *)malloc(stemhtsize * sizeof(kstem_HSLOT));
    if (!stemht)
      fprintf(stderr, "Failed to allocate kstem hash table");

    for (i = 0; i < stemhtsize; i++) 
    {
      /* Set things up so that the first slot is used first */
	stemht[i].flag = 2;
	stemht[i].word1[0] = stemht[i].word2[0] = '\0';
	stemht[i].stem1[0] = stemht[i].stem2[0] = '\0';
    }
}

void kstem_release_memory()
{
dict_initialized_flag = FALSE;
free_hashobj(dict_ht);
free(main_deps);

stemht_init_flag = FALSE;
free(stemht);
}

////////////////////////// YARI-specific below /////////////////////////

void lock (int *x) ; // link with synq.c
void unlock (int *x) ;
void kstem_stemmer (char *word, char *stem) { // thread-safe (locks)
  static int KSTEM = 0;
  lock (&KSTEM);
  char *tmp = kstem_stem(word);
  strcpy(stem,tmp);
  unlock (&KSTEM);
}

#ifdef MAIN

int main (int argc, char *argv[]) {
  char line[9999];
  (void) argc;
  (void) argv;
  
  while (fgets (line,9999,stdin)) {
    
    if (*line == '#') continue; // skip comments
    char *nl = strchr (line,'\n'), *toks = line;
    if (nl) *nl = '\0'; // strip newline
    
    while (word = next_token(&toks," \t")) {
      char *stem = kstem_stem (word);
      printf(" %s", stem);
    }
    printf ("\n");
  }
  
  return(0);
}

#endif
