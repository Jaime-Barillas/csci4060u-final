project('final', 'c',
  version : '0.1',
  default_options : [
    'c_std=c11',
    'warning_level=3',
    'default_library=static'
  ]
)

fs = import('fs')



# ##############
# Shared Dependencies
# ##############
sdl3 = subproject('sdl')
sdl3_dep = sdl3.get_variable('SDL3_shared_dep')



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

