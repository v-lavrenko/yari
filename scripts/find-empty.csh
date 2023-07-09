#!/bin/tcsh -f

if ("$argv" == "") then
    echo "Usage: $0 dir1 dir2 ..."
    exit
endif

foreach dir ($argv)
foreach f (`find $dir -type f -size -100c`)
    set sz = `acat $f | wc -c`
    if (""$sz == "0") echo $f
end
end
