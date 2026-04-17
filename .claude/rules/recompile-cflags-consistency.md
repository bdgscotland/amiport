Paths: ports/openttd/**, scripts/recompile_*.sh, /tmp/recompile_*.sh

# Recompile Scripts Must Use EXACT CMake Flags

## The Rule

When using a hand-written shell script to rebuild C++ source files in a CMake-managed port (instead of running `cmake --build` / `make`), the script MUST source the EXACT `CXX_DEFINES`, `CXX_INCLUDES`, and `CXX_FLAGS` strings from `CMakeFiles/<target>.dir/flags.make`. Do NOT abbreviate them, even if the abbreviated version "looks equivalent".

## Why

C++ template instantiations (especially fmt, std::string, std::map operations) are highly sensitive to compile flags. Different `-W*` warnings, `-D*` defines, and even `-fno-*-vrp` style optimization tweaks can cause GCC to instantiate the same template differently across translation units.

When two .o files have inconsistent template instantiations of the same symbol (different code in the same `.text._ZN3fmt2v76detail*` section), the linker fails:

```
CMakeFiles/openttd.dir/src/window.cpp.o: duplicate section
  `.text._ZN3fmt2v76detail10vformat_toIcEEv...' has different size
collect2: error: ld returned 1 exit status
```

This is NOT a hook issue, NOT a code-transformer issue, NOT a bebbo-gcc bug. It is purely a build-environment hygiene issue.

## How to apply

In any port-specific recompile script:

```bash
# CORRECT — sources flags directly from CMake
DEFS=$(grep '^CXX_DEFINES' CMakeFiles/<target>.dir/flags.make | sed 's/CXX_DEFINES = //')
INCS=$(grep '^CXX_INCLUDES' CMakeFiles/<target>.dir/flags.make | sed 's/CXX_INCLUDES = //')
FLAGS=$(grep '^CXX_FLAGS' CMakeFiles/<target>.dir/flags.make | sed 's/CXX_FLAGS = //')

# Apply per-port modifications AFTER sourcing
FLAGS=$(echo "$FLAGS" | sed 's/-O0/-O1/g')   # whatever your port needs

# Use eval so quoted -D"x=y" args expand correctly
eval $CXX $DEFS $INCS $FLAGS -o $OUTFILE -c $SOURCE
```

```bash
# WRONG — hardcoded "looks equivalent" flags. Will eventually break.
FLAGS='-std=c++17 -m68040 -m68881 -O1 -noixemul -fpermissive -g'
$CXX $FLAGS -o $OUTFILE -c $SOURCE   # template instantiations differ vs CMake build
```

## Recovery

If you've already broken the build by mass-recompiling with abbreviated flags, the fix is to do a CLEAN CMake build (which requires `cmake` -- not present in the runtime image of `ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest`). Workaround:

```bash
docker run --rm -v $(pwd):/amiport -v /tmp:/tmp \
  ghcr.io/bdgscotland/amiport-toolchain-gcc13:latest bash -c '
    apt-get update -qq && apt-get install -y -qq cmake
    cd /amiport/ports/<port>/original/<src>/build-<config>
    make <target>  # may fail at link due to -rdynamic; that is fine, .o files are produced
  '
```

Then relink with the proper recompile script (which strips -rdynamic).

## Discovery

OpenTTD 13.4 port (PDR-015), 2026-04-16. A "rebuild all 439 .cpp files via batch script" attempt used a shorter FLAGS string than CMake's actual build (omitted `-D_DEBUG`, `-W`, `-Wcast-qual`, `-Wsign-compare`, `-Wundef`, `-flifetime-dse=1`, `-Wno-redundant-move`, etc.). Result: window.cpp.o had different fmt template instantiations than other .o files. Link failed with "duplicate section ... has different size" and could not be resolved with `-Wl,--allow-multiple-definition` or `-Wl,--no-gc-sections`. Required full clean CMake rebuild to recover.
