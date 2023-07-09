#!/usr/local/bin/gawk -f

function time2unix(time,unix) { # 2019-05-17,06:47:57.0301 -> 1558100877.0301
    gsub("[-,:]"," ",time);
    unix = sprintf("%d",mktime(substr(time,1,19)));
    if (length(time>19)) unix = unix""substr(time,20);
    return unix;
}

function unix2time(unix,time) { # 1558100877.0301 -> 2019-05-17,06:47:57.0301
    time = strftime("%F,%T",int(unix));
    frac = index(unix,".");
    if (frac) time = time""substr(unix,frac);
    return time;
}

function time2time(t1,conv) {
    gsub("[-,:]"," ",t1);
    if (length(t1) == 10) t1 = t1" 00 00 01";
    return strftime(conv,mktime(t1));
}

BEGIN {
    if (!f) f = 1;
    if (d) FS = OFS = d;
}

NR == 1 { inv = index($f,","); }

{
    if (conv)     $f = time2time($f,conv);
    else if (inv) $f = time2unix($f);
    else          $f = unix2time($f);
    print;
}
