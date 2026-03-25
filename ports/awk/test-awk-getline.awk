BEGIN {
    while ((getline line < "WORK:test-awk-words.txt") > 0)
        print toupper(line)
}
