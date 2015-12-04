/*

   Copyright (C) 2014 AENALYTICS LLC

   All rights reserved. 

   THIS SOFTWARE IS PROVIDED BY AENALYTICS LLC AND OTHER CONTRIBUTORS
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

typedef unsigned uint;
typedef unsigned long ulong;

//#define off64_t off_t;

typedef struct { uint i; float x; } ix_t;

typedef struct { uint i; uint t; } it_t;

typedef struct { uint j; uint i; float x; } jix_t;

typedef struct { float x; float y; } xy_t;

typedef struct { uint i; float x; float y; } ixy_t;

#endif
