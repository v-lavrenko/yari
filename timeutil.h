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

char *time2strf (char *format, time_t time) ; 
char *time2str (time_t time) ;
int time2hour (time_t time) ;
int time2wday (time_t time) ;
int same_day (time_t t1, time_t t2) ;
int same_week (time_t t1, time_t t2) ;
time_t today_hhmm (time_t now, char *hhmm) ;

double ftime (); // time in microseconds since epoch

#endif
