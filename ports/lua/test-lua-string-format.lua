-- test-lua-string-format.lua: string.format() with various format specifiers
print(string.format("%d", 42))
print(string.format("%05d", 42))
print(string.format("%.2f", 3.14159))
print(string.format("%s and %s", "hello", "world"))
print(string.format("%x", 255))
print(string.format("%q", 'say "hello"'))
