-- test-lua-metatables.lua: metatable __index and __tostring
local mt = {}
mt.__index = mt
mt.__tostring = function(self) return "obj:" .. self.value end
mt.new = function(v) return setmetatable({value = v}, mt) end
mt.doubled = function(self) return self.value * 2 end

local obj = mt.new(21)
print(obj:doubled())
print(tostring(obj))
