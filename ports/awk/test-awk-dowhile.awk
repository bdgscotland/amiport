BEGIN {
    i = 1
    s = 0
    do {
        s += i
        i *= 2
    } while (i <= 16)
    print s
}
