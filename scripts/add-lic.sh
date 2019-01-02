#!/bin/bash
for F in $(rg -P --files-without-match '(llnl-code|copyright)' | grep '\.[ch]p*$') ; do
  mv $F $F.prelic
  cat scripts/lic $F.prelic > $F
done
for F in $(rg -P --files-without-match '(llnl-code|copyright)' | grep '\.lua$') ; do
  mv $F $F.prelic
  cat scripts/lic-lua $F.prelic > $F
done
