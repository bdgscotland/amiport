BEGIN { a["x"]=1; a["y"]=2; delete a["x"]; n=0; for(k in a) n++; print n }
