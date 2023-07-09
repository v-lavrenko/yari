#!/usr/local/bin/gawk -f

BEGIN { # convert every instance of old id to new format
    if (!OLD) OLD = "<DOCID>[^<]+"; # old ids look like this
    if (!NEW) NEW = "<%d>"; # new format looks like this
    if (!NN) NN = 0; # first new id (-1)
}

match($0,OLD,a) {
    old = a[0]; # extract the ID
    if (old in N) n = N[old]; # seen before: use known number
    else n = N[old] = ++NN; # never seen: assign new number
    new = sprintf(NEW,n);
    gsub(OLD,new,$0);
}

{ print; }
