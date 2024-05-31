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

void delta_encode (uint *V) ; // delta encoding
void delta_decode (uint *V) ;

uint *mtf_encode (uint *U) ; // move-to-front
uint *mtf_decode (uint *U) ;

byte *vbyte_encode (uint *U) ; // V-Byte | VarInt
uint *vbyte_decode (byte *B) ;

byte *msint_encode (uint *U) ;
uint *msint_decode (byte *B) ;

byte *nibbl_encode (uint *U) ; // MySQL-like nibble encoding
uint *nibbl_decode (byte *_B) ;

byte *gamma_encode (uint *U) ; // Elias Gamma
uint *gamma_decode (byte *_B) ;

byte *bmask_encode (uint *U) ; // Bit-mask encode
uint *bmask_decode (byte *B) ;

byte *zstd_encodeU (uint *U) ; // zstdlib
uint *zstd_decodeU (byte *B) ;

char *str2vec(char *str) ;
byte *zstd_vec (void *vec, int level) ; // vec <-> byte-vec
void *unzstd_vec (byte *zst, uint el_size) ;

// compress src[:ssz] into re-allocated trg[:sz]
void zstd (char **trg, size_t *sz, char *src, size_t ssz, int level) ;
// uncompress src[:ssz] into re-allocated trg[:sz]
void unzstd (char **trg, size_t *sz, char *src, size_t ssz) ;
