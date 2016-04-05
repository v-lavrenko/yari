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

#ifndef MATRIX
#define MATRIX

//typedef struct { uint r; uint c; float v; } rcv_t;

//typedef struct { uint c; float v; } cv_t;

//typedef struct {char*path; coll_t*rows; coll_t*cols; hash_t*rids; hash_t*cids;} mtx_t;

typedef struct {
  uint   ndocs;
  uint   nwords;
  double nposts;
  uint  *df; // document frequency = non-zeros in each column
  float *cf; // collection frequency = sum of each column
  float *s2; // sum of squares in each column
  float  k; // free parameter
  float  b; // free parameter
} stats_t;

int cmp_jix (const void *n1, const void *n2) ; // by increasing j then i
int cmp_ix_i (const void *n1, const void *n2) ; // by increasing id
int cmp_ix_x (const void *n1, const void *n2) ; // by increasing value
int cmp_ix_X (const void *n1, const void *n2) ; // by decreasing value
int cmp_ixy_i (const void *n1, const void *n2) ; // by increasing id
int cmp_ixy_x (const void *n1, const void *n2) ; // by increasing x
int cmp_ixy_y (const void *n1, const void *n2) ; // by increasing y
int cmp_ixy_X (const void *n1, const void *n2) ; // by decreasing x
int cmp_ixy_Y (const void *n1, const void *n2) ; // by decreasing y
int cmp_x (const void *n1, const void *n2) ; // increasing float
int cmp_X (const void *n1, const void *n2) ; // decreasing float

void uniq_jix (jix_t *vec) ;
void chop_jix (jix_t *vec) ;

ix_t *vec_find (ix_t *vec, uint i) ;
ix_t *vec_set (ix_t *vec, uint id, float x) ;
float vec_get (ix_t *vec, uint id) ;

void uniq_vec (ix_t *vec) ;
void chop_vec (ix_t *vec) ;
void trim_vec (ix_t *vec, int k) ;
void trim_vec2(ix_t *vec, int k) ;
void trim_mtx (coll_t *M, int k) ;

ix_t *simhash (ix_t *vec, uint n, char *dist) ;
ix_t *bits2codes (ix_t *vec, uint L) ;
ix_t *num2bitvec (ulong num) ;
ulong bitvec2num (ix_t *vec) ;

ix_t *shuffle_vec (ix_t *vec) ;

ix_t *const_vec (uint n, float x) ;
ix_t *rand_vec (uint n) ;

ix_t *rand_vec_uni (uint n, float lo, float hi); // uniformly sampled over [lo..hi]
ix_t *rand_vec_std (uint n); // standard normal distribution N(0,1)
ix_t *rand_vec_exp (uint n); // exponential distribution
ix_t *rand_vec_log (uint n); // logistic distribution
ix_t *rand_vec_sphere (uint n); // uniformly over a unit sphere
ix_t *rand_vec_simplex (uint n); // uniformly over a multinomial simplex
ix_t *rand_vec_sparse (uint n, uint k) ; // [0,1] for k random positions

void scan_mtx (coll_t *rows, coll_t *cols, hash_t *rh, hash_t *ch) ;
ix_t *parse_vec_svm (char *str, char **id) ;
ix_t *parse_vec_csv (char *str, char **id) ;
ix_t *parse_vec_txt (char *str, char **id, hash_t *ids, char *prm) ;
ix_t *parse_vec_xml (char *str, char **id, hash_t *ids, char *prm) ;


void print_mtx (coll_t *rows, hash_t *rh, hash_t *ch) ;
void print_vec_rcv (ix_t *vec, hash_t *ids, char *vec_id, char *fmt) ;
void print_vec_svm (ix_t *vec, hash_t *ids, char *class, char *svm) ;
void print_vec_csv (ix_t *vec, uint ncols, char *fmt) ;
void print_vec_txt (ix_t*vec, hash_t *ids, char *vec_id, int xml) ;
void print_vec_top (ix_t *vec, hash_t *dict, uint top) ;

void transpose_mtx   (coll_t *rows, coll_t *cols) ;
void transpose_mtx_1 (coll_t *rows, coll_t *cols) ;
void transpose_mtx_2 (coll_t *rows, coll_t *cols) ;
coll_t *transpose (coll_t *M) ;

uint last_id (ix_t *vec) ;
uint num_rows (coll_t *rows) ;
uint num_cols (coll_t *rows) ;
uint *len_rows (coll_t *rows) ;
uint *len_cols (coll_t *rows) ;
float *sum_rows (coll_t *rows, float p) ;
float *sum_cols (coll_t *rows, float p) ;
float *norm_rows (coll_t *rows, float p, float root) ;
float *norm_cols (coll_t *rows, float p, float root) ;
float *cols_avg (coll_t *rows, float p) ;
double sum_sums (coll_t *rows, float p) ;
xy_t mrange (coll_t *m) ;

jix_t *ix2jix (uint j, ix_t *ix) ;
jix_t *max_rows (coll_t *rows) ;
jix_t *min_rows (coll_t *rows) ;
jix_t *max_cols (coll_t *rows) ;
jix_t *min_cols (coll_t *rows) ;
void m_range (coll_t *m, int top, float *_min, float *_max) ;
void disjoin_rows (coll_t *rows) ; // make all rows disjoint (column = indicator)

stats_t *blank_stats () ;
stats_t *coll_stats (coll_t *c) ;
void update_stats_from_vec (stats_t *s, ix_t *vec) ;
void update_stats_from_file (stats_t *s, hash_t *dict, char *file) ;
void free_stats (stats_t *s) ;
void dump_stats (stats_t *s, hash_t *dict) ;

ix_t *doc2lm (ix_t *doc, float *cf, double mu, double lambda) ;

void weigh_vec_clarity (ix_t *vec, stats_t *);
void weigh_vec_lmj (ix_t *vec, stats_t *) ;
void weigh_vec_lmd (ix_t *vec, stats_t *) ;
void weigh_vec_idf (ix_t *vec, stats_t *) ;
void weigh_vec_inq (ix_t *vec, stats_t *) ;
void weigh_vec_ntf (ix_t *vec, stats_t *s) ;
void weigh_vec_bm25 (ix_t *vec, stats_t *) ;
void weigh_vec_std (ix_t *vec, stats_t *) ;
void weigh_vec_laplacian (ix_t *vec, stats_t *s) ;
void weigh_vec_range01 (ix_t *vec) ;
void weigh_vec_rnd (ix_t *vec) ;
void weigh_vec_ranks (ix_t *vec) ;

void weigh_invl_lmd (ix_t *invl, stats_t *s, float mu) ;
void crop_outliers (ix_t *vec, stats_t *s, float Z) ;

void weigh_mtx (coll_t *M, char *prm, stats_t *s) ;

double log_sum_exp (ix_t *vec) ;
void softmax (ix_t *vec) ; // log2posterior
void col_softmax (coll_t *trg, coll_t *M) ;

float *chk_SCORE (uint N) ;

ix_t *full2vec (float *full) ;
ix_t *full2vec_keepzero (float *full) ;


float *vec2full (ix_t *vec, uint N) ;

ix_t *rows_x_vec (coll_t *rows, ix_t *vec) ;
ix_t *cols_x_vec (coll_t *cols, ix_t *vec) ;
ix_t *vec_x_rows (ix_t *vec, coll_t *rows) ;
void rows_x_cols (coll_t *out, coll_t *rows, coll_t *cols) ;
void rows_x_rows (coll_t *P, coll_t *A, coll_t *B) ;
void rows_o_rows (coll_t *out, coll_t *A, char op, coll_t *B) ;
void rows_a_rows (coll_t *out, float wa, coll_t *A, float wb, coll_t *B) ;

double dot (ix_t *A, ix_t *B) ;
double kdot (uint k, ix_t *A, ix_t *B) ;
double pdot (float p, ix_t *A, ix_t *B) ;
double pnorm (float p, ix_t *A, ix_t *B) ;
double wpnorm (float p, ix_t *A, ix_t *B, ix_t *W) ; // weighted p-norm
double pnorm_full (float p, ix_t *A, ix_t *B) ; // faster version for full vectors
double dot_full (ix_t *vec, float *full) ;

ix_t *max (ix_t *V) ;
ix_t *min (ix_t *V) ;
double sum (ix_t *V) ;
double sum2 (ix_t *V) ;
double sump (float p, ix_t *V) ;
double norm (float p, float root, ix_t *V) ;
double variance (ix_t *X, uint N) ;
double covariance (ix_t *X, ix_t *Y, uint N) ;
double MSE (ix_t *X, ix_t *Y, uint N) ; // N=1 gives SSE
double sumf (float *V) ;
float *maxf (float *V) ;
float *minf (float *V) ;
ulong  sumi (uint *V) ;
uint  *maxi (uint *V) ;

uint count (ix_t *V, char op, float x) ;

double H (ix_t *X, ix_t *Y, double mu, float *CF, ulong CL) ;
double KL (ix_t *X, ix_t *Y, double mu, float *CF, ulong CL) ;
double cosine (ix_t *X, ix_t *Y) ;
double dice (ix_t *X, ix_t *Y) ;
double jaccard (ix_t *X, ix_t *Y) ;
double hellinger (ix_t *X, ix_t *Y) ;
double chi2 (ix_t *X, ix_t *Y) ;
double chi2_full (ix_t *A, ix_t *B) ;
double correlation (ix_t *X, ix_t *Y, double N) ;
double rsquared (ix_t *Y, ix_t *F, double N) ;

double igain (ix_t *score, ix_t *truth, double N) ;

double F1 (ix_t *system, ix_t *truth) ;
double AveP (ix_t *system, ix_t *truth) ;
xy_t maxF1 (ix_t *system, ix_t *truth, double b) ;

void dump_evl_ixy (FILE *out, uint id, ixy_t *evl) ;
void dump_evl (FILE *out, uint id, ix_t *system, ix_t *truth) ;
void dump_roc (FILE *out, uint id, ix_t *system, ix_t *truth, uint top) ;

void eval_dump_evl (FILE *out, uint id, ixy_t *evl) ;
void eval_dump_roc (FILE *out, uint id, ixy_t *evl, float b) ;
void eval_dump_map (FILE *out, uint id, ixy_t *evl, char *prm) ;

ixy_t *join (ix_t *X, ix_t *Y, float def) ;
void disjoin (ix_t *X, ix_t *Y) ; // X & Y grab 

void vec_x_full (ix_t *V, char op, float *F) ;
ix_t *vec_x_vec (ix_t *X, char op, ix_t *Y) ;
void num_x_vec (double b, char op, ix_t *A) ;
void vec_x_num (ix_t *A, char op, double b) ;
ix_t *vec_add_vec (float x, ix_t *X, float y, ix_t *Y) ;
void vec_mul_vec (ix_t *V, ix_t *F) ; // fast, sparse, in-place
void filter_and (ix_t *V, ix_t *F) ; // in-place
void filter_not (ix_t *V, ix_t *F) ; // in-place
void filter_set (ix_t *V, ix_t *F, float def) ;
char *vec2set (ix_t *vec, uint N) ;
void vec_x_set (ix_t *vec, char op, char *set) ;

void f_vec (double (*F) (double), ix_t *A) ;

ix_t *vec_X_vec (ix_t *A, ix_t *B) ; // polynomial expansion

double ppndf (double p) ; // inverse gaussian (standard normal) cdf
float powa (float x, float p) ; // fast |x|^p
float rnd(); // = random() / RAND_MAX

#endif
