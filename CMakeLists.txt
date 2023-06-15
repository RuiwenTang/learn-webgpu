cmake_minimum_required(VERSION 3.10)

project(leanr-webgpu)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_STANDARD 17)

# webgpu
add_subdirectory(deps/webgpu)

# common helper code
add_subdirectory(common)

add_subdirectory(hello-instance)
add_subdirectory(render-loop)
add_subdirectory(render-pipeline)