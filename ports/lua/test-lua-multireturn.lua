-- test-lua-multireturn.lua: multiple return values and table.unpack
local function swap(a, b) return b, a end
local x, y = swap(1, 2)
print(x, y)

-- table.unpack
local t = {10, 20, 30}
print(table.unpack(t))

-- string.find returns multiple values
local s = "hello world"
local i, j = string.find(s, "world")
print(i, j)
