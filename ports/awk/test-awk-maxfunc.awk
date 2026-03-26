function max(a, b,    result) {
    result = (a > b) ? a : b
    return result
}
BEGIN { print max(7, 3) }
