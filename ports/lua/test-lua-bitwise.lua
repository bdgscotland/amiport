-- test-lua-bitwise.lua: Lua 5.4 bitwise operators (all six)
-- AND, OR, XOR, NOT, left shift, right shift
print(0xFF & 0x0F)     -- 15
print(0xF0 | 0x0F)     -- 255
print(0xFF ~ 0x0F)     -- 240 (XOR)
print(~0)              -- -1 (bitwise NOT, 32-bit long: all bits set = -1)
print(1 << 4)          -- 16
print(256 >> 4)        -- 16
