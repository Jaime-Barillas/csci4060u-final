project('final', 'cpp', 'c',
  version : '0.1',
  default_options : [
    'cpp_std=c++17',
    'warning_level=3',
  ]
)

fs = import('fs')



# ##############
# Shared Dependencies
# ##############
sdl3_proj = subproject('sdl3', default_options: ['default_library=shared'])
sdl3 = sdl3_proj.get_variable('sdl3_dep')
sdl3_lib = sdl3_proj.get_variable('sdl_lib')
microui = dependency('microui', default_options: ['default_library=static'])

libui_incdirs = include_directories('ui')
libui_srcs = files('ui/ui.cpp')
libui_name_suffix = []

# Use standard '.lib' extension when compiling static libraries on windows.
if host_machine.system() == 'windows'
  libui_name_suffix = 'lib'
endif

libui = static_library('ui',
  libui_srcs,
  include_directories: libui_incdirs,
  name_suffix: libui_name_suffix,
  dependencies: [sdl3, microui.as_link_whole()],
)



# ##############
# CPP Program
# ##############
cpp_exe_sequential = 'final-cpp-sequential'
cpp_exe_parallel = 'final-cpp'
cpp_sources = files(
  'cpp/main.cpp',
  'cpp/vec.cpp',
  'cpp/simulator.cpp',
)
cpp_args_sequential = []
cpp_args_parallel = ['-fopenmp', '-DENABLE_PARALLELISM']
cpp_link_args = ['-fopenmp']

# SDL3 currently has a bug where debug text does not render correctly with the
# default renderer. On Windows, we must explicitly use the vulkan renderer.
if host_machine.system() == 'windows'
  if meson.get_compiler('cpp').get_id() == 'msvc'
    # msvc requires -openmp over -fopenmp
    cpp_args_parallel = ['-openmp', '-DENABLE_PARALLELISM']
    cpp_link_args = []
  endif
  cpp_args_sequential += '-DUSE_VULKAN_RENDERER'
  cpp_args_parallel += '-DUSE_VULKAN_RENDERER'
endif
# Provide sdl3 and microui as dependencies despite libui containing them.
# This ensures the necessary deps are linked against for SDL3 (using
# partial_dependency() does not seem to include them).
executable(cpp_exe_sequential,
  cpp_sources,
  include_directories: [libui_incdirs],
  link_with: [libui],
  cpp_args: cpp_args_sequential,
  link_args: cpp_link_args,
  build_rpath: '$ORIGIN',
  dependencies: [sdl3, microui]
)

executable(cpp_exe_parallel,
  cpp_sources,
  include_directories: [libui_incdirs],
  link_with: [libui],
  cpp_args: cpp_args_parallel,
  link_args: cpp_link_args,
  build_rpath: '$ORIGIN',
  dependencies: [sdl3, microui]
)



# ##############
# Pony Program
# ##############
ponyc = find_program('ponyc', version: '>=0.58.10', required: false)

if ponyc.found()
  pony_exe = 'final-pony'
  pony_opts = []

  if get_option('debug') == true
    pony_opts += '--debug'
  endif

  if host_machine.system() == 'windows'
    pony_exe += '.exe'
    pony_opts += '-DUSE_VULKAN_RENDERER'
  endif

  pony_opts += [
    '--path', fs.parent(libui.full_path()),
    '--path', fs.parent(sdl3_lib.full_path()),
    '--output', '@OUTDIR@',
    '--bin-name', pony_exe
  ]

  # Source files aren't fed to ponyc directly, but meson does need them to
  # determine when to recompile.
  pony_sources = files(
    'pony/main.pony',
    'pony/sdl.pony',
    'pony/ui.pony',
  )

  final_pony = custom_target('final-pony',
    output: pony_exe,
    input: pony_sources,
    build_by_default: true,
    depends: libui,
    command: [
      ponyc,
      pony_opts,
      '@SOURCE_ROOT@/pony',
    ]
  )
endif

