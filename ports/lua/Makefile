# Makefile for lua (amiport)
include ../common.mk

TARGET      = lua
VERSION     = 5.4.7
DESCRIPTION = Lua 5.4.7 scripting language for AmigaOS
AUTHOR      = Roberto Ierusalimschy / PUC-Rio

# Category 2 scripting interpreter — posix-shim only
AMINET_CAT  = dev/lang

# Lua core library sources (everything except lua.c and luac.c)
LUA_CORE = \
	ported/lapi.c ported/lauxlib.c ported/lbaselib.c ported/lcode.c \
	ported/lcorolib.c ported/lctype.c ported/ldblib.c ported/ldebug.c \
	ported/ldo.c ported/ldump.c ported/lfunc.c ported/lgc.c \
	ported/linit.c ported/liolib.c ported/llex.c ported/lmathlib.c \
	ported/lmem.c ported/loadlib.c ported/lobject.c ported/lopcodes.c \
	ported/loslib.c ported/lparser.c ported/lstate.c ported/lstring.c \
	ported/lstrlib.c ported/ltable.c ported/ltablib.c ported/ltm.c \
	ported/lundump.c ported/lutf8lib.c ported/lvm.c ported/lzio.c

# Interpreter main
LUA_MAIN = ported/lua.c

SOURCES = $(LUA_CORE) $(LUA_MAIN)

# Object files for separate compilation
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean test package

all: $(TARGET)

# Compile each .c to .o separately (faster rebuilds, avoids long command lines)
ported/%.o: ported/%.c $(SHIM_LIB)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS) -lm

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@# Test 1: version flag
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -v
	@# Test 2: evaluate expression
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -e "print(2+3)"
	@# Test 3: string operations
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -e "print(string.upper('hello amiga'))"
	@# Test 4: table operations
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -e "t={1,2,3}; print(#t)"
	@# Test 5: math library
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -e "print(math.sqrt(144))"
	@# Test 6: for loop
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -e "s=0; for i=1,100 do s=s+i end; print(s)"
	@# Test 7: function definition and call
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -e "function fib(n) if n<2 then return n else return fib(n-1)+fib(n-2) end end; print(fib(10))"
	@# Test 8: os.clock
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -e "print(type(os.clock()))"
	@# Test 9: io.write
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -e "io.write('Hello from Lua on AmigaOS!\n')"
	@# Test 10: integer arithmetic (Lua 5.4 feature)
	../../toolchain/scripts/vamos -C "" ./$(TARGET) -e "print(7 // 2)"
	@echo "=== $(TARGET) tests complete ==="

clean:
	rm -f $(TARGET) ported/*.o $(TARGET)-*.lha
