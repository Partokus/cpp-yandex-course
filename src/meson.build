global_inc = include_directories('.')

linker_script_flags = []
foreach entry : meson.get_cross_property('linker_scripts', [''])
if entry != ''
  linker_script_flags += '-T' + entry
endif
endforeach
add_project_link_arguments(linker_script_flags, language: ['c', 'cpp'])

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
    'tests/test_runner.cpp'
]

if host_machine.system() == 'windows'
    winmm_lib = meson.get_compiler('cpp').find_library('winmm', required : true)
    win_dep = [winmm_lib]
endif

white = executable('white',
    sources : src,
    include_directories : inc,
    dependencies : win_dep,
    cpp_args: ['-DNOMINMAX']
)


