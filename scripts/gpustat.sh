
nvidia-smi -a -q -l 3 | gawk '\
BEGIN {print "GPU\tMEM\tPSU\tTemp\tFan\tProcess"}
/Fan Speed/ {fan = $(NF-1)} \
/GPU Current Temp/ {temp = $(NF-1)} \
/Power Draw/ {psu = $(NF-1)} \
/Memory/ && /%/ {mem = $(NF-1)} \
/Gpu/ && /%/ {gpu = $(NF-1)} \
/Process ID/ {printf "%s%%\t%s%%\t%dW\t%sºC\t%s%%\t", gpu, mem, psu, temp, fan; \
	      system("ps -p "$NF" -o command=")}'

#echo 'GPU%,MEM%,PSU,,Temp,Fan,Card' | tr ',' '\t'
#nvidia-smi -l --format=csv,noheader \
#--query-gpu=utilization.gpu,utilization.memory,power.draw,temperature.gpu,fan.speed,name \
#| tr ',' '\t'
#| gawk -F, '{gsub(" ",""); printf "gpu:%s mem:%s psu:%s T:%s, fan:%s\n", $1, $2, $3, $4, $5, $6}'

# nvidia-smi -a -q
# nvidia-smi -q -g 0 -d UTILIZATION -l
# nvidia-smi pmon -i 0

