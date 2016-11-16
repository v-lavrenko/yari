/*
  
  Copyright (c) 1997-2016 Victor Lavrenko (v.lavrenko@gmail.com)
  
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

char *time2strf (char *buf, char *format, time_t time) {
  struct tm tm = time2tm (time);
  strftime (buf, 99, format, &tm);
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
