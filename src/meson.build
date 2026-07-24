dep = [
]

inc = include_directories(
    '.',
    'tests',
    'profile'
)

src = [
    'main.cpp',
    'tests/test_runner.cpp',
    'json_printer.cpp'
]

white = executable('white',
    sources : src,
    include_directories : inc,
    cpp_args: ['-DNOMINMAX']
)


