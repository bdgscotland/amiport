-- test-lua-string-ops.lua: string library operations
local s = "Hello, Amiga!"
print(string.len(s))
print(string.sub(s, 1, 5))
print(string.lower(s))
print(string.rep("ab", 3))
