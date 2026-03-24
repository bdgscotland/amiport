# Makefile for less 692 (amiport)
# Category 3: Console UI — first console program in amiport
include ../common.mk

TARGET      = less
VERSION     = 692
REVISION    = 1
DESCRIPTION = GNU less pager for AmigaOS
AUTHOR      = Mark Nudelman
AMINET_CAT  = util/cli
CATEGORY    = 3

# amiport: Console-shim for termcap, posix-shim for termios
CONSOLE_SHIM_LIB = ../../lib/console-shim/libamiport-console.a
CFLAGS += -I../../lib/console-shim/include -Iported
LDFLAGS += -L../../lib/console-shim -lamiport-console

# amiport: ADR-022 permits gnu99 for ports where upstream requires C99
CFLAGS := $(subst -ansi,,$(CFLAGS))
CFLAGS += -std=gnu99

# amiport: Stack size — less does recursive operations (search, line parsing)
# vamos needs explicit stack via -s flag (libnix __stack not read at runtime)
VAMOS_STACK = 256

# Source files (37 .c files — everything except lessecho, lesskey, scrsize)
# amiport: lesskey_parse.c added — provides parse_lesskey/parse_lesskey_content for HAVE_LESSKEYSRC=1
SRCS = \
	ported/lesskey_parse.c \
	ported/brac.c \
	ported/ch.c \
	ported/charset.c \
	ported/cmdbuf.c \
	ported/command.c \
	ported/cvt.c \
	ported/decode.c \
	ported/edit.c \
	ported/evar.c \
	ported/filename.c \
	ported/forwback.c \
	ported/help.c \
	ported/ifile.c \
	ported/input.c \
	ported/jump.c \
	ported/line.c \
	ported/linenum.c \
	ported/lsystem.c \
	ported/main.c \
	ported/mark.c \
	ported/optfunc.c \
	ported/option.c \
	ported/opttbl.c \
	ported/os.c \
	ported/output.c \
	ported/pattern.c \
	ported/position.c \
	ported/prompt.c \
	ported/regexp.c \
	ported/screen.c \
	ported/search.c \
	ported/signal.c \
	ported/tags.c \
	ported/ttyin.c \
	ported/version.c \
	ported/xbuf.c

OBJECTS = $(SRCS:.c=.o)

.PHONY: all clean test package

all: $(TARGET)

# Per-file compilation for 36-file port
ported/%.o: ported/%.c $(SHIM_LIB) $(CONSOLE_SHIM_LIB)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS) $(SHIM_LIB) $(CONSOLE_SHIM_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "less 692 - AmigaOS 3.x port" | ../../toolchain/scripts/vamos -s $(VAMOS_STACK)k -C "" ./$(TARGET) -X -F -E
	@echo "=== $(TARGET) smoke test complete ==="

clean:
	rm -f ported/*.o $(TARGET).map
