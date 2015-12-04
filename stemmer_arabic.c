/*

  Copyright (c) 1997-2006 University of Massachusetts. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    3. The names "CIIR", "UMass" and "University of Massachusetts"
       must not be used to endorse or promote products derived from
       this software without specific prior written permission.
       To obtain permission, contact info@ciir.cs.umass.edu

  THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY OF MASSACHUSETTS AND OTHER
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
  THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "hash.h"
#include "tokenize.h"

void arabic_remove_diacritics (char *, char *); //removes diacritics from word
void arabic_stop(char *, char *);  // only removes stops
void arabic_no_stem(char *, char *) ;	   // doesn't do anything
void arabic_norm2(char *, char *); //normalize arabic word
void arabic_norm2_stop(char *, char *);
void arabic_light10(char *, char *);   
void arabic_light10_stop(char *, char *);   
void arabic_clean_up (void); //use this to clean up hash files created to hold the words list
void arabic_show_stemmer_options() ;
void *arabic_set_stemmer(char *) ;
char *arabic_stem(char *phrase, int *numtoks, void (*stemmer)(char*,char*)) ;

typedef struct { /******* set_stemmer support **********/
     char *option;
     char *description;
     void (*stem_fct)(char *, char *) ;
} arabic_stem_info_t;

#define NUMSTEMMERS 6
#define ARABIC_BREAKS  " \t\r\n~`!@#$%^&*()_-+=[]{}|\\:;\"'<>,.?/\xA1\xF7\xD7\xBA\xBF"

#define STEM_TO_WORD 99
#define STEM_WORD 0

#define WAW "0xe6"
#define CHAR_WAW 0xe6
char *defarticles[] = {"«·", "Ê«·","»«·", "ﬂ«·", "›«·", "··", "\0"};
char *suffixes[] = {"Â«","«‰","« ","Ê‰","Ì‰","ÌÂ","Ì…","Â","…","Ì","\0"};  

void light_stem (char *, char *);

/**** Set stemmer support *******/
/* Three entries are option name, description, function */
/*  Option name is the string you use in "-stem optionname" */
arabic_stem_info_t stemtable[NUMSTEMMERS] = {
 {"none", "none", arabic_no_stem},
 {"arabic_stop", "arabic_stop", arabic_stop},
 {"arabic_norm2", "table normalization", arabic_norm2},
 {"arabic_norm2_stop", "table normalization with stopping", arabic_norm2_stop},
 {"arabic_light10", "light stemming", arabic_light10}, 
 {"arabic_light10_stop", "light10 and remove stop words", arabic_light10_stop}
 } ;

/**** stem funcs ***********/

int remove_diacritics (char *, char *);
void remove_definite_articles(char *, char *);

char *substring (const char *, int , int);

/************* globals ******************************/
int files_loaded = 0;


/* isWhitespace is the same
java aproximation of 
Character.isWhitespace();
*/
const int isWhitespace[256] = {
0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,
1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* Classifies arabic characters for normalization
   Combines removing diacritics, punctuation,
   nonletters, and substitutions  */

const int NormChar[256] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0x81,0,0,0,0,0,0,0,0,0x8a,0,0,0x8d,0x8e,0x8f,
0x90,0,0,0,0,0,0,0,0x98,0,0x9a,0,0,0,0,0x9f,
0,0,0,0,0,0,0,0,0,0,0xaa,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0xc0,0xc1,0xc7,0xc7,0xc4,0xc7,0xc6,0xc7,0xc8,0xe5,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,   0,0xd8,0xd9,0xda,0xdb,   0,0xdd,0xde,0xdf,
   0,0xe1,   0,0xe3,0xe4,0xe5,0xe6,   0,   0,   0,   0,   0,0xed, 0xed,   0,   0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

const int Norm3Char[256] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0x81,0,0,0,0,0,0,0,0,0x8a,0,0,0x8d,0x8e,0x8f,
0x90,0,0,0,0,0,0,0,0x98,0,0x9a,0,0,0,0,0x9f,
0,0,0,0,0,0,0,0,0,0,0xaa,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0xc0,0xc7,0xc7,0xc7,0xc7,0xc7,0xc7,0xc7,0xc8,0xe5,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,   0,0xd8,0xd9,0xda,0xdb,   0,0xdd,0xde,0xdf,
   0,0xe1,   0,0xe3,0xe4,0xe5,0xe6,   0,   0,   0,   0,   0,0xed, 0xed,   0,   0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

const int ArabicVowel[256] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,   0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0xe6,0,0,0,0,0,0xec,0xed,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


/******************** FILE DATA **************************************/
hash_t *stop_words_ht = NULL;

void freeWordSets() {
  hfree(stop_words_ht);
}

/* String function */

void substring_copy (char dest[], const char *word, int start, int end) {
	
	char *p;
	int len;
	len = strlen (word);
	if (start < len && end <= len && (end-start) > 0) {
		p = (char *) ((int)word + (int)start);
		strncpy (dest, p, end - start);
		dest[end-start] = '\0';
	} else {
		strcpy (dest, "");
	}
}

char *substring (const char *word, int start, int end) {
	static char dest[100];
	substring_copy (dest, word, start, end);
	return dest;
}



int Str_equals (const char *s1, const char *s2) {
	if (strcmp (s1, s2) == 0) return 1;
	else return 0;
}
/**************************************************************************/
/**********************  STEMMING FUNCTIONS *******************************/
/**************************************************************************/

int is_whitespace (const char c) {
	int character;
	character = c;
	if (character < 0) character = character + 256;
	return isWhitespace[character];
}

/***************************************************************************/
/* LOAD STATIC FILES *******************************************************/
void load_static_files (const char *path) {
	char filename[150];
	strcpy (filename, path);
	strcat (filename, "/stopwords.txt");
	stop_words_ht = load_stopwords(filename);
}

/***************************************************************************/
/* CHECK STEMMER FILES ******/
/* Load files if not yet loaded ********************************************/
void check_stemmer_files () {
   char *path;
   if (!files_loaded) {
      path = getenv ("ARABIC_STEM_DIR");
      if (path == NULL) {
	 load_static_files("StemmerFiles");
	 files_loaded = 1;
      } else {
	load_static_files(path);
	files_loaded = 1;
      }
   }
}

/************************************************************
 *  REMOVE_DEFINITE_ARTICLES
 *  corrected version of REMOVE_DEFINITE_ARTICLE
 *     should be done after alef's are normalized
 ************************************************************/
void remove_definite_articles(char *word, char *result) {
  size_t len;
  size_t wordlen=strlen(word);
  char **nextart=defarticles ;
  strcpy(result,word);

  for ( ; (len=strlen(*nextart)) > 0 ; nextart++) {
    if (wordlen > len+1 && strncmp(word, *nextart, len)==0) {
      strcpy(result, word+len);
      break ;  // only want to find one
    }
  }
}

/************************************************************
 *  REMOVE_suffixes
 *     should be done after alef's are normalized and prefixes removed
 *    this removes one suffix
 *    REMOVE_ALL_SUFFIXES removes all suffixes, order is significant
 ************************************************************/

void remove_all_suffixes(char *word, char *result, 
			 size_t lenlimit) /* min size for remainder */ {
  size_t suflen;
  size_t wordlen=strlen(word);
  char **nextsuf=suffixes ;

  if (wordlen==0) {
    result[0] = '\0';
    return;
  }
  strcpy (result, word);

  for ( ; (suflen=strlen(*nextsuf)) > 0 ; nextsuf++) {
    wordlen=strlen(result);
    if (wordlen > suflen+lenlimit &&
	strncmp(word+wordlen-suflen, *nextsuf, suflen) == 0) {
      result[wordlen-suflen]='\0' ;
    }
  }
}

//**************************************************************
// doesn't do anything any more

void arabic_clean_up (void) {
  return;	
}


//*******************************************************************


// Maybe add a table driven version of this
// void arabic_remove_diacritics (char *word, char *result) {
//	remove_diacritics (word, result);
//}

void arabic_no_stem(char *word, char *result) {
   strcpy(result, word);
}

/* on_stop_list.  If the current word is a stopword then return true. */
int on_stop_list (char *word) {
   if (!files_loaded) 
      check_stemmer_files();

   if (key2i(stop_words_ht,word))
      return 1;
   else	return 0;

}

void arabic_stop (char *word, char *result) {
   if (on_stop_list(word))
       result[0]='\0';
   else strcpy(result, word);
}


/****************************************************************************/

// Version of normalization that uses NormChar table
void arabic_norm2(char *word, char *result) {
   unsigned char *i=(unsigned char *)word ;
   unsigned char *o=(unsigned char *)result ;
   while (*i) {
     //if (*i > 256) *i &= 0xff ;
     if (NormChar[*i])
       *o++ = NormChar[*i] ;
     i++;
   }
   *o='\0';
}
void arabic_norm2_stop (char *word, char *result) {
   arabic_norm2(word, result);
   if (on_stop_list(result))
      *result='\0' ;
}

void arabic_light10 (char *word, char *result) {
   unsigned char *tempstring=(unsigned char *)strdup(word) ;
   unsigned char *tempstring2=(unsigned char *)strdup(word);

   char *begin ;
   *tempstring2='\0';
   arabic_norm2 (word, (char *)tempstring) ;

   // Should remove stop words here

   begin=(char *)tempstring;

   // Remove waw (advance pointer) if remainder >= 3 chars
   if (*tempstring == CHAR_WAW && strlen((char *)tempstring) > 3) begin++ ;

   // Remove definite article if remainder >=2 chars
   remove_definite_articles(begin, (char *)tempstring2);

   // Remove suffixes from small suffix list
      if (strlen((char *)tempstring2) > 2)
          remove_all_suffixes((char *)tempstring2, result, (size_t)1);  
      else 
     strcpy(result, (char *)tempstring2);
   free (tempstring) ;
   free (tempstring2) ;
}
void arabic_light10_stop (char *word, char *result) {
   unsigned char *tempstring=(unsigned char *)strdup(word) ;
   unsigned char *tempstring2=(unsigned char *)strdup(word);

   char *begin ;
   *tempstring2='\0';
   arabic_norm2 (word, (char *)tempstring) ;
   
   if (!files_loaded)
      check_stemmer_files();
   if (on_stop_list((char *)tempstring)) {
      result[0]='\0';
      free (tempstring) ;
      free (tempstring2) ;
      return;
   }
   // Not a stop word   
   begin=(char *)tempstring;
   // Remove waw (advance pointer) if remainder >= 3 chars
   if (*tempstring == CHAR_WAW && strlen((char *)tempstring) > 3) begin++ ;

   // Remove definite article if remainder >=2 chars
   remove_definite_articles(begin, (char *)tempstring2);

   // Remove suffixes from small suffix list
      if (strlen((char *)tempstring2) > 2)
          remove_all_suffixes((char *)tempstring2, result, (size_t)1);  
      else 
     strcpy(result, (char *)tempstring2);
   free (tempstring) ;
   free (tempstring2) ;
}



void arabic_show_stemmer_options() {
   int i;
   fprintf(stderr, "           Arabic stemmer names for -stem option:\n");
   for (i=0 ; i < NUMSTEMMERS; i++)
     fprintf(stderr, "               %-20s : %s\n", 
	     stemtable[i].option, stemtable[i].description);
   fprintf(stderr, "          ***Requires $ARABIC_STEM_DIR to be set\n");
   fprintf(stderr, "          Arabic text must be in windows encoding\n");
}
void *arabic_set_stemmer(char *stemval) {
    int i ;
    if (!getenv ("ARABIC_STEM_DIR")) {
  	fprintf (stderr, 
    "*** $ARABIC_STEM_DIR environment variable must be set\n");
    exit (1);
    }
    for (i=0 ; i < NUMSTEMMERS; i++) {
      if (strcmp(stemval, stemtable[i].option)==0)
	return((void *)stemtable[i].stem_fct);
    }
    /*  if you get here, stem val didn't match a legal stemmer */ 
    fprintf(stderr, "%s is not a supported arabic stemmer\n", stemval);
    arabic_show_stemmer_options() ;
    exit (1) ;
}


// STEM PHRASE breaks up a phrase into tokens, stems the tokens,
//  puts them back together into result string, and also indicates
// The number of tokens in the phrase
// the stemmed phrase should be freed later
char *arabic_stem(char *phrase, int *numtoks, void (*stemmer)(char*,char*)) {
  char *tok, *word=strdup(phrase);
  int tokencount=0;
  char *result=(char *)calloc(strlen(word)+20,1);
  char *phrasestem=result ;
  //  assert(result);
  phrasestem[0]='\0';

    // might be a phrase, so split it even if stemmer is null
    for (tok=strtok(word, ARABIC_BREAKS) ; tok ;
         tok=strtok(NULL, ARABIC_BREAKS)) {
      int len;
      if (!stemmer) strcpy(phrasestem, tok) ;
      else stemmer(tok,phrasestem) ;
      if ((len=strlen(phrasestem)) > 0) {
        phrasestem[len] = ' ';
        phrasestem += len+1;
        tokencount++;
      }
    }
  if (tokencount > 0)
    phrasestem[-1] = '\0';  // Remove trailing space
  else *phrasestem = '\0';
  free (word);
  *numtoks=tokencount ;
  return result ;
}

void arabic_stemmer (char *word, char *stem) {
  int numtoks = 0;
  char *result = arabic_stem (word, &numtoks, arabic_light10);
  strcpy (stem, result);
  free(result);
}
