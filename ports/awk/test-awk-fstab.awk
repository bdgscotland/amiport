BEGIN { FS = "\t" }
{ print NF, $2; exit }
