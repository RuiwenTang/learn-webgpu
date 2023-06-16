add_executable(
        uniform-buffer
        main.cc
)

target_compile_definitions(uniform-buffer PRIVATE -DASSET_DIR="${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(uniform-buffer PRIVATE webgpu util)