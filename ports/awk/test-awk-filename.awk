FNR == 1 {
    n = split(FILENAME, parts, ":")
    if (n > 1)
        print parts[n]
    else
        print FILENAME
    if (NR >= 2) exit
}
