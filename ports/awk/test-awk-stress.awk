BEGIN {
    total = 0
    for (i = 1; i <= 10000; i++)
        total += i
    print total
}
