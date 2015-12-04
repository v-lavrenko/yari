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

#include "hash.h"

#ifndef YSVM
#define YSVM

ix_t *svm_weights (ix_t *target, coll_t *K) ;
void svm_train_1v1 (coll_t *W, coll_t *C, coll_t *K) ;
void svm_train_1vR (coll_t *W, coll_t *C, coll_t *K) ;
void svm_train (char *_W, char *_C, char *_K) ;
ix_t *svm_1v1 (ix_t *Y) ;
void svm_classify (char *_Y, char *_W, char *_K) ;

ix_t *cdescent (ix_t *_Y, coll_t *XT, ix_t *W, char *prm) ;
void cd_train_1vR (char *_W, char *_C, char *_T, char *prm) ;


#endif
