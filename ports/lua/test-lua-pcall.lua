-- test-lua-pcall.lua: pcall catches runtime errors and returns false+msg
local ok, err = pcall(function()
  error("deliberate error")
end)
print(ok)
print(type(err))

-- xpcall with message handler
local ok2, msg2 = xpcall(function()
  error("xpcall error")
end, function(e) return "caught: " .. e end)
print(ok2)
print(msg2)
