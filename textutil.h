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

#ifndef TEXTUTIL
#define TEXTUTIL

// in str replace any occurence of chars from what[] with 'with'
void csub (char *str, char *what, char with) ; 

void chop (char *str, char *blanks) ; // remove head/tail blanks

char *substr (char *str, uint n) ; // str[0:n) in a static buffer

void squeeze (char *str, char *what) ;
void cgrams (char *str, uint lo, uint hi, uint step, char *buf, uint eob) ; // character n-grams

// erase (overwrite with 'C') every occurence of A...B
void erase_between (char *buf, char *A, char *B, int C) ;

// extract the text between A and B
char *extract_between (char *buf, char *A, char *B) ;

char *next_token (char **text, char *ws) ;

int read_doc (FILE *in, char *buf, int n, char *beg, char *end) ;

void porter_stemmer (char *word, char *stem) ;
void kstem_stemmer (char *word, char *stem) ;
void arabic_stemmer (char *word, char *stem) ;
void lowercase_stemmer (char *word, char *stem) ;
void stem_word (char *word, char *stem, char *type) ;
int stop_word (char *word) ;

char **str2toks (char *str, char *ws, uint maxlen) ;
void stem_toks (char **toks, char *type) ;
void stop_toks (char **toks) ;
ix_t *toks2ids (char **toks, hash_t *ids) ;
char **toks2pairs (char **toks, char *prm) ;
void free_toks (char **toks) ;

char *get_xml_docid (char *xml) ;

#endif
