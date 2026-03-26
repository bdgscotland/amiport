{ words[$1]++ }
END {
    n = 0
    for (w in words) n++
    print n
}
