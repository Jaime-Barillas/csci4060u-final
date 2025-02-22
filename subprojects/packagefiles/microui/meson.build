project('microui', 'c',
  version : '2.02',
  default_options: ['default_library=static']
)

inc = include_directories('src')

libmicroui = library('microui',
  'src/microui.c',
  include_directories: inc,
)

libmicroui_dep = declare_dependency(
  include_directories: inc,
  link_with: libmicroui
)

meson.override_dependency('microui', libmicroui_dep)

