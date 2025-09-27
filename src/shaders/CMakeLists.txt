find_program(GLSLANG glslang)

add_custom_command(
  OUTPUT
    "${PROJECT_BINARY_DIR}/shaders/gen-point-sprites.spv"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/shaders"
  COMMAND
    "${GLSLANG}"
    --target-env "vulkan1.0"
    -S comp
    -o "${PROJECT_BINARY_DIR}/shaders/gen-point-sprites.spv"
    "${CMAKE_CURRENT_SOURCE_DIR}/gen-point-sprites.glsl"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/gen-point-sprites.glsl"
)
add_custom_target(
  shader-gen-point-sprites
  ALL
  DEPENDS
    "${PROJECT_BINARY_DIR}/shaders/gen-point-sprites.spv"
)

add_custom_command(
  OUTPUT
    "${PROJECT_BINARY_DIR}/shaders/vertex.spv"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/shaders"
  COMMAND
    "${GLSLANG}"
    --target-env "vulkan1.0"
    -S vert
    -o "${PROJECT_BINARY_DIR}/shaders/vertex.spv"
    "${CMAKE_CURRENT_SOURCE_DIR}/vertex.glsl"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/vertex.glsl"
)
add_custom_target(
  vertex-shaders
  ALL
  DEPENDS
    "${PROJECT_BINARY_DIR}/shaders/vertex.spv"
)

add_custom_command(
  OUTPUT
    "${PROJECT_BINARY_DIR}/shaders/fragment.spv"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/shaders"
  COMMAND
    "${GLSLANG}"
    --target-env "vulkan1.0"
    -S frag
    -o "${PROJECT_BINARY_DIR}/shaders/fragment.spv"
    "${CMAKE_CURRENT_SOURCE_DIR}/fragment.glsl"
)
add_custom_target(
  fragment-shaders
  ALL
  DEPENDS
    "${PROJECT_BINARY_DIR}/shaders/fragment.spv"
)
