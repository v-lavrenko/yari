#!/bin/bash

curl -s -L $1 | lynx -dump -stdin \
| gawk '/^References$/ {p=1} p && /^ *[0-9]+[.] / {print $2}' \
| gawk -vR=$1 -F/ '/^file:/ {$0 = R"/"$NF} {print}'
