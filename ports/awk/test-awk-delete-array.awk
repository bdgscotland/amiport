BEGIN { a[1]=1; a[2]=2; delete a; n=0; for(k in a) n++; print n }
