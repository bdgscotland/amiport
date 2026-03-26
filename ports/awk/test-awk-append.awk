BEGIN {
    print "line1" > "T:awk-app-test.txt"
    print "line2" >> "T:awk-app-test.txt"
    close("T:awk-app-test.txt")
    while ((getline line < "T:awk-app-test.txt") > 0)
        print line
    close("T:awk-app-test.txt")
}
