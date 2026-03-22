-- test-lua-fileio.lua: write and read back a file on T: volume
-- Uses T: (RAM:T/) for temp storage as there is no /tmp on AmigaOS
local fname = "T:lua_test_io.txt"
local f = io.open(fname, "w")
f:write("amiga file io works\n")
f:close()
local g = io.open(fname, "r")
local line = g:read("*l")
g:close()
os.remove(fname)
print(line)
