-- test-lua-runtime-error.lua: triggers a runtime error
-- calling a nil value is a classic runtime error
local x = nil
x()
