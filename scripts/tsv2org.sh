#!/usr/bin/env -S tcsh -f

foreach f ($argv)
  echo "** $f"
  cat $f | gawk '++n[$3,$4,$5]==1' | head | tr '\t' '|' | sed 's/^/  |/'
  echo ""
end
