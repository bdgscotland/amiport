BEGIN {
    s = 0
    for (i = 1; i <= 10; i++)
        for (j = 1; j <= 10; j++)
            s += i * j
    print s
}
