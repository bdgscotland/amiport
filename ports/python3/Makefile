# Makefile for CPython 3.11.12 (amiport)
#
# Phase 1: Compile core .o files
# Phase 2: Link python3 binary
# Phase 3+: Boot, REPL, stdlib (incremental)

include ../common.mk

TARGET      = python3
VERSION     = 3.11.12
REVISION    = 1
DESCRIPTION = Python 3.11 interpreter
AUTHOR      = Python Software Foundation
CATEGORY    = 2
AMINET_CAT  = dev/lang

# --- Compiler flags ---
# CPython 3.11 requires C11 (for stdatomic.h — we provide a local one).
# Use -m68020 minimum (68030 target, but 68020 instructions are compatible).
# -O0 initially per known-pitfalls (bebbo-gcc codegen bugs at -O2).
CFLAGS = -noixemul -m68020 -std=gnu11 -O0 \
	-Iported -Iported/Include -Iported/Include/internal \
	-DPy_BUILD_CORE -DNDEBUG -D__AMIGA__ \
	-Wall -Wno-unused-function -Wno-unused-variable \
	-Wno-missing-field-initializers -Wno-attributes

# Suppress warnings that CPython generates copiously
CFLAGS += -Wno-sign-compare -Wno-parentheses -Wno-format \
	-Wno-pointer-sign

# amiport shim includes (for stat, signal, time, etc.)
CFLAGS += -I../../lib/posix-shim/include

# Module-specific: built-in modules need Py_BUILD_CORE_BUILTIN
MOD_CFLAGS = $(CFLAGS) -DPy_BUILD_CORE_BUILTIN

# Linker
LDFLAGS = -noixemul -m68020 -L../../lib/posix-shim -lamiport -lm

# vamos
VAMOS_STACK = 1024
VAMOS_MEM = 16384

# --- Source file lists ---

# Python/ — interpreter core
PYTHON_SRCS = \
	ported/Python/Python-ast.c \
	ported/Python/Python-tokenize.c \
	ported/Python/_warnings.c \
	ported/Python/asdl.c \
	ported/Python/ast.c \
	ported/Python/ast_opt.c \
	ported/Python/ast_unparse.c \
	ported/Python/bltinmodule.c \
	ported/Python/bootstrap_hash.c \
	ported/Python/ceval.c \
	ported/Python/codecs.c \
	ported/Python/compile.c \
	ported/Python/context.c \
	ported/Python/dtoa.c \
	ported/Python/dynamic_annotations.c \
	ported/Python/dynload_stub.c \
	ported/Python/errors.c \
	ported/Python/fileutils.c \
	ported/Python/formatter_unicode.c \
	ported/Python/frame.c \
	ported/Python/frozen.c \
	ported/Python/frozenmain.c \
	ported/Python/future.c \
	ported/Python/getargs.c \
	ported/Python/getcompiler.c \
	ported/Python/getcopyright.c \
	ported/Python/getopt.c \
	ported/Python/getplatform.c \
	ported/Python/getversion.c \
	ported/Python/hamt.c \
	ported/Python/hashtable.c \
	ported/Python/import.c \
	ported/Python/importdl.c \
	ported/Python/initconfig.c \
	ported/Python/marshal.c \
	ported/Python/modsupport.c \
	ported/Python/mysnprintf.c \
	ported/Python/mystrtoul.c \
	ported/Python/pathconfig.c \
	ported/Python/preconfig.c \
	ported/Python/pyarena.c \
	ported/Python/pyctype.c \
	ported/Python/pyfpe.c \
	ported/Python/pyhash.c \
	ported/Python/pylifecycle.c \
	ported/Python/pymath.c \
	ported/Python/pystate.c \
	ported/Python/pystrcmp.c \
	ported/Python/pystrhex.c \
	ported/Python/pystrtod.c \
	ported/Python/pythonrun.c \
	ported/Python/pytime.c \
	ported/Python/specialize.c \
	ported/Python/structmember.c \
	ported/Python/suggestions.c \
	ported/Python/symtable.c \
	ported/Python/sysmodule.c \
	ported/Python/thread.c \
	ported/Python/traceback.c

# Objects/ — type implementations
OBJECTS_SRCS = \
	ported/Objects/abstract.c \
	ported/Objects/accu.c \
	ported/Objects/boolobject.c \
	ported/Objects/bytearrayobject.c \
	ported/Objects/bytes_methods.c \
	ported/Objects/bytesobject.c \
	ported/Objects/call.c \
	ported/Objects/capsule.c \
	ported/Objects/cellobject.c \
	ported/Objects/classobject.c \
	ported/Objects/codeobject.c \
	ported/Objects/complexobject.c \
	ported/Objects/descrobject.c \
	ported/Objects/dictobject.c \
	ported/Objects/enumobject.c \
	ported/Objects/exceptions.c \
	ported/Objects/fileobject.c \
	ported/Objects/floatobject.c \
	ported/Objects/frameobject.c \
	ported/Objects/funcobject.c \
	ported/Objects/genericaliasobject.c \
	ported/Objects/genobject.c \
	ported/Objects/interpreteridobject.c \
	ported/Objects/iterobject.c \
	ported/Objects/listobject.c \
	ported/Objects/longobject.c \
	ported/Objects/memoryobject.c \
	ported/Objects/methodobject.c \
	ported/Objects/moduleobject.c \
	ported/Objects/namespaceobject.c \
	ported/Objects/object.c \
	ported/Objects/obmalloc.c \
	ported/Objects/odictobject.c \
	ported/Objects/picklebufobject.c \
	ported/Objects/rangeobject.c \
	ported/Objects/setobject.c \
	ported/Objects/sliceobject.c \
	ported/Objects/structseq.c \
	ported/Objects/tupleobject.c \
	ported/Objects/typeobject.c \
	ported/Objects/unicodectype.c \
	ported/Objects/unicodeobject.c \
	ported/Objects/unionobject.c \
	ported/Objects/weakrefobject.c

# Parser/
PARSER_SRCS = \
	ported/Parser/action_helpers.c \
	ported/Parser/myreadline.c \
	ported/Parser/parser.c \
	ported/Parser/peg_api.c \
	ported/Parser/pegen.c \
	ported/Parser/pegen_errors.c \
	ported/Parser/string_parser.c \
	ported/Parser/token.c \
	ported/Parser/tokenizer.c

# Modules/ — mandatory built-in modules only
MODULES_SRCS = \
	ported/Modules/config.c \
	ported/Modules/main.c \
	ported/Modules/gcmodule.c \
	ported/Modules/posixmodule.c \
	ported/Modules/signalmodule.c \
	ported/Modules/errnomodule.c \
	ported/Modules/timemodule.c \
	ported/Modules/getpath_amiga.c \
	ported/Modules/getbuildinfo.c \
	ported/Modules/_io/_iomodule.c \
	ported/Modules/_io/bufferedio.c \
	ported/Modules/_io/bytesio.c \
	ported/Modules/_io/fileio.c \
	ported/Modules/_io/iobase.c \
	ported/Modules/_io/stringio.c \
	ported/Modules/_io/textio.c \
	ported/Modules/_io/winconsoleio.c \
	ported/Modules/_codecsmodule.c \
	ported/Modules/_weakref.c \
	ported/Modules/_collectionsmodule.c \
	ported/Modules/_sre/sre.c \
	ported/Modules/_functoolsmodule.c \
	ported/Modules/_operator.c \
	ported/Modules/itertoolsmodule.c \
	ported/Modules/atexitmodule.c \
	ported/Modules/_stat.c \
	ported/Modules/_threadmodule.c \
	ported/Modules/_tracemalloc.c \
	ported/Modules/faulthandler.c \
	ported/Modules/_abc.c \
	ported/Modules/symtablemodule.c \
	ported/Modules/mathmodule.c \
	ported/Modules/cmathmodule.c \
	ported/Modules/_struct.c \
	ported/Modules/binascii.c \
	ported/Modules/_json.c \
	ported/Modules/_bisectmodule.c \
	ported/Modules/_heapqmodule.c

# Programs/ — entry point
PROGRAMS_SRCS = \
	ported/Programs/python.c

# AmigaOS stubs — missing POSIX functions
AMIGAOS_SRCS = \
	ported/amigaos_stubs.c

# Deepfreeze — generated frozen module bytecode from host Python build
DEEPFREEZE_SRCS = \
	ported/Python/deepfreeze/deepfreeze.c

# Combine all
ALL_SRCS = $(PYTHON_SRCS) $(OBJECTS_SRCS) $(PARSER_SRCS) $(MODULES_SRCS) $(PROGRAMS_SRCS) $(AMIGAOS_SRCS) $(DEEPFREEZE_SRCS)
ALL_OBJS = $(ALL_SRCS:.c=.o)

# --- Build rules ---

.PHONY: all clean compile-check

all: $(TARGET)

$(TARGET): $(ALL_OBJS)
	$(CC) -o $@ $(ALL_OBJS) $(LDFLAGS)

# General rule for Python/ Objects/ Parser/ Programs/
ported/Python/%.o: ported/Python/%.c ported/pyconfig.h
	$(CC) $(CFLAGS) -c $< -o $@

ported/Objects/%.o: ported/Objects/%.c ported/pyconfig.h
	$(CC) $(CFLAGS) -c $< -o $@

ported/Parser/%.o: ported/Parser/%.c ported/pyconfig.h
	$(CC) $(CFLAGS) -c $< -o $@

ported/Programs/%.o: ported/Programs/%.c ported/pyconfig.h
	$(CC) $(CFLAGS) -c $< -o $@

# Modules need MOD_CFLAGS
ported/Modules/%.o: ported/Modules/%.c ported/pyconfig.h
	$(CC) $(MOD_CFLAGS) -c $< -o $@

ported/Modules/_io/%.o: ported/Modules/_io/%.c ported/pyconfig.h
	$(CC) $(MOD_CFLAGS) -c $< -o $@

ported/Modules/_sre/%.o: ported/Modules/_sre/%.c ported/pyconfig.h
	$(CC) $(MOD_CFLAGS) -c $< -o $@

# AmigaOS stubs
ported/amigaos_stubs.o: ported/amigaos_stubs.c ported/pyconfig.h
	$(CC) $(CFLAGS) -c $< -o $@

# Deepfreeze (154K lines -- suppress all warnings, takes a while to compile)
ported/Python/deepfreeze/deepfreeze.o: ported/Python/deepfreeze/deepfreeze.c ported/pyconfig.h
	$(CC) $(CFLAGS) -w -c $< -o $@

# Phase 1 gate: just compile, don't link
compile-check:
	@echo "Compiling core Python/ files..."
	@$(MAKE) $(patsubst %.c,%.o,$(PYTHON_SRCS)) 2>&1 | tail -20
	@echo "---"
	@echo "Compiling Objects/ files..."
	@$(MAKE) $(patsubst %.c,%.o,$(OBJECTS_SRCS)) 2>&1 | tail -20
	@echo "---"
	@echo "Compiling Parser/ files..."
	@$(MAKE) $(patsubst %.c,%.o,$(PARSER_SRCS)) 2>&1 | tail -20
	@echo "Done."

clean:
	find ported -name '*.o' -delete
	rm -f $(TARGET)
