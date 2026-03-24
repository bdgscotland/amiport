# Makefile for mg (amiport)
include ../common.mk

TARGET      = mg
VERSION     = 3.7
REVISION    = 1
DESCRIPTION = Micro Emacs-like text editor
AUTHOR      = Joachim Wiberg (troglobit), OpenBSD
AMINET_CAT  = text/edit
CATEGORY    = 3

# mg is a multi-file port — compile each .c to .o, then link
SOURCES = \
    ported/ansi.c \
    ported/autoexec.c \
    ported/basic.c \
    ported/bell.c \
    ported/buffer.c \
    ported/cinfo.c \
    ported/cmode.c \
    ported/dir.c \
    ported/display.c \
    ported/echo.c \
    ported/extend.c \
    ported/extensions.c \
    ported/file.c \
    ported/fileio.c \
    ported/fparseln.c \
    ported/funmap.c \
    ported/help.c \
    ported/interpreter.c \
    ported/kbd.c \
    ported/keymap.c \
    ported/line.c \
    ported/log.c \
    ported/macro.c \
    ported/main.c \
    ported/match.c \
    ported/modes.c \
    ported/paragraph.c \
    ported/re_search.c \
    ported/region.c \
    ported/search.c \
    ported/spawn.c \
    ported/tags.c \
    ported/tty.c \
    ported/ttyio.c \
    ported/ttykbd.c \
    ported/undo.c \
    ported/util.c \
    ported/version.c \
    ported/window.c \
    ported/word.c \
    ported/yank.c

OBJECTS = $(SOURCES:.c=.o)

# C99 for long long (strtonum), designated initializers, for-init decls
CFLAGS += -std=gnu99

# WITHOUT_CURSES uses built-in ANSI backend (ansi.c) — no ncurses dependency
# Disable features requiring fork/exec (not available on AmigaOS)
CFLAGS += -DWITHOUT_CURSES -DREGEX
CFLAGS += -I ported

# Emu library for POSIX regex (include path + link)
CFLAGS  += -I../../lib/posix-emu/include
LDFLAGS += -L../../lib/posix-emu -lamiport-emu

# vamos stack (editor is recursive — display, buffer operations)
VAMOS_STACK = 256

# Per-file compilation for multi-file port
ported/%.o: ported/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS) $(SHIM_LIB)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

all: $(TARGET)

.PHONY: all clean test package compare

test: $(TARGET)
	@echo "=== Testing $(TARGET) ==="
	@echo "Category 3 — vamos smoke test only, full testing via FS-UAE"
	../../toolchain/scripts/vamos -C "" -s $(VAMOS_STACK) ./$(TARGET) -h
	@echo "=== $(TARGET) tests complete ==="

clean:
	rm -f ported/*.o $(TARGET).map
