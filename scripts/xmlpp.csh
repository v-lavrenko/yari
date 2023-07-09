#!/bin/tcsh -f

# null, TAB, CR, LF -> spaces
# &amp; &nbsp; etc -> spaces
# XML <tag> on its own line
# no empty lines + collapse spaces

tr '\0\t\r' ' ' \
| sed 's/&[^;]*;/ /g' \
| sed 's/</\r</g' \
| sed 's/>/>\r/g' \
| tr '\n' ' '	\
| tr '\r' '\n' \
| grep '[^ ]' \
| sed 's/  */ /g'

#| sed 's/<xref[^>]*>//g' \
#| sed 's/<\/xref>//g' \
#| sed 's/<\/*bold>//g' \
#| sed 's/<\/*italic>//g' \
#| sed 's/<\/*su[bp]>//g' \
