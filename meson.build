project('final', 'c',
  version : '0.1',
  default_options : [
    'c_std=c11',
    'warning_level=3',
    'default_library=shared'
  ]
)

cmake = import('cmake')


# ##############
# Shared Dependencies
# ##############
lib_type = get_option('default_library')
sdl3_opts = cmake.subproject_options()
sdl3_opts.add_cmake_defines({
  'SDL_SHARED': lib_type == 'shared',
  'SDL_STATIC': lib_type == 'static',
  'SDL_TEST_LIBRARY': false,
  'CMAKE_POSITION_INDEPENDENT_CODE': true,  # For Linux
})
sdl3 = cmake.subproject('sdl', options: sdl3_opts)
sdl3_dep = sdl3.get_variable(lib_type == 'shared' ? 'SDL3_shared_dep' : 'SDL3_static_dep')



# ##############
# C Program
# ##############
c_exe = 'final-c'
c_sources = files(
  'c/main.c'
)
executable(c_exe,
  c_sources,
  c_args: ['-fopenmp'],
  link_args: ['-fopenmp'],
  dependencies: sdl3_dep,
)



# ##############
# Pony Program
# ##############
ponyc = find_program('ponyc')
pony_exe = 'final-pony'
pony_opts = [
  #'--path', additional_ponyc_path,
  #'--path', libzlib_dir,
  '--output', '@OUTDIR@',
  '--bin-name', pony_exe
]
# Source files aren't fed to ponyc directly, but meson does need them to
# determine when to recompile.
pony_sources = files(
  'pony/main.pony',
)

if host_machine.system() == 'windows'
  pony_exe = pony_exe + '.exe'
endif

final_pony = custom_target(pony_exe,
  output: pony_exe,
  input: pony_sources,
  build_by_default: true,
  command: [
    ponyc,
    pony_opts,
    '@SOURCE_ROOT@/pony',
  ]
)

