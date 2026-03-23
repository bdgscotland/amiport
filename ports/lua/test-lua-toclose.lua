local log = {}
local function make_closeable(name)
  return setmetatable({}, {
    __close = function(self, err)
      table.insert(log, "closed:" .. name)
    end
  })
end
do
  local x <close> = make_closeable("x")
  local y <close> = make_closeable("y")
  table.insert(log, "inside")
end
for _, v in ipairs(log) do
  print(v)
end
