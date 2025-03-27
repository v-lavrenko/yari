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

#ifndef MATRIX
#define MATRIX

//typedef struct { uint r; uint c; float v; } rcv_t;

//typedef struct { uint c; float v; } cv_t;

//typedef struct {char*path; coll_t*rows; coll_t*cols; hash_t*rids; hash_t*cids;} mtx_t;

typedef struct {
  ulong   ndocs;
  ulong   nwords;
  double nposts;
  ulong  *df; // document frequency = non-zeros in each column
  double *cf; // collection frequency = sum of each column
  double *s2; // sum of squares in each column
  double  k; // free parameter
  double  b; // free parameter
} stats_t;

void uniq_jix (jix_t *vec) ;
void chop_jix (jix_t *vec) ;

ix_t *vec_find (ix_t *vec, uint i) ;
ix_t *vec_set (ix_t *vec, uint id, float x) ;
float vec_get (ix_t *vec, uint id) ;
uint vec_is_sorted (ix_t *V) ;


void dedup_vec (ix_t *vec) ; // skip duplicate ids (keep 1st)
void aggr_vec (ix_t *vec, char aggr) ; // min, Max, sum, avg, 1st
void sort_uniq_vec (ix_t *vec) ; // sortd by id, then below
void uniq_vec (ix_t *vec) ; // merge duplicate ids (add weights)
void chop_vec (ix_t *vec) ;
void trim_vec (ix_t *vec, int k) ;
void trim_vec2(ix_t *vec, int k) ;
void trim_mtx (coll_t *M, int k) ;
void qselect (ix_t *X, int k) ; // helper for trim_vec
void nksample (ix_t *X, uint n, int k) ; // keep n elements, preserve top k / bottom -k

void drop_vec_el (ix_t *vec, uint el) ;
void sparse_vec (ix_t *vec, float zero) ;
ix_t *dense_vec (ix_t *vec, float zero, uint n) ;

ix_t *simhash (ix_t *vec, uint n, char *dist) ;
ix_t *bits2codes (ix_t *vec, uint L) ;
ix_t *num2bitvec (ulong num) ;
ulong bitvec2num (ix_t *vec) ;

ix_t *vec2psg (ix_t *V, uint i, uint w); // i'th w/2-overlapping passage or NULL
ix_t *shuffle_vec (ix_t *vec) ;
void permute_vec (ix_t *V, uint nc) ; // scatter values into random columns 1..nc
uint *random_ints(uint k, uint M) ; // k random ints in [0..M) with replacement
uint *sample_ints(uint k, uint M) ; // k distinct ints in [0..M) without replacement

ix_t *const_vec (uint n, float x) ;
ix_t *rand_vec (uint n) ;

ix_t *rand_vec_uni (uint n, float lo, float hi); // uniformly sampled over [lo..hi]
ix_t *rand_vec_std (uint n); // standard normal distribution N(0,1)
ix_t *rand_vec_exp (uint n); // exponential distribution
ix_t *rand_vec_log (uint n); // logistic distribution
ix_t *rand_vec_sphere (uint n); // uniformly over a unit sphere
ix_t *rand_vec_simplex (uint n); // uniformly over a multinomial simplex
ix_t *rand_vec_sparse (uint n, uint k) ; // [0,1] for k random positions

void rand_mtx (coll_t *M, uint nr, uint nc) ;
void rand_mtx_sparse (coll_t *M, uint nr, uint nc, uint k) ;

void scan_mtx (coll_t *rows, coll_t *cols, hash_t *rh, hash_t *ch, char *prm) ;
ix_t *parse_vec_svm (char *str, char **id, hash_t *ids) ;
ix_t *parse_vec_csv (char *str, char **id) ;
ix_t *parse_vec_txt (char *str, char **id, hash_t *ids, char *prm) ;
ix_t *parse_vec_xml (char *str, char **id, hash_t *ids, char *prm) ;
ix_t *parse_as_ngrams (char *str, hash_t *ids, int ngramsz, char *pos);

void print_mtx (coll_t *rows, hash_t *rh, hash_t *ch, char *how) ;
void print_vec_rcv (ix_t *vec, hash_t *ids, char *vec_id, char *fmt) ;
void print_vec_svm (ix_t *vec, hash_t *ids, char *class, char *svm) ;
void print_vec_txt (ix_t*vec, hash_t *ids, char *vec_id, int xml) ;
void print_vec_top (ix_t *vec, hash_t *dict, uint top) ;
void print_vec_json (ix_t *vec, hash_t *ids, char *vec_id, char *fmt) ;
void print_vec_csv (ix_t *vec, uint ncols, char *vec_id, char *fmt) ;
void print_hdr_csv (hash_t *H, uint ncols);

void transpose_mtx   (coll_t *rows, coll_t *cols) ;
void transpose_mtx_1 (coll_t *rows, coll_t *cols) ;
void transpose_mtx_2 (coll_t *rows, coll_t *cols) ;
coll_t *transpose (coll_t *M) ;

uint last_id (ix_t *vec) ;
uint num_rows (coll_t *rows) ;
uint num_cols (coll_t *rows) ;
uint *row_ids(coll_t *rows) ; // list of non-empty row ids
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
void dump_stats (stats_t *s, hash_t *dict, char *prm) ;
stats_t *open_stats (char *path) ;
stats_t *open_stats_if_exists (char *path) ;
void save_stats (stats_t *s, char *path) ;

ix_t *doc2lm (ix_t *doc, double *cf, double mu, double lambda) ;

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
void weigh_vec_cdf (ix_t *vec) ;

void weigh_invl_lmd (ix_t *invl, stats_t *s, float mu) ;
void zstd_outliers (ix_t *vec, stats_t *s, float Z) ;
void keep_outliers (ix_t *X, xy_t CI) ;
void crop_outliers (ix_t *X, xy_t CI) ;
void drop_far_outliers (ix_t *X, xy_t CI) ;
xy_t cdf_interval (ix_t *X, float p) ;
xy_t median_interval (ix_t *_X, float p) ;
xy_t compact_interval (ix_t *_X, float p) ;
xy_t mean_and_stdev(ix_t *_X, float p) ;

float ci_value_threshold (ix_t *V, float p, float z) ;
float ci_delta_threshold (ix_t *V, float p, float z) ;

// reweigh every vector in M, or given vector V (if M==0)
void weigh_mtx_or_vec (coll_t *M, ix_t *V, char *prm, stats_t *s) ;

double log_sum_exp (ix_t *vec) ;
void softmax (ix_t *vec) ; // log2posterior
void col_softmax (coll_t *trg, coll_t *M) ;

float *chk_SCORE (uint N) ;

ix_t *full2vec (float *full) ;
ix_t *double2vec (double *full) ;
ix_t *full2vec_keepzero (float *full) ;
float *vec2full (ix_t *vec, uint N, float def) ;

uint *vec2ids (ix_t *V) ; // sequence of ids
ix_t *ids2vec (uint *U, float x) ;

ix_t *rows_x_vec (coll_t *rows, ix_t *vec) ;
ix_t *cols_x_vec (coll_t *cols, ix_t *vec) ;
ix_t *vec_x_rows (ix_t *vec, coll_t *rows) ;
void rows_x_cols (coll_t *out, coll_t *rows, coll_t *cols) ;
void rows_x_rows (coll_t *P, coll_t *A, coll_t *B) ;
void rows_o_rows (coll_t *out, coll_t *A, char op, coll_t *B) ;
void rows_a_rows (coll_t *out, float wa, coll_t *A, float wb, coll_t *B) ;
void filter_rows (coll_t *rows, char op, ix_t *mask) ;
void rows_x_num (coll_t *rows, char op, double num) ;
ix_t *cols_x_vec_iseen (coll_t *cols, ix_t *V) ; // SCORE + list of seen ids
ix_t *cols_x_vec_iskip (coll_t *cols, ix_t *V) ; // SCORE + skip-lists

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
double median (ix_t *X, uint n) ;
double variance (ix_t *X, uint N) ;
double mean(ix_t *X) ;
double stdev(ix_t *X) ;
double covariance (ix_t *X, ix_t *Y, uint N) ;
double MSE (ix_t *X, ix_t *Y, uint N) ; // N=1 gives SSE
double sumf (float *V) ;
float *maxf (float *V) ;
float *minf (float *V) ;
ulong  sumi (uint *V) ;
uint  *maxi (uint *V) ;

uint count (ix_t *V, char op, float x) ;
ix_t *distinct_values (ix_t *V, float eps) ;
double value_entropy (ix_t *V) ;
ix_t *value_deltas(ix_t *X) ;

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
float *mtx_full_row (char *_M, uint row) ; // open M, return full(row), close M

ixy_t *join (ix_t *X, ix_t *Y, float def) ;
void disjoin (ix_t *X, ix_t *Y) ; // X & Y grab
ixy_t *ix2ixy (ix_t *ix, float y) ;


void vec_x_full (ix_t *V, char op, float *F) ;
ix_t *vec_x_vec (ix_t *X, char op, ix_t *Y) ;
void num_x_vec (double b, char op, ix_t *A) ;
void vec_x_num (ix_t *A, char op, double b) ;
void vec_x_range (ix_t *A, char op, xy_t R) ;
ix_t *vec_add_vec (float x, ix_t *X, float y, ix_t *Y) ;
void vec_mul_vec (ix_t *V, ix_t *F) ; // fast, sparse, in-place
void filter_and (ix_t *V, ix_t *F) ; // in-place
void filter_not (ix_t *V, ix_t *F) ; // in-place
void filter_set (ix_t *V, ix_t *F, float def) ;
void filter_and_sum (ix_t *V, ix_t *F) ; // Boolean AND + SUM the scores
void filter_sum (ix_t **V, ix_t *F) ; // OR + sum scores + free old vec
char *vec2set (ix_t *vec, uint N) ;
void vec_x_set (ix_t *vec, char op, char *set) ;

void f_vec (double (*F) (double), ix_t *A) ;

ix_t *vec_X_vec (ix_t *A, ix_t *B) ; // polynomial expansion

double ppndf (double p) ; // inverse gaussian (standard normal) cdf
float powa (float x, float p) ; // fast |x|^p
float rnd(); // = random() / RAND_MAX

uint mtx_append (coll_t *M, uint id, ix_t *vec, char how) ;
ix_t *next_in_jix (jix_t *jix, jix_t **last) ;
void transpose_jix (jix_t *vec) ;
jix_t *scan_jix (FILE *in, uint num, hash_t *rows, hash_t *cols) ;
void scan_mtx_rcv (FILE *in, coll_t *M, hash_t *R, hash_t *C, char how, char verb) ;
void append_jix (coll_t *c, jix_t *jix) ;

void show_jix (jix_t *J, char *tag, char how);

#endif
