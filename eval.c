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

double AveP (ix_t *system, ix_t *truth) {
  double sumPrec = 0, rel = 0, ret = 0;
  ixy_t *evl = join (system, truth, 0), *e; // x = system, y = truth
  sort_vec (evl, cmp_ixy_x); // sort by decreasing index scores
  for (e = evl; e < evl + len(evl); ++e, ++ret) 
    if (e->y) sumPrec += (++rel / ret); 
  free_vec (evl);
  return sumPrec / rel;
}

/*
double correlation (ix_t *system, ix_t *truth) {
  ixy_t *evl = join (system, truth, 0), *e; // x = system, y = truth
  sort_vec (evl, cmp_ixy_x); // sort by decreasing index scores
  for (e = evl; e < evl + len(evl); ++e) {
    EX += e->x; EY += e->Y; 
  }
  if (e->y) sumPrec += (++rel / ret); 
}
*/

/*
void dump_evl (FILE *out, uint id, ix_t *system, ix_t *truth) {
  ixy_t *evl = join (system, truth, 0), *e; // x = system, y = truth
  sort_vec (evl, cmp_ixy_x); // sort by decreasing index scores
  fprintf (out, "%d %d", id, len(truth));
  for (e = evl; e < evl + len(evl); ++e) 
    if (e->y) fprintf (out, " %d", e - evl);
  fprintf (out, " /\n");
  free_vec (evl);
}
*/
