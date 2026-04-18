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
set(AMIGA_COMMON_FLAGS "-m68020 -O1 -noixemul -D__libnix__ -DTTD_ENDIAN=1 -Dalloca=__builtin_alloca")
set(AMIGA_THREAD_STUB "-include ${CMAKE_CURRENT_LIST_DIR}/ported/amiga_openttd_stubs.h")
set(AMIGA_STUB_INCLUDE "-I${CMAKE_CURRENT_LIST_DIR}/ported")

set(CMAKE_C_FLAGS_INIT "${AMIGA_COMMON_FLAGS} ${AMIGA_STUB_INCLUDE} ${AMIGA_THREAD_STUB}")
set(CMAKE_CXX_FLAGS_INIT "${AMIGA_COMMON_FLAGS} ${AMIGA_STUB_INCLUDE} ${AMIGA_THREAD_STUB} -fpermissive")

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
set(Fontconfig_FOUND FALSE)
set(Fluidsynth_FOUND FALSE)
set(ICU_FOUND FALSE)

# SDL2 -- Duncan's libSDL2-amigaos3 port (mounted into Docker at /sdl2).
# Force CMake to pick up the static lib without find_package fingerprinting
# (which fails on cross-built archives -- it tries to run a test binary).
set(SDL2_FOUND TRUE)
set(SDL2_INCLUDE_DIRS "/sdl2/include")
set(SDL2_INCLUDE_DIR  "/sdl2/include")
set(SDL2_LIBRARIES    "/sdl2/libSDL2.a")
set(SDL2_LIBRARY      "/sdl2/libSDL2.a")
# Imported target form -- some OpenTTD CMake paths use SDL2::SDL2
if(NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 STATIC IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "/sdl2/libSDL2.a"
        INTERFACE_INCLUDE_DIRECTORIES "/sdl2/include"
    )
endif()
