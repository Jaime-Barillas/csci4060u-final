set(
  PONY_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/main.pony"
)

add_custom_command(
  OUTPUT "${PROJECT_BINARY_DIR}/sph-pony"
  COMMAND
    ponyc
    --output "${PROJECT_BINARY_DIR}"
    --bin-name sph-pony
    --path "$<TARGET_FILE_DIR:SDL3::SDL3>"
    --path "$<TARGET_FILE_DIR:SDL3_ttf::SDL3_ttf>"
    --path "$<TARGET_FILE_DIR:ui>"
    --link-ldcmd "ld"
    "$<$<CONFIG:Debug>:--debug>"
    "$<$<CONFIG:Debug>:-DLogEnabled>"
    "${CMAKE_CURRENT_SOURCE_DIR}"
  DEPENDS
    ui
    ${PONY_SRCS}
  COMMENT "Compiling Pony program"
)
add_custom_target(
  sph-pony-target
  ALL
  DEPENDS
    "${PROJECT_BINARY_DIR}/sph-pony"
)
add_custom_target(sph-pony ALIAS sph-pony-target)
