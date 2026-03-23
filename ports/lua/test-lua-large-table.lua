-- test-lua-large-table.lua: build and traverse a large table (1000 entries)
-- tests table hash/array parts and sequential access under 32-bit constraints
local t = {}
for i = 1, 1000 do
  t[i] = i * i
end
-- verify first, last, and a middle entry
print(t[1])
print(t[500])
print(t[1000])
print(#t)
