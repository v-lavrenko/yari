#!/usr/bin/python3 
### apt install python3-unidecode
import sys
from unidecode import unidecode
for line in sys.stdin:
    sys.stdout.write(unidecode(line))
