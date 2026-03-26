BEGIN { s=0; for(i=1;i<=5;i++) { if(i==3) continue; s+=i }; print s }
