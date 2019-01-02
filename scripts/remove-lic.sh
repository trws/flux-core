#strip from c/h
rg -l CODE-658032 src t | grep '\.[ch]p*$' | xargs perl -0777 -i.original -pe 's|^/\**\\.*?Copyr.*?CODE-658032.*?licenses/.*?\**/\n?||igs'
#strip from lua
rg -l CODE-658032 src t | grep '\.lua$' | xargs perl -0777 -i.original -pe 's|^--\[\[-*\n.*?Copyr.*?CODE-658032.*?licenses/\n -*\]\]\n?||igs'

