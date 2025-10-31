cmake_minimum_required(VERSION 3.31)

project(
  sph-pony
  VERSION 1.0.0
)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(SDLTTF_VENDORED True)
add_subdirectory(subprojects/sdl3 EXCLUDE_FROM_ALL)
add_subdirectory(subprojects/sdl3_ttf EXCLUDE_FROM_ALL)
add_subdirectory(subprojects/catch2 EXCLUDE_FROM_ALL)

add_subdirectory(src/libcommon)
add_subdirectory(src/cpp)
add_subdirectory(test/cpp)
# add_subdirectory(src/pony)
add_subdirectory(src/shaders)
