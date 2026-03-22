-- test-lua-syntax-error.lua: intentionally broken syntax
-- missing 'end' causes a compile-time syntax error
function broken(
  print("no closing paren or end")
