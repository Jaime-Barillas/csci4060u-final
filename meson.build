project('final', 'cpp',
  version : '0.1',
  default_options : [
    'cpp_std=c++17',
    'warning_level=3',
    'default_library=shared'
  ]
)



# ##############
# Shared Dependencies
# ##############
sdl3 = dependency('sdl3')
microui = dependency('microui')



# ##############
# CPP Program
# ##############
cpp_exe = 'final-cpp'
cpp_sources = files(
  'cpp/main.cpp',
)
executable(cpp_exe,
  cpp_sources,
  cpp_args: ['-fopenmp'],
  link_args: ['-fopenmp'],
  dependencies: [sdl3, microui],
)



# ##############
# Pony Program
# ##############
# ponyc = find_program('ponyc')
# pony_exe = 'final-pony'
# pony_opts = [
#   #'--path', additional_ponyc_path,
#   #'--path', libzlib_dir,
#   '--output', '@OUTDIR@',
#   '--bin-name', pony_exe
# ]
# # Source files aren't fed to ponyc directly, but meson does need them to
# # determine when to recompile.
# pony_sources = files(
#   'pony/main.pony',
# )

# if host_machine.system() == 'windows'
#   pony_exe = pony_exe + '.exe'
# endif

# final_pony = custom_target(pony_exe,
#   output: pony_exe,
#   input: pony_sources,
#   build_by_default: true,
#   command: [
#     ponyc,
#     pony_opts,
#     '@SOURCE_ROOT@/pony',
#   ]
# )

