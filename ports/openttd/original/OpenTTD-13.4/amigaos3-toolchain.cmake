# CMake toolchain file for cross-compiling OpenTTD to AmigaOS 3.x
# Uses bebbo-gcc 13.3 via Docker
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=../amigaos3-toolchain.cmake \
#         -DOPTION_DEDICATED=ON -DOPTION_USE_THREADS=OFF ..

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)

# Compiler paths (inside Docker container)
set(CMAKE_C_COMPILER m68k-amigaos-gcc)
set(CMAKE_CXX_COMPILER m68k-amigaos-g++)
set(CMAKE_AR m68k-amigaos-ar)
set(CMAKE_RANLIB m68k-amigaos-ranlib)

# Cross-compilation flags
set(CMAKE_C_FLAGS_INIT "-m68040 -m68881 -noixemul -D__libnix__")
set(CMAKE_CXX_FLAGS_INIT "-m68040 -m68881 -noixemul -D__libnix__ -fno-exceptions -fno-rtti")

# Big-endian 68k
set(CMAKE_C_BYTE_ORDER BIG_ENDIAN)
set(CMAKE_CXX_BYTE_ORDER BIG_ENDIAN)

# Don't try to run test binaries during configuration
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Disable features not available on AmigaOS
set(CMAKE_THREAD_LIBS_INIT "")
set(CMAKE_HAVE_THREADS_LIBRARY 0)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 0)
set(Threads_FOUND TRUE)

# Disable packages we don't have
set(ZLIB_FOUND FALSE)
set(PNG_FOUND FALSE)
set(LIBLZMA_FOUND FALSE)
set(LZO_FOUND FALSE)
set(Freetype_FOUND FALSE)
set(SDL2_FOUND FALSE)
set(Fontconfig_FOUND FALSE)
set(Fluidsynth_FOUND FALSE)
set(ICU_FOUND FALSE)
