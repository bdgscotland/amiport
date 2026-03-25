BEGIN {
    a["x"] = 1
    a["y"] = 2
    a["z"] = 3
    delete a["y"]
    for (k in a) count++
    print count
}
