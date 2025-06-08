project('fluid-sim-sph', 'c',
  version : '0.1',
  default_options : [
    'warning_level=3',
  ],
)

fs = import('fs')

ponyc = find_program('ponyc', version: '>=0.58.10')
proj = meson.project_name()
# Ensure ponyc uses the default linker for the compiler. Chimera Linux
# doesn't provide the default gold linker.
ld = meson.get_compiler('c').get_linker_id()
if ld.startswith('ld.')
  ld = ld.split('.')[1]
endif
windows = host_machine.system() == 'windows'


#################
# C Wrapper Lib #
#################
microui_proj = subproject('microui', default_options: ['default_library=static'])
microui = microui_proj.get_variable('microui_dep')
microui_path = fs.parent(microui_proj.get_variable('libmicroui').full_path())
# If sdl3 comes from meson's wrapdb, we need to pass the lib's path to the
# Pony compiler, this is not necessary with the system level dependency. In
# fact, it is not possible to get the path to the lib if dependency() is
# used.
sdl3 = dependency(
  'sdl3',
  default_options: ['default_library=shared'],
  allow_fallback: false,
  required: false,
)
sdl3_path = false
if not sdl3.found()
  sdl3_proj = subproject('sdl3', default_options: ['default_library=shared'])
  sdl3 = sdl3_proj.get_variable('sdl3_dep')
  sdl3_path = fs.parent(sdl3_proj.get_variable('sdl_lib').full_path())
endif

libwrapper_srcs = [
  'c' / 'microui' / 'lib.c',
  'c' / 'sdl3' / 'lib.c',
]

libwrapper = static_library('wrapper',
  libwrapper_srcs,
  name_suffix: windows ? 'lib' : [], # ponyc requires .lib suffix on windows
  dependencies: [sdl3, microui],
)

libwrapper_path = fs.parent(libwrapper.full_path())


########
# Pony #
########
# Source files aren't fed to ponyc directly, but meson does need them to
# determine when to recompile.
pony_sources = files(
  proj / 'main.pony',
  proj / 'sdl3' / 'sdl3.pony',
  proj / 'sdl3' / 'display.pony',
)
pony_output = windows ? proj + '.exe' : proj
pony_opts = [
  '--path', microui_path,
  '--path', libwrapper_path,
  '--link-ldcmd', ld,
  '--output', '@OUTDIR@',
]
if sdl3_path
  pony_opts += ['--path', sdl3_path]
endif

if get_option('debug')
  pony_opts += '--debug'
endif

if windows
  # The default renderer cannot draw text on Windows.
  # TODO: Investigate if it's still an issue...
  pony_opts += '-DUSE_VULKAN_RENDERER'
endif

message('ponyc opts: ' + ' '.join(pony_opts) + '\n')
final_pony = custom_target(proj,
  output: pony_output,
  input: pony_sources,
  build_by_default: true,
  depends: [libwrapper],
  command: [
    ponyc,
    pony_opts,
    '@SOURCE_ROOT@' / proj,
  ],
)
