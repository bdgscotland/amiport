# FindSDL2.cmake -- find libSDL2-amigaos3 for OpenTTD cross-compilation

set(SDL2_FOUND TRUE)
set(SDL2_INCLUDE_DIRS "/sdl2/include")
set(SDL2_LIBRARIES "/sdl2/libSDL2.a")
set(SDL2_VERSION "2.0.20")

if(NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 STATIC IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${SDL2_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(SDL2_INCLUDE_DIRS SDL2_LIBRARIES)
