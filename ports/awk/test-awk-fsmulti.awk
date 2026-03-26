BEGIN { FS = ":+" }
{ print NF, $3; exit }
