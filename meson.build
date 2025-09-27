project(
  'pendulums',
  ['cpp', 'c'],
  version: '0.1',
  default_options: ['warning_level=3', 'cpp_std=c++20'],
)


cmake = import('cmake')
box2d_opts = cmake.subproject_options()
box2d_opts.set_install(false)

box2d_subproject = cmake.subproject('box2d', options: box2d_opts)

dependencies = []

dependencies += dependency('raylib')
dependencies += box2d_subproject.dependency('box2d')
# dependencies+=dependency('box2d') # Link with shared library

sources = ['main.cpp']

executable(
  meson.project_name(),
  sources,
  dependencies: dependencies,
)

