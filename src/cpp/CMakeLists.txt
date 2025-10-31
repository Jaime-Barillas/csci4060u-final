set(
  CPP_LIB_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/particles.cpp"
)

# Sequential C program
add_library(sph-cpp-lib STATIC ${CPP_LIB_SRCS})
target_include_directories(
  sph-cpp-lib
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
  sph-cpp-lib
  PUBLIC
    common
)

add_executable(
  sph-cpp
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
target_link_libraries(
  sph-cpp
  PUBLIC
    sph-cpp-lib
)
set_target_properties(
  sph-cpp
  PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Parallel C program
# TODO
find_package(Threads REQUIRED)
add_executable(
  sph-cpp-par
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  ${CPP_LIB_SRCS}
)
target_link_libraries(
  sph-cpp-par
  PUBLIC
    Threads::Threads
    sph-cpp-lib
    common
)
set_target_properties(
  sph-cpp-par
  PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
