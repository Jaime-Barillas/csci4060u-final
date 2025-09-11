set(
  UI_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/ui.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/log.c"
)

add_library(
  ui
  STATIC
  ${UI_SRCS}
)
target_include_directories(
  ui
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
  ui
  PUBLIC
    SDL3_ttf::SDL3_ttf
    SDL3::SDL3
)
target_compile_definitions(
  ui
  PRIVATE
    "$<$<CONFIG:Debug>:LogEnabled=1>"
)
