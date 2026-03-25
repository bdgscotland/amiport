{ count[$1]++ }
END { for (k in count) print k, count[k] }
