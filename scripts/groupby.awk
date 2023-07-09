#!/usr/local/bin/gawk -f

BEGIN {
    if (!RX) RX = "<[0-9]+>"; # group ids match RX
}

{
    id = match($0,RX,a) ? a[0] : "NOGROUP";
    G[id] = G[id]""$0"\n";
}

END {
    for (id in G) print "GROUP\t"id"\n"G[id]"END\t"id"\n";
}
