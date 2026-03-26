BEGIN {
    print "written" > "T:awk-pf-test.txt"
    close("T:awk-pf-test.txt")
    getline line < "T:awk-pf-test.txt"
    print line
    close("T:awk-pf-test.txt")
}
