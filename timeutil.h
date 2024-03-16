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

#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef TIMEUTIL
#define TIMEUTIL

#define MINUTES   60
#define HOURS   3600
#define DAYS   86400
#define WEEKS 604800

extern char *strptime(const  char *buf, const char *format, struct tm *tm);

// convert abbreviated time to number of seconds, e.g. 2h --> 7200
int str2seconds (char *s) ;

// convert string expression, e.g. 2004-02-13,18:01:39 --> unix timestamp
time_t str2time (char *str) ;
time_t strf2time (char *str, char *fmt) ;

struct tm time2tm (time_t time) ; // unix timestamp --> broken-down time
//                  int tm_sec;         /* seconds */
//                  int tm_min;         /* minutes */
//                  int tm_hour;        /* hours */
//                  int tm_mday;        /* day of the month */
//                  int tm_mon;         /* month */
//                  int tm_year;        /* year */
//                  int tm_wday;        /* day of the week */
//                  int tm_yday;        /* day in the year */
//                  int tm_isdst;       /* daylight saving time */

char *time2strf (char *buf, char *format, time_t time) ; 
char *time2str (char *buf, time_t time) ;
int time2hour (time_t time) ;
int time2wday (time_t time) ;
int same_day (time_t t1, time_t t2) ;
int same_week (time_t t1, time_t t2) ;
time_t today_hhmm (time_t now, char *hhmm) ;

double ftime (); // time in seconds.microseconds since epoch
double mstime(); // time in milliseconds sinse epoch
unsigned long ustime (); // time in microseconds since epoch
double msdiff (double *T) ; // milliseconds since T, update T
void loglag(char *tag) ; // log time elapsed since last call

#endif
