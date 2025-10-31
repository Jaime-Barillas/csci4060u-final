set(
  CPP_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/generators.cpp"
)

add_executable(
  sph-cpp-test
  ${CPP_SRCS}
)
target_link_libraries(
  sph-cpp-test
  PUBLIC
    sph-cpp-lib
    Catch2::Catch2WithMain
)
set_target_properties(
  sph-cpp-test
  PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
