project('chip8', ['c'],
    version: '1.0.0',
    default_options: [
        'buildtype=release',
        'c_std=c99',
        'c_thread_count=16'
    ]
)

dependencies = [
    dependency('sdl2', version : '>=2.0.0')
]

source = files([
    'chip8.c'
])

include = include_directories([
    '.'
])

flags = [
    '-Wall',
    '-Wextra',
    '-Wshadow',
    '-Wcast-align',
    '-Wunused',
    '-Wpedantic',
    '-Wmisleading-indentation',
    '-Wduplicated-cond',
    '-Wduplicated-branches',
    '-Wlogical-op',
    '-Wnull-dereference',
    '-Wdouble-promotion',
    '-Wformat=2',
    '-Wno-missing-field-initializers'
]

executable(
    'chip8',
    [ source ],
    dependencies : [ dependencies ] ,
    install: false,
    include_directories: [ include ],
    c_args: [ flags ],
    link_args: [ '-s' ]
)