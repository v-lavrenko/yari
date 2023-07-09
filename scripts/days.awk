#!/usr/local/bin/gawk -f

BEGIN {
    if (!n) { print "Usage: days.awk -vn=1 -vpre=5 -vfmt='%F' -vwk=1 -vwe=1 -vbeg=2019-01-01 -vend=2020-01-01" > "/dev/stderr"; }
    if (!fmt) fmt = "%F";
    if (!hop) hop = 1;
    if (!day) day = hop * 24 * 3600;
    if (beg) { gsub("-"," ",beg); beg = mktime(beg" 06 00 01"); }
    if (end) { gsub("-"," ",end); end = mktime(end" 18 59 59"); }
    now = systime();
    if (pre) beg = now - pre * day;
    if (!beg) beg = now;
    if (!end) end = beg + (n ? (n-1)*day : 0);
    for (t = beg; t <= end; t += day) {
	if (wk && strftime("%u",t) > 5) continue; # only weekdays
	if (we && strftime("%u",t) < 6) continue; # only weekends
	print strftime(fmt,t);
    }
}
