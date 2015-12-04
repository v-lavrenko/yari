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

#include "timeutil.h"

double ftime () { // time in seconds.micros since epoch
  struct timeval tv; 
  struct timezone tz;
  if (!gettimeofday(&tv,&tz)) return tv.tv_sec + tv.tv_usec / 1E6;
  perror("gettimeofday failed");
  return 0;
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
time_t str2time (char *str) { 
  struct tm tm; 
  memset (&tm, 0, sizeof(struct tm));
  if (!strptime(str, "%Y-%m-%d,%H:%M:%S", &tm)) return 0;
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

struct tm time2tm (time_t time) {
  struct tm tm; 
  localtime_r (&time, &tm);
  return tm;
}

char *time2strf (char *format, time_t time) {
  static char buf[100];
  struct tm tm = time2tm (time);
  strftime (buf, 99, format, &tm);
  return buf; 
}

char *time2str (time_t time) { 
  return time2strf ("%F,%T", time); 
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
