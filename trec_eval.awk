#!/usr/bin/gawk -f

{
    ap = rel = 0; 
    for (ret=3; ret<NF; ++ret) ap += ++rel / $ret; 
    ap /= rel; 
    map += ap; 
    ++qrys; 
}

END { printf "%.4f MAP over %3d qrys in %s\n", (map / qrys), qrys, FILENAME }
