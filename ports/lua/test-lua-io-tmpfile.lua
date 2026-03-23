-- test-lua-io-tmpfile.lua: io.tmpfile() via amiport_tmpfile()
-- creates an anonymous temp file on T:, writes, seeks back, reads
local f = io.tmpfile()
if f == nil then
  print("tmpfile failed")
  os.exit(10)
end
f:write("tmpfile content\n")
f:seek("set", 0)
local line = f:read("*l")
f:close()
print(line)
