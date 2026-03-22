-- test-lua-coroutine.lua: basic coroutine yield and resume
local co = coroutine.create(function(a)
  coroutine.yield(a + 10)
  return a + 20
end)
local ok, v1 = coroutine.resume(co, 5)
local ok2, v2 = coroutine.resume(co)
print(v1)
print(v2)
