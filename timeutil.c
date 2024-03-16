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

#include "timeutil.h"
#include "hl.h"

double mstime () { // time in milliseconds
  struct timespec tp;
  clock_gettime (CLOCK_MONOTONIC, &tp);
  return 1000.*tp.tv_sec + tp.tv_nsec/1E6;
}

double ftime () { // time in seconds.micros since epoch
  struct timeval tv;
  struct timezone tz;
  if (!gettimeofday(&tv,&tz)) return tv.tv_sec + tv.tv_usec / 1E6;
  perror("gettimeofday failed");
  return 0;
}

unsigned long ustime () { // time in microseconds since epoch
  struct timeval tv;
  struct timezone tz;
  if (!gettimeofday(&tv,&tz)) return 1E6 * tv.tv_sec + tv.tv_usec;
  perror("gettimeofday failed");
  return 0;
}

double msdiff (double *t) {
  double t0 = *t;
  *t = mstime();
  return t0 ? (*t - t0) : 0;
}

// print time elapsed since last call
void loglag(char *tag) {
  static double t0 = 0;
  double t1 = mstime(), lag = t1 - t0;
  t0 = t1;
  if (!tag || !*tag) return;
  char *color = (lag <   10 ? fg_GREEN :
		 lag <   30 ? fg_CYAN :
		 lag <  100 ? fg_YELLOW :
		 lag <  300 ? fg_MAGENTA :
		 lag < 1000 ? fg_RED : bg_MAGENTA);
  fputs(color,stderr);
  fputs(tag,stderr);
  fputs(" "RESET,stderr);
}


// convert abbreviated time to number of seconds, e.g. 2h --> 7200
int str2seconds (char *s) {
  char *ptr = s;
  double sec = (int) strtod (s, &ptr);
  if      (*ptr == 'm') sec *= MINUTES;
  else if (*ptr == 'h') sec *= HOURS;
  else if (*ptr == 'd') sec *= DAYS;
  else if (*ptr == 'w') sec *= WEEKS;
  return (int) sec;
}

// convert string expression, e.g. 2004-02-13,18:01:39 --> unix timestamp
time_t strf2time (char *str, char *fmt) {
  struct tm tm;
  memset (&tm, 0, sizeof(struct tm));
  if (!strptime(str, fmt, &tm)) return 0;
  tm.tm_isdst = -1; // strptime does not set this!!!
  time_t time = mktime (&tm);
  return (unsigned) time;
  //printf ("%s -> %u -> %s \n", str, (unsigned) time, time2strf(time,"%F,%T"));
  //if (!ok) {
  //fprintf (stderr, "str2time() couldn't parse: %s", str); exit(1);}
  //time_t min =  473403600; // Jan 1 1985
  //time_t max = 1262322000; // Jan 1 2010
  //if ((time < min) || (time > max)) {
  //fprintf (stderr, "str2time() out of range: %s", str); exit(1); }
}

// convert string expression, e.g. 2004-02-13,18:01:39 --> unix timestamp
time_t str2time (char *str) {
  return strf2time(str,"%Y-%m-%d,%H:%M:%S");
}

struct tm time2tm (time_t time) {
  struct tm tm;
  localtime_r (&time, &tm);
  return tm;
}

char *time2strf (char *buf, char *format, time_t time) {
  struct tm tm = time2tm (time);
  strftime (buf, 199, format, &tm);
  return buf;
}

char *time2str (char *buf, time_t time) {
  return time2strf (buf, "%F,%T", time);
}

int time2hour (time_t time) { 
  return time2tm(time).tm_hour; 
}

int time2wday (time_t time) { 
  return time2tm(time).tm_wday; 
}

int same_day (time_t t1, time_t t2) {
  int d1 = time2tm(t1).tm_wday;
  int d2 = time2tm(t2).tm_wday;
  return (d1 == d2) && ((t2 - t1) < 1 * DAYS); 
}

int same_week (time_t t1, time_t t2) {
  int d1 = time2tm(t1).tm_wday;
  int d2 = time2tm(t2).tm_wday;
  return (d1 <= d2) && ((t2 - t1) < 5 * DAYS); 
}

time_t today_hhmm (time_t now, char *hhmm) {
  struct tm tm = time2tm(now);
  tm.tm_hour = atoi (hhmm);
  tm.tm_min = atoi (strchr(hhmm,':')+1);
  tm.tm_sec = 0;
  return mktime (&tm);
}

/*
typedef struct {
  time_t beg;
  time_t last;
  uint dots;
} eta_t;

inline void *show_eta (ulong done, ulong total, char *s, void *_state) {
  eta_t *state = (eta_t *)_state;
  if (!state) {
    state = malloc (sizeof(eta_t));
    state->beg = eta->last = time(0);
    state->dots = 0;
  }
  time_t now = time(0); // 3 clock cycles, 3x faster than anything else
  if (now == eta->last) return; // <1s since last invoke
  eta->last = now;
  if (eta->beg == 0) { eta->beg = now; return; }
  fprintf (stderr, ".");
  
  return (void*) state;
}

inline void show_progress (ulong done, ulong total, char *s) { // thread-unsafe: static
  static ulong dots = 0, prev = 0, line = 50;
  static time_t last = 0, begt = 0;
  time_t this = time(0);
  //printf ("%d %d %d\n", this, last, CLOCKS_PER_SEC);
  //if (this - last < CLOCKS_PER_SEC) return; 
  if (this == last) return;
  last = this;
  fprintf (stderr, ".");
  if (!begt) begt = this;
  if (done < prev) prev = done; 
  if (++dots < line) return;
  double todo = total-done, di = done-prev, ds = this-begt, rpm = 60*di/ds, ETA = todo/rpm;
  //double ETA = ((double)(N-n)) / ((n-m) * 60 / line); // minutes
  if (!total) fprintf (stderr, "%ld %s @ %.0f / minute\n", done, s, rpm);
  else {      fprintf (stderr, "%ld / %ld %s", done, total, s);
    if (ETA < 60)        fprintf (stderr, " ETA: %.1f minutes\n", ETA);
    else if (ETA < 1440) fprintf (stderr, " ETA: %.1f hours\n", ETA/60);
    else                 fprintf (stderr, " ETA: %.1f days\n", ETA/1440);
  }
  prev = done;
  begt = this;
  dots = 0;
}
*/

/*

http://stackoverflow.com/questions/6498972/faster-equivalent-of-gettimeofday

time (s) => 3 cycles
ftime (ms) => 54 cycles
gettimeofday (us) => 42 cycles
clock_gettime (ns) => 9 cycles (CLOCK_MONOTONIC_COARSE)
clock_gettime (ns) => 9 cycles (CLOCK_REALTIME_COARSE)
clock_gettime (ns) => 42 cycles (CLOCK_MONOTONIC)
clock_gettime (ns) => 42 cycles (CLOCK_REALTIME)
clock_gettime (ns) => 173 cycles (CLOCK_MONOTONIC_RAW)
clock_gettime (ns) => 179 cycles (CLOCK_BOOTTIME)
clock_gettime (ns) => 349 cycles (CLOCK_THREAD_CPUTIME_ID)
clock_gettime (ns) => 370 cycles (CLOCK_PROCESS_CPUTIME_ID)
rdtsc (cycles) => 24 cycles

http://www.systutorials.com/5086/measuring-time-accurately-in-programs/

time (s) => 4ns
ftime (ms) => 39ns
gettimeofday (us) => 30ns
clock_gettime (ns) => 26ns (CLOCK_REALTIME)
clock_gettime (ns) => 8ns (CLOCK_REALTIME_COARSE)
clock_gettime (ns) => 26ns (CLOCK_MONOTONIC)
clock_gettime (ns) => 9ns (CLOCK_MONOTONIC_COARSE)
clock_gettime (ns) => 170ns (CLOCK_PROCESS_CPUTIME_ID)
clock_gettime (ns) => 154ns (CLOCK_THREAD_CPUTIME_ID

*/
