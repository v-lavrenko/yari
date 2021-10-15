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

#ifndef YARITYPES
#define YARITYPES

//#define MAX(a,b,c) ((a) >= (b) ? ((a) >= (c) ? (a) : (c)) : ((b) >= (c) ? (b) : (c)))
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ABS(x)   ((x) >= 0 ? (x) : -(x))
#define SGN(x)   ((x) >= 0 ? +1 : -1)
#define SWAP(a,b) (tmp=(a), (a)=(b), (b)=tmp)
#endif

#define Infinity   9999999999999
#define	EXP	   2.7182818284590
#define PI	   3.1415926535898
#define EulerGamma 0.5772156649015
#define EPS        2.2204e-16
#define EMAXID     (1./(1.+EXP))

typedef unsigned uint;
typedef unsigned long ulong;
typedef unsigned char uchar;

//#define off64_t off_t;

typedef struct { uint i; float x; } ix_t;

typedef struct { uint i; uint t; } it_t;

typedef struct { float x; float y; } xy_t;

typedef struct { uint j; uint i; float x; } jix_t;

typedef struct { uint i; float x; float y; } ixy_t;

typedef struct { uint i; uint j; uint k; } ijk_t;

#endif
