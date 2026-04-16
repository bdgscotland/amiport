# CMake toolchain file for cross-compiling OpenTTD to AmigaOS 3.x
# Uses bebbo-gcc 13.3 via Docker
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=../amigaos3-toolchain.cmake \
#         -DOPTION_DEDICATED=ON -DOPTION_USE_THREADS=OFF \
#         -DHOST_BINARY_DIR=../build-host -DUNIX=1 ..

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR m68k)

# Compiler paths (inside Docker container)
set(CMAKE_C_COMPILER m68k-amigaos-gcc)
set(CMAKE_CXX_COMPILER m68k-amigaos-g++)
set(CMAKE_AR m68k-amigaos-ar)
set(CMAKE_RANLIB m68k-amigaos-ranlib)

# AmigaOS-specific defines:
# -DTTD_ENDIAN=1        big-endian 68k
# -Dalloca=...          libnix has no alloca
# -Duint=...            UNIX mode skips uint typedef in stdafx.h
# -include ...          no-op mutex/condition_variable stubs
set(AMIGA_COMMON_FLAGS "-m68040 -m68881 -O0 -noixemul -D__libnix__ -DTTD_ENDIAN=1 -Dalloca=__builtin_alloca")
set(AMIGA_THREAD_STUB "-include ${CMAKE_CURRENT_LIST_DIR}/original/OpenTTD-13.4/amigaos3/amiga_thread_stubs.h")

set(CMAKE_C_FLAGS_INIT "${AMIGA_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${AMIGA_COMMON_FLAGS} ${AMIGA_THREAD_STUB} -fno-exceptions -fno-rtti")

# Big-endian 68k
set(CMAKE_C_BYTE_ORDER BIG_ENDIAN)
set(CMAKE_CXX_BYTE_ORDER BIG_ENDIAN)

# Don't try to run test binaries during configuration
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Force UNIX so OpenTTD's build system recognizes directory paths
set(UNIX TRUE)

# Disable threading
set(CMAKE_THREAD_LIBS_INIT "")
set(CMAKE_HAVE_THREADS_LIBRARY 0)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 0)
set(Threads_FOUND TRUE)

# Disable packages not yet ported
set(ZLIB_FOUND FALSE)
set(PNG_FOUND FALSE)
set(LIBLZMA_FOUND FALSE)
set(LZO_FOUND FALSE)
set(Freetype_FOUND FALSE)
set(SDL2_FOUND FALSE)
set(Fontconfig_FOUND FALSE)
set(Fluidsynth_FOUND FALSE)
set(ICU_FOUND FALSE)
