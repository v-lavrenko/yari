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
