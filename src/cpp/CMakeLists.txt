set(
  CPP_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

# Sequential C program
add_executable(
  sph-cpp
  ${CPP_SRCS}
)
target_link_libraries(
  sph-cpp
  PUBLIC
    common
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
  ${CPP_SRCS}
)
target_link_libraries(
  sph-cpp-par
  PUBLIC
    Threads::Threads
    common
)
set_target_properties(
  sph-cpp-par
  PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
