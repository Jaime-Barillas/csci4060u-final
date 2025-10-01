set(
  COMMON_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/lib.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib-c.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/matrix.cpp"
)

add_library(common STATIC ${COMMON_SRCS})
target_include_directories(
  common
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
  common
  PUBLIC
    SDL3_ttf::SDL3_ttf
    SDL3::SDL3
)
target_compile_definitions(
  common
  PRIVATE
    "$<$<CONFIG:Debug>:LogEnabled=1>"
)
