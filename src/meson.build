dep = [
]

inc = include_directories(
    '.',
    'tests',
    'profile'
)

src = [
    'main.cpp',
    'json.cpp',
    'descriptions.cpp',
    'transport_catalog.cpp',
    'transport_router.cpp',
    'utils.cpp',
    'sphere.cpp',
    'requests.cpp',
    'tests/test_runner.cpp'
]

white = executable('white',
    sources : src,
    include_directories : inc,
    cpp_args: ['-DNOMINMAX']
)


