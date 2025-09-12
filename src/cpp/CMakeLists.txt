set(
  C_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

# Sequential C program
add_executable(
  sph-c
  ${C_SRCS}
)
target_link_libraries(
  sph-c
  PUBLIC
    ui
)
set_target_properties(
  sph-c
  PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

# Parallel C program
# TODO
find_package(Threads REQUIRED)
add_executable(
  sph-c-par
  ${C_SRCS}
)
target_link_libraries(
  sph-c-par
  PUBLIC
    Threads::Threads
    ui
)
set_target_properties(
  sph-c-par
  PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
