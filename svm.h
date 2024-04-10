/*

  Copyright (c) 1997-2024 Victor Lavrenko (v.lavrenko@gmail.com)

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
