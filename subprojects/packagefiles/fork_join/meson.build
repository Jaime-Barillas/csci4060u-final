project('fork_join', 'c',
  version : '0.1.1',
)

fork_join_path = meson.project_source_root()

fork_join_dep = declare_dependency(
  variables: {'fork_join_path': fork_join_path},
)
meson.override_dependency('fork_join', fork_join_dep)
