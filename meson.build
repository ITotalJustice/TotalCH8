project('TotalCH8', 'c',
  version : '1.0.0',
  license : 'MIT',
  default_options : [
    'warning_level=3',
    'optimization=3',
    'strip=true',
    'c_std=c99',
  ]
)

c_flags = [
]

c_warnings = [
  # warns
  '-Wall',
  '-Wextra',
  '-Werror',
  '-Wfatal-errors',
  '-Wpedantic',
  '-Wshadow',
  '-Wdouble-promotion',
  '-Wformat=2',
  '-Wformat-overflow=2',
  '-Wformat-truncation=1',
  '-Wundef',
  '-Wmissing-include-dirs',
  '-Wstrict-aliasing=2',
  '-Walloca',
  '-Wduplicated-branches',
  '-Wduplicated-cond',
  '-Wwrite-strings',
  '-Wdate-time',
  '-Wsizeof-pointer-memaccess',
  '-Wlogical-op',
  '-Wpacked',
  '-Wredundant-decls',
  '-Wnested-externs',
  '-Winline',
]

source = files([
    'src/ch8.c',
])

# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = [
  '-DBUILDING_LIB',
]

shlib = shared_library(
  'TotalCH8', source,
  install : true,
  c_args : [lib_args, c_warnings, c_flags],
  gnu_symbol_visibility : 'hidden',
)

test_sanity_exe = executable(
  'args_test', 'tests/sanity.c',
  link_with : shlib
)

test('sanity', test_sanity_exe)

# Make this library usable as a Meson subproject.
test_dep = declare_dependency(
  include_directories: include_directories('src'),
  link_with : shlib
)

# Make this library usable from the system's
# package manager.
install_headers('src/ch8.h', subdir : 'TotalCH8')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'TotalCH8',
  filebase : 'TotalCH8',
  description : 'Chip8 emulator core!',
  subdirs : 'TotalCH8',
  libraries : shlib,
  version : '1.0.0',
)

# scripts
run_target('cppcheck',
  command : 'scripts/cppcheck.sh')
