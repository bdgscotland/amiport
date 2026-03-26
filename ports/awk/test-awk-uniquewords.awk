{
    for (i = 1; i <= NF; i++)
        words[$i]++
}
END {
    n = 0
    for (w in words) n++
    print n
}
