BEGIN {
    for (i = 1; i <= 5; i++) {
        if (i % 2 == 0)
            print i, "even"
        else
            print i, "odd"
    }
}
