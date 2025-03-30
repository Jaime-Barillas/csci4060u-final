project('final', 'cpp',
  version : '0.1',
  default_options : [
    'cpp_std=c++17',
    'warning_level=3',
    'default_library=static'
  ]
)



# ##############
# Shared Dependencies
# ##############
# SDL3, microui, and the libui Pony wrapper library are compiled into a single
# library dependency to accommodate how Pony links with static libraries. Pony
# expects the static library to end with the '.lib' extension on windows but
# meson uses a '.a' extension by default.
sdl3 = dependency('sdl3')
microui = dependency('microui')

libui_incdirs = include_directories('ui')
libui_srcs = files('ui/ui.cpp')
libui_name_suffix = []

# Use standard '.lib' extension when compiling static libraries on windows.
if host_machine.system() == 'windows' and get_option('default_library') == 'static'
  libui_name_suffix = 'lib'
endif

libui = library('ui',
  libui_srcs,
  include_directories: libui_incdirs,
  name_suffix: libui_name_suffix,
  dependencies: [sdl3.as_link_whole(), microui.as_link_whole()],
)



# ##############
# CPP Program
# ##############
cpp_exe = 'final-cpp'
cpp_sources = files(
  'cpp/main.cpp',
  'cpp/vec.cpp',
  'cpp/simulator.cpp',
)
# Provide sdl3 and microui as dependencies despite libui containing them.
# This ensures the necessary deps are linked against for SDL3 (using
# partial_dependency() does not seem to include them).
executable(cpp_exe,
  cpp_sources,
  include_directories: [libui_incdirs],
  link_with: [libui],
  cpp_args: ['-fopenmp'],
  link_args: ['-fopenmp'],
  dependencies: [sdl3, microui]
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

