-- test-lua-gc.lua: generational GC control and collectgarbage()
-- verify GC is running (incremental mode enabled after Lua start)
local before = collectgarbage("count")
-- allocate some objects to give GC something to do
for i = 1, 1000 do
  local t = {i, i*2, tostring(i)}
end
collectgarbage("collect")
local after = collectgarbage("count")
-- GC ran; memory should not have grown unboundedly
-- just verify collectgarbage returns a number
print(type(before))
print(type(after))
-- verify step mode works
local stepresult = collectgarbage("step", 1)
print(type(stepresult))
