#!/usr/bin/gawk -f

BEGIN {
    print "GPU\tMEM\tPSU\tTemp\tFan\tProcess";    
    while ("nvidia-smi -a -q -l 3" | getline) {
	if (/Fan Speed/) fan = $(NF-1);
	if (/GPU Current Temp/) temp = $(NF-1);
	if (/Power Draw/) psu = $(NF-1);
	if (/Memory/ && /%/) mem = $(NF-1);
	if (/Gpu/ && /%/) gpu = $(NF-1);
	if (/Processes/) printf "%s%%\t%s%%\t%dW\t%sºC\t%s%%\t", gpu, mem, psu, temp, fan;
	if (/Processes/ && /None/) print "";
	if (/Process ID/) system("ps -p "$NF" -o command=");
    }
}
