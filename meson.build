project(
    'white',
    ['c', 'cpp'],
    default_options : [
        'c_std=c99',
        'cpp_std=c++17',
        'buildtype=debug',
        'optimization=g',
        # 'b_sanitize=address,undefined'
    ],
    version: '0.1.0'
)

# add_project_arguments('-D_GLIBCXX_DEBUG', language: 'cpp')
# add_project_arguments('-D_GLIBCXX_DEBUG_PEDANTIC', language: 'cpp')
# add_project_arguments('-D_LIBCPP_DEBUG=1', language: 'cpp')

subdir('src')