{ words += NF; chars += length($0) + 1 }
END { print NR, words, chars }
