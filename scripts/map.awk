#!/usr/bin/env -S gawk -f

function dedup_1st() { # keep 1st of any duplicate keys
    ARGV[1] = "";
    while (getline > 0) if (++N[$1] == 1) print;
    exit;
}

function dedup_cat() { # concatenate values for duplicates
    if (!sep) sep=" | ";
    ARGV[1] = "";
    while (getline > 0)
	if ($1 in M) M[$1] = M[$1]""sep""$2;
	else M[$1] = $2;
    for (k in M) print k, M[k];
    exit;
}

BEGIN {
    if (ARGC < 2) {
	print "Usage: map.awk [-vf=1] [-vh=Name] MAP < TSV";
	print "               -1st < MAP1 > MAP2";
	print "               -cat < MAP1 > MAP2";	
	exit;
    }
    
    OFS = FS = "\t"; # Tab-separated everything
    if (sep) OFS = FS = sep; # unless overriden
    
    if (ARGV[1] == "-1st") dedup_1st();
    if (ARGV[1] == "-cat") dedup_cat();
    
    if (!f) f = 1; # assume we're mapping field 1
    if (!o) o = 0; # replace $o, or append if 0
    if (!none) none = ""; # if $f not found in map
    if (!strict) strict = 0; # drop if $f not found

    # map is a file containing tab-separated key-value pairs
    #if    (0 < (getline < ARGV[1])) hdr = $2; # column name
    while (0 < (getline < ARGV[1])) map[$1] = $2;
    print "map.awk: loaded", ARGV[1] > "/dev/stderr";
    ARGV[1] = ""; # otherwise gawk will read from it again
}

NR==1 && h { print $0, h; next; }

{
    found = ($f in map);
    if (strict && !found) next;
    val = found ? map[$f] : none; 
    if (o) { $o = val; print }
    else print $0, val;
}

# { print $0,  ($f in map) ? map[$f] : ""; } # old
