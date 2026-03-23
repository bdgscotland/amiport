-- test-lua-closures.lua: closures and upvalue capture
local function make_counter(start)
  local n = start
  return function()
    n = n + 1
    return n
  end
end

local c1 = make_counter(0)
local c2 = make_counter(10)
print(c1())
print(c1())
print(c2())
