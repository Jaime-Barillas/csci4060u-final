glslang = find_program('glslang')

custom_target(
  'vertex_shaders',
  output: 'vertex.spv',
  input: files('vertex.glsl'),
  build_by_default: true,
  command: [
    glslang,
    '--target-env',
    'vulkan1.0',
    '-S',
    'vert',
    '@INPUT@',
    '-o',
    '@OUTPUT@'
  ]
)

custom_target(
  'fragment_shaders',
  output: 'fragment.spv',
  input: files('fragment.glsl'),
  build_by_default: true,
  command: [
    glslang,
    '--target-env',
    'vulkan1.0',
    '-S',
    'frag',
    '@INPUT@',
    '-o',
    '@OUTPUT@'
  ]
)
